#include "imu_generation.hpp"
#include <chrono>
#include <flight_sim.hpp>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <thread>
#include <stdexcept>
#include <string>
#include <memory>

void initializeLogging();
void logData(double t, const Eigen::Vector3d &desiredPos,
             const Eigen::Vector3d &currentPos, const Eigen::Vector3d &error,
             const Eigen::Vector3d &controlOutput);
void closeLogging();

std::ofstream logFile;

struct Waypoint {
  double time;
  Eigen::Vector3d position;
};

// Integration test based on the original simulator entry point.
// Default: Pi SPI master -> STM32, mode 0, 500 kHz, 256 bytes/sample.
int runOldMain(int argc, char** argv) {
  std::string device = "/dev/spidev0.0";
  std::string capture;
  std::string pathFile = FLIGHT_SIM_DEFAULT_PATH;
  uint32_t samples = 1800;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help") {
      std::cout << "Usage: old_main_spi_test [--device /dev/spidev0.0] "
                   "[--samples 1800] [--path FILE] [--capture FILE]\n"
                   "60 Hz physics; immediate injection; LiDAR=1000 mm; RC=no-op.\n";
      return 0;
    }
    if (i + 1 >= argc) throw std::invalid_argument("Missing value for " + arg);
    const std::string value = argv[++i];
    if (arg == "--device") device = value;
    else if (arg == "--capture") {
      if (value.empty()) throw std::invalid_argument("Capture path is empty");
      capture = value;
    }
    else if (arg == "--path") pathFile = value;
    else if (arg == "--samples") {
      if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Samples must be a positive integer");
      const auto n = std::stoull(value);
      if (n == 0 || n > 1000000) throw std::invalid_argument("Samples must be 1..1000000");
      samples = static_cast<uint32_t>(n);
    } else throw std::invalid_argument("Unknown option " + arg);
  }
  auto transport = capture.empty() ? makeLinuxSpiTransport(device, 500000)
                                   : makeCaptureTransport(capture);
  std::cout << "Old-main simulation -> " << (capture.empty() ? device : capture)
            << "; IMMEDIATE injection, simulation-relative timestamps.\n";

  std::vector<Waypoint> path;

  std::vector<Eigen::Vector3d> rc_instructions;

  ImuSimulator sim;

  rc_instructions = rc_read(pathFile);
  for (Eigen::Vector3d a : rc_instructions) {
    std::cout << "vector: \n" << a << std::endl;
  }
  double time = 0.0;
  for (Eigen::Vector3d instruction : rc_instructions) {
    Waypoint temp = {time, instruction};
    path.push_back(temp);
    time += 0.5;
  }
  if (path.empty()) throw std::runtime_error("Flightpath has no waypoints");
  int currentWaypointIndex = 0;
  auto drone = std::make_unique<Drone>(
      new RigidBody(1.0, Eigen::Matrix3d::Identity(),
                    Eigen::Vector3d(0, 0, 0)), // body
      new RigidBody(0.5, Eigen::Matrix3d::Identity(),
                    Eigen::Vector3d(1, 0, 0)), // motors
      new RigidBody(0.5, Eigen::Matrix3d::Identity(),
                    Eigen::Vector3d(-1, 0, 0)),
      new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0, 1, 0)),
      new RigidBody(0.5, Eigen::Matrix3d::Identity(),
                    Eigen::Vector3d(0, -1, 0)));

  auto ground = std::make_unique<RigidBody>();
  ground->setBounds(
      50, 50,
      5); // Plane perpendicular to the z-axis with dimensions 100x100x10

  using clock = std::chrono::steady_clock;
  using duration = std::chrono::duration<double>;

  // Time is in seconds.
  const double DELTATIME = 1.0 / 60.0;
  uint32_t sent = 0;
  double elapsedTime = 0.0;
  auto previousTime = std::chrono::steady_clock::now();

  Eigen::Vector3d posGains(0.5, 0.5, 2.0);
  auto positionControl = std::make_unique<positionController>(posGains);
  Eigen::Vector3d vKp(1.0, 1.0, 3.0);
  Eigen::Vector3d vKi(0.1, 0.1, 0.5);
  Eigen::Vector3d vKd(0.05, 0.05, 0.2);
  auto velocityControl =
      std::make_unique<velocityController>(vKp, vKi, vKd, 20);
  positionControl->setTarget(path[0].position);

  // Current condition set to break when the drone collides with the ground
  //	while( !drone->isColliding(ground) ){ got rid of collisions for now
  initializeLogging();
  while (sent < samples) {
    auto currentTime = clock::now();
    duration dt = currentTime - previousTime;

    if (dt.count() >= DELTATIME) {
      // TODO: logic and the actual physics
      if (currentWaypointIndex < path.size() - 1) {
        if (elapsedTime >= path[currentWaypointIndex + 1].time) {
          currentWaypointIndex++;
          positionControl->setTarget(path[currentWaypointIndex].position);
          std::cout << "--- SWITCHING TO WAYPOINT" << currentWaypointIndex
                    << " ---" << std::endl
                    << std::endl;
        }
      }
      drone->applyForce(Eigen::Vector3d(0, 0, -9.81 * drone->getMass()));
      // Eigen::Vector3d force = control->compute(drone->getPosition(),
      // DELTATIME);
      Eigen::Vector3d targetVelocity =
          positionControl->compute(drone->getPosition(), DELTATIME);
      Eigen::Vector3d force = velocityControl->compute(
          drone->getVelocity(), targetVelocity, DELTATIME);
      force *= drone->getMass();
      force.z() += drone->getMass() * 9.81;
      drone->applyForce(force);
      // sim->update(force, )
      drone->update(DELTATIME);
      // Same simulation/controller loop, sampled through the current IMU API.
      // Net acceleration already includes gravity and uses the aggregate mass.
      imu_data_t imuData = sim.sample(drone->getOrientation(),
                                     drone->getAngularVelocity(),
                                     drone->getAcceleration());
      const uint32_t timestampUs = static_cast<uint32_t>(
          (uint64_t{sent + 1} * 1000000) / 60);
      const auto frame = encodeSensorFrame(imuData, timestampUs, 1000);
      transport->transmit(frame.data(), frame.size());
      ++sent;
      // Print decoded integer fields so the STM32 RX log can be compared.
      if (sent == 1 || sent % 60 == 0) {
        auto raw = [](i2c_imu_data_16_t v) -> int32_t {
          const uint32_t bits = static_cast<uint8_t>(v.lsb) |
                               (uint32_t{static_cast<uint8_t>(v.msb)} << 8);
          return bits >= 32768 ? static_cast<int32_t>(bits) - 65536 : bits;
        };
        std::cout << "TX #" << sent << " sim_us=" << timestampUs
                  << " euler=(" << raw(imuData.euler_angles.x) << ','
                  << raw(imuData.euler_angles.y) << ',' << raw(imuData.euler_angles.z)
                  << ") gyro=(" << raw(imuData.gyro.x) << ',' << raw(imuData.gyro.y)
                  << ',' << raw(imuData.gyro.z) << ") acc=("
                  << raw(imuData.linear_acceleration.x) << ','
                  << raw(imuData.linear_acceleration.y) << ','
                  << raw(imuData.linear_acceleration.z) << ")\n";
      }


      std::cout << "pos: (" << drone->getPosition().transpose() << ")";

      std::cout << "\tvel: (" << drone->getVelocity().transpose() << ")";

      std::cout << "\tori: [" << drone->orientation.coeffs().transpose() << "]";

      std::cout
          << "error: "
          << (positionControl->getTarget() - drone->getPosition()).transpose();

      std::cout << "\ttimestep: " << elapsedTime << std::endl;

      Eigen::Vector3d posError =
          positionControl->getTarget() - drone->getPosition();
      logData(elapsedTime, positionControl->getTarget(), drone->getPosition(),
              posError, force);
      // Leave a full period after transmission so there is no catch-up burst.
      previousTime = clock::now();
      elapsedTime = double(sent) / 60.0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  closeLogging();
  std::cout << "Completed " << sent << " frames. Check STM32 RX/applied counters "
               "to confirm delivery; SPI transfer success is not a decode acknowledgment.\n";

  return 0;
}

void initializeLogging() {
  logFile.exceptions(std::ios::failbit | std::ios::badbit);
  logFile.open("old_main_pid_tuning.csv");
  logFile << "Time, X, Y, Z, errorZ, \n";
}

void logData(double t, const Eigen::Vector3d &desiredPos,
             const Eigen::Vector3d &currentPos, const Eigen::Vector3d &error,
             const Eigen::Vector3d &controlOutput) {
  logFile << t << "," << currentPos.x() << "," << currentPos.y() << ","
          << currentPos.z() << "," << error.z() << "," << controlOutput.z()
          << "\n";
}
void closeLogging() { logFile.close(); }


int main(int argc, char** argv) {
  try {
    return runOldMain(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "old_main_spi_test: " << error.what() << '\n';
    return 1;
  }
}
