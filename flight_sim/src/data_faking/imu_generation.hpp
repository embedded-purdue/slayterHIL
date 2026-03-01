#// imu_sim.h

#ifndef IMU_SIM_H
#define IMU_SIM_H

#define EULER_SCALE  16.0f
#define ACCEL_SCALE  100.0f
#define GYRO_SCALE   16.0f

#include <stdint.h>

// --- IMU hardware struct definitions ---
typedef struct {
    int8_t lsb;
    int8_t msb;
} i2c_imu_data_16_t;

typedef struct {
    i2c_imu_data_16_t x;
    i2c_imu_data_16_t y;
    i2c_imu_data_16_t z;
} i2c_imu_triplet_t;

typedef struct {
    i2c_imu_triplet_t euler_angles;
    i2c_imu_triplet_t linear_acceleration;
    i2c_imu_triplet_t gyro;
} imu_data_t;

class ImuSimulator {
public:
    ImuSimulator(float mass, float moment_of_inertia);
    imu_data_t update(float fx, float fy, float fz,
                      float tx, float ty, float tz,
                      float dt);

private:
    float m_;
    float I_;

    float wx_, wy_, wz_;
    float roll_, pitch_, yaw_;

    i2c_imu_data_16_t pack_16(int16_t value);
};


#endif
