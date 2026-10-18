#include "drone.hpp"
#include "imu_generation.hpp"
#include "positionController.hpp"
#include "rc_parser.hpp"
#include "serialize.hpp"
#include "spi_interface.hpp"
#include "velocityController.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
struct Options {
    std::string transport = "capture";
    std::string capture = "spi_frames.bin";
    std::string device = "/dev/spidev0.0";
    std::string path = FLIGHT_SIM_DEFAULT_PATH;
    uint32_t speedHz = 500000;
    uint32_t samples = 1800;
    uint32_t rateHz = 60;
    uint16_t lidarMm = 1000;
    bool fixture = false;
    bool unpaced = false;
};

uint32_t number(const std::string& text, uint32_t min, uint32_t max) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Expected an unsigned integer: " + text);
    const auto value = std::stoull(text);
    if (value < min || value > max) throw std::invalid_argument("Value out of range: " + text);
    return static_cast<uint32_t>(value);
}

Options parseOptions(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--fixture") { o.fixture = true; continue; }
        if (arg == "--unpaced") { o.unpaced = true; continue; }
        if (i + 1 == argc) throw std::invalid_argument("Missing value for " + arg);
        const std::string value = argv[++i];
        if (arg == "--transport") o.transport = value;
        else if (arg == "--capture") o.capture = value;
        else if (arg == "--device") o.device = value;
        else if (arg == "--path") o.path = value;
        else if (arg == "--speed-hz") o.speedHz = number(value, 10000, 10000000);
        else if (arg == "--samples") o.samples = number(value, 1, 10000000);
        else if (arg == "--rate-hz") o.rateHz = number(value, 1, 200);
        else if (arg == "--lidar-mm") o.lidarMm = static_cast<uint16_t>(number(value, 0, 65535));
        else if (arg == "--timing" && value == "immediate") { /* Only supported MVP mode. */ }
        else throw std::invalid_argument("Unsupported option/value: " + arg + " " + value);
    }
    if (o.transport != "capture" && o.transport != "spi")
        throw std::invalid_argument("Transport must be capture or spi");
    if (o.unpaced && o.transport == "spi")
        throw std::invalid_argument("--unpaced is only allowed for capture");
    if (o.transport == "spi" && uint64_t{o.speedHz} < uint64_t{o.rateHz} * 256 * 8 * 2)
        throw std::invalid_argument("SPI speed must leave at least half each period for receiver rearming");
    return o;
}

void help() {
    std::cout << "Usage: flight_sim [options]\n"
        "  --transport capture|spi    Default: capture\n"
        "  --capture FILE             Default: spi_frames.bin (overwritten)\n"
        "  --device PATH              Default: /dev/spidev0.0 (Linux)\n"
        "  --speed-hz N               Default: 500000; mode 0, 8-bit, MSB first\n"
        "  --timing immediate         Apply on receipt; no clock synchronization\n"
        "  --samples N                Default: 1800\n"
        "  --rate-hz N                 Default: 60 (1..200)\n"
        "  --path FILE                Flightpath input (default: repository test.txt)\n"
        "  --lidar-mm N               Combined-packet placeholder, default: 1000\n"
        "  --fixture                  Send a known sample instead of physics\n"
        "  --unpaced                  Disable wall-clock pacing for capture tests\n";
}

imu_data_t fixtureSample() {
    auto pack = [](int16_t raw) {
        const auto bits = static_cast<uint16_t>(raw);
        return i2c_imu_data_16_t{static_cast<int8_t>(bits & 0xff), static_cast<int8_t>(bits >> 8)};
    };
    return {{pack(1440), pack(-160), pack(320)},
            {pack(100), pack(-200), pack(300)},
            {pack(-16), pack(32), pack(-48)}};
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") { help(); return 0; }
        const auto options = parseOptions(argc, argv);
        const auto path = options.fixture ? std::vector<Eigen::Vector3d>{Eigen::Vector3d::Zero()}
                                          : rc_read(options.path);
        if (path.empty()) throw std::runtime_error("Flightpath contains no waypoints");

        auto transport = options.transport == "spi"
            ? makeLinuxSpiTransport(options.device, options.speedHz)
            : makeCaptureTransport(options.capture);
        std::ofstream log("pid_tuning.csv", std::ios::trunc);
        log.exceptions(std::ios::failbit | std::ios::badbit);
        log << "Time,X,Y,Z,errorZ,forceZ\n";

        Drone drone(new RigidBody(1.0, Eigen::Matrix3d::Identity()),
                    new RigidBody(0.5, Eigen::Matrix3d::Identity(), {1, 0, 0}),
                    new RigidBody(0.5, Eigen::Matrix3d::Identity(), {-1, 0, 0}),
                    new RigidBody(0.5, Eigen::Matrix3d::Identity(), {0, 1, 0}),
                    new RigidBody(0.5, Eigen::Matrix3d::Identity(), {0, -1, 0}));
        ImuSimulator imu;
        positionController positionControl({0.5, 0.5, 2.0});
        velocityController velocityControl({1, 1, 3}, {0.1, 0.1, 0.5}, {0.05, 0.05, 0.2}, 20);
        using Clock = std::chrono::steady_clock;
        const double dt = 1.0 / options.rateHz;
        const auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(dt));
        auto deadline = Clock::now() + period;
        uint32_t overruns = 0;
        std::cout << "Transport=" << options.transport << " timing=immediate rate=" << options.rateHz
                  << " Hz; timestamps are simulation-relative microseconds.\n"
                  << "LiDAR placeholder=" << options.lidarMm << " mm, RC=no-op.\n";
        for (uint32_t step = 0; step < options.samples; ++step) {
            if (!options.unpaced) std::this_thread::sleep_until(deadline);
            const double startTime = double(step) / options.rateHz;
            const double sampleTime = double(step + 1) / options.rateHz;
            const size_t waypoint = std::min(static_cast<size_t>(startTime / 0.5), path.size() - 1);
            positionControl.setTarget(path[waypoint]);
            Eigen::Vector3d force = Eigen::Vector3d::Zero();
            imu_data_t data{};
            if (options.fixture) {
                data = fixtureSample();
            } else {
                const auto velocity = positionControl.compute(drone.getPosition(), dt);
                force = velocityControl.compute(drone.getVelocity(), velocity, dt) * drone.getMass();
                force.z() += drone.getMass() * 9.81;
                drone.applyForce(force);
                drone.applyForce({0, 0, -drone.getMass() * 9.81});
                drone.update(dt);
                data = imu.sample(drone.getOrientation(), drone.getAngularVelocity(), drone.getAcceleration());
            }
            // This is metadata in immediate mode, never a test-node deadline.
            // Integer arithmetic avoids floating-point accumulation; wraps at 2^32 us.
            const auto timestamp = static_cast<uint32_t>((uint64_t{step + 1} * 1000000) / options.rateHz);
            const auto frame = encodeSensorFrame(data, timestamp, options.lidarMm);
            transport->transmit(frame.data(), frame.size());
            const auto p = drone.getPosition();
            log << sampleTime << ',' << p.x() << ',' << p.y() << ',' << p.z() << ','
                << (positionControl.getTarget() - p).z() << ',' << force.z() << '\n';
            deadline += period;
            if (!options.unpaced && Clock::now() >= deadline) {
                ++overruns;
                // Never burst old samples into a slave that has not rearmed.
                deadline = Clock::now() + period;
            }
        }
        log.close();
        std::cout << "Completed " << options.samples << " frames; pacing overruns=" << overruns
                  << ". SPI completion does not acknowledge receiver decoding.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "flight_sim: " << error.what() << '\n';
        return 1;
    }
}
