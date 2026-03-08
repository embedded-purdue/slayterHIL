#include "imu_generation.hpp"

ImuSimulator::ImuSimulator () :
  m_(0.0), I_(0.0)
{}

ImuSimulator::ImuSimulator (float m, float moi) :
  m_(m), I_(moi)
{}

i2c_imu_data_16_t ImuSimulator::pack_16(int16_t value) {
    i2c_imu_data_16_t result;
    result.lsb = (int8_t)(value & 0xFF);
    result.msb = (int8_t)((value >> 8) & 0xFF);
    return result;
}

imu_data_t ImuSimulator::update (float fx, float fy, float fz,
                                 float tx, float ty, float tz,
                                 float dt) 
{
  float accel_x = fx / m_;
  float accel_y = fy / m_;
  float accel_z = fz / m_;

  float angular_x = wx_ + (tx / I_) * dt;
  float angular_y = wy_ + (ty / I_) * dt;
  float angular_z = wz_ + (tz / I_) * dt;

  float roll = roll_ + angular_x * dt;
  float pitch = pitch_ + angular_y * dt;
  float yaw = yaw_ + angular_z * dt;

  wx_ = angular_x; wy_ = angular_y; wz_ = angular_x;
  roll_ = roll; pitch_ = pitch; yaw_ = yaw;

  imu_data_t data;

  
  data.euler_angles.x = pack_16((int16_t)(roll  * EULER_SCALE));
  data.euler_angles.y = pack_16((int16_t)(pitch * EULER_SCALE));
  data.euler_angles.z = pack_16((int16_t)(yaw   * EULER_SCALE));

  data.linear_acceleration.x = pack_16((int16_t)(accel_x * ACCEL_SCALE));
  data.linear_acceleration.y = pack_16((int16_t)(accel_y * ACCEL_SCALE));
  data.linear_acceleration.z = pack_16((int16_t)(accel_z * ACCEL_SCALE));

  data.gyro.x = pack_16((int16_t)(angular_x * GYRO_SCALE));
  data.gyro.y = pack_16((int16_t)(angular_y * GYRO_SCALE));
  data.gyro.z = pack_16((int16_t)(angular_z * GYRO_SCALE));

  return data;

}



