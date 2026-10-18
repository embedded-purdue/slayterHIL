#include "serialize.hpp"
#include "sensor_data.pb.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

i2c_imu_data_16_t pack(int16_t value) {
    const auto bits = static_cast<uint16_t>(value);
    return {static_cast<int8_t>(bits & 0xff), static_cast<int8_t>(bits >> 8)};
}

int main() {
    imu_data_t data{};
    data.euler_angles = {pack(-32768), pack(32767), pack(-1)};
    data.linear_acceleration = {pack(128), pack(-129), pack(100)};
    data.gyro = {pack(-16), pack(16), pack(0)};
    const auto frame = encodeSensorFrame(data, 0xffffffffU, 1234, -127, 127);
    const size_t length = frame[0] | (size_t{frame[1]} << 8);
    require(frame.size() == 256 && length > 0 && length <= 254, "Invalid framing");
    require(std::all_of(frame.begin() + 2 + length, frame.end(),
                        [](uint8_t byte) { return byte == 0; }), "Nonzero padding");
    SensorPacket packet;
    require(packet.ParseFromArray(frame.data() + 2, static_cast<int>(length)),
            "Cannot decode payload");
    require(packet.timestamp_us() == 0xffffffffU && packet.lidar_mm() == 1234,
            "Metadata mismatch");
    require(packet.injection_mode() == IMMEDIATE, "Injection mode mismatch");
    require(packet.rc().vertical() == -127 && packet.rc().horizontal() == 127,
            "RC mismatch");
    const auto& imu = packet.imu();
    require(imu.euler_x() == -32768 && imu.euler_y() == 32767 && imu.euler_z() == -1,
            "Euler sign or field mismatch");
    require(imu.lin_acc_x() == 128 && imu.lin_acc_y() == -129 && imu.lin_acc_z() == 100,
            "Acceleration sign or field mismatch");
    require(imu.gyro_x() == -16 && imu.gyro_y() == 16 && imu.gyro_z() == 0,
            "Gyro sign or field mismatch");

    // Independent protobuf wire fixture: timestamp=1; gyro_x=-1 uses ZigZag=1;
    // Empty RC follows the IMU; field 5 specifies IMMEDIATE (1).
    data = {};
    data.gyro.x = pack(-1);
    const auto small = encodeSensorFrame(data, 1);
    const uint8_t expected[] = {10, 0, 0x08, 1, 0x1a, 2, 0x08, 1, 0x22, 0, 0x28, 1};
    require(std::equal(std::begin(expected), std::end(expected), small.begin()),
            "Wire bytes differ from receiver schema");
    const auto zero = encodeSensorFrame({}, 0);
    require(packet.ParseFromArray(zero.data() + 2, zero[0] | (zero[1] << 8)),
            "Zero sample cannot decode");
    require(packet.has_imu() && packet.has_rc() && packet.imu().gyro_x() == 0,
            "Zero sample contains stale values");
}
