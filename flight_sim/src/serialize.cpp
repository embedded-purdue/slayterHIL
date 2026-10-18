#include "serialize.hpp"
#include "sensor_data.pb.h"
#include <stdexcept>

namespace {
int32_t unpack(const i2c_imu_data_16_t& value) {
    const uint32_t bits = static_cast<uint8_t>(value.lsb) |
                         (uint32_t{static_cast<uint8_t>(value.msb)} << 8);
    return bits >= 0x8000 ? static_cast<int32_t>(bits) - 0x10000
                          : static_cast<int32_t>(bits);
}
}

SensorFrame encodeSensorFrame(const imu_data_t& data, uint32_t timestampUs,
                              uint16_t lidarMm, int8_t rcVertical,
                              int8_t rcHorizontal) {
    SensorPacket packet;
    packet.set_timestamp_us(timestampUs);
    packet.set_injection_mode(IMMEDIATE);
    packet.set_lidar_mm(lidarMm);
    auto* imu = packet.mutable_imu();
    imu->set_euler_x(unpack(data.euler_angles.x));
    imu->set_euler_y(unpack(data.euler_angles.y));
    imu->set_euler_z(unpack(data.euler_angles.z));
    imu->set_lin_acc_x(unpack(data.linear_acceleration.x));
    imu->set_lin_acc_y(unpack(data.linear_acceleration.y));
    imu->set_lin_acc_z(unpack(data.linear_acceleration.z));
    imu->set_gyro_x(unpack(data.gyro.x));
    imu->set_gyro_y(unpack(data.gyro.y));
    imu->set_gyro_z(unpack(data.gyro.z));
    packet.mutable_rc()->set_vertical(rcVertical);
    packet.mutable_rc()->set_horizontal(rcHorizontal);

    SensorFrame frame{};
    const auto length = packet.ByteSizeLong();
    if (length == 0 || length > frame.size() - 2) {
        throw std::length_error("SensorPacket exceeds SPI payload capacity");
    }
    frame[0] = static_cast<uint8_t>(length & 0xff);
    frame[1] = static_cast<uint8_t>(length >> 8);
    if (!packet.SerializeToArray(frame.data() + 2, static_cast<int>(length))) {
        throw std::runtime_error("Failed to encode SensorPacket");
    }
    return frame;
}
