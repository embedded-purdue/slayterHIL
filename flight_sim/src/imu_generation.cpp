#include "imu_generation.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace {
i2c_imu_data_16_t pack(double value) {
    if (!std::isfinite(value)) throw std::invalid_argument("Non-finite IMU value");
    // Clamp before rounding/casting, so overflow cannot wrap a sensor reading.
    const auto raw = static_cast<int16_t>(std::lround(std::clamp(value, -32768.0, 32767.0)));
    const auto bits = static_cast<uint16_t>(raw);
    return {static_cast<int8_t>(bits & 0xff), static_cast<int8_t>(bits >> 8)};
}
}

imu_data_t ImuSimulator::sample(const Eigen::Quaterniond& bodyToWorld,
                               const Eigen::Vector3d& angularVelocityBody,
                               const Eigen::Vector3d& linearAccelerationWorld) const {
    if (!bodyToWorld.coeffs().allFinite() || bodyToWorld.norm() < 1e-12 ||
        !std::isfinite(bodyToWorld.norm()) || !angularVelocityBody.allFinite() ||
        !linearAccelerationWorld.allFinite()) {
        throw std::invalid_argument("Invalid rigid-body state for IMU");
    }
    const auto q = bodyToWorld.normalized();
    // ZYX yaw/pitch/roll convention: q = Rz(yaw) * Ry(pitch) * Rx(roll).
    const double roll = std::atan2(2 * (q.w()*q.x() + q.y()*q.z()),
                                  1 - 2 * (q.x()*q.x() + q.y()*q.y()));
    const double pitch = std::asin(std::clamp(2 * (q.w()*q.y() - q.z()*q.x()), -1.0, 1.0));
    double heading = std::atan2(2 * (q.w()*q.z() + q.x()*q.y()),
                               1 - 2 * (q.y()*q.y() + q.z()*q.z()));
    if (heading < 0) heading += 2 * std::numbers::pi;
    constexpr double degreesPerRadian = 180.0 / std::numbers::pi;
    const Eigen::Vector3d accelBody = q.conjugate() * linearAccelerationWorld;
    const Eigen::Vector3d gyro = angularVelocityBody * degreesPerRadian * GYRO_SCALE;
    imu_data_t result{};
    // BNO055 register order is heading, roll, pitch (not roll, pitch, yaw).
    result.euler_angles = {pack(heading * degreesPerRadian * EULER_SCALE),
                           pack(roll * degreesPerRadian * EULER_SCALE),
                           pack(pitch * degreesPerRadian * EULER_SCALE)};
    result.linear_acceleration = {pack(accelBody.x() * ACCEL_SCALE),
                                  pack(accelBody.y() * ACCEL_SCALE),
                                  pack(accelBody.z() * ACCEL_SCALE)};
    result.gyro = {pack(gyro.x()), pack(gyro.y()), pack(gyro.z())};
    return result;
}
