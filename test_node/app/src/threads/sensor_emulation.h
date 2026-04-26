#ifndef THREADS_SENSOR_EMULATION_H
#define THREADS_SENSOR_EMULATION_H

// Decodes protobuf messages from orchestrator. Queues actions based on timestamps and sends commands to the sensor emulation thread.

#include <stdint.h>

#define LIDAR_ADDRESS 0x62
#define IMU_ADDRESS 0x29

// types that represent the data format from the IMU sensor

/*
    i2c_imu_data_16_t represents each individual data value in the IMU. they are signed 16 bit values with consecutive registers representing LSB/MSB
    example: QUA_DATA_Y_LSB is at 0x24 and QUA_DATA_Y_MSB is at 0x25.
*/
typedef struct {
    int8_t lsb;
    int8_t msb;
} i2c_imu_data_16_t;

/* i2c_imu_triplet_t represents a triplet of data values (x, y, z) from the IMU sensor. */
typedef struct {
    i2c_imu_data_16_t x;
    i2c_imu_data_16_t y;
    i2c_imu_data_16_t z;
} i2c_imu_triplet_t;

/* imu_data_t represents the complete set of data from the IMU sensor. */
/* NOTE: the order of fields in this struct is VERY IMPORTANT */
typedef struct {
    i2c_imu_triplet_t euler_angles;
    i2c_imu_triplet_t linear_acceleration;
    i2c_imu_triplet_t gyro;
} imu_data_t;

/* if a no-op, insert I for no-op */
typedef struct {
    char rc_vert;
    char rc_horiz;
} rc_data_t;

/*
EXPECTED STRUCTURE FROM PROTOBUF

typedef struct  {
  uint32 timestamp;
  imu_data_t imu_data;
  uint16_t lidar_distance_nm;
  rc_data_t rc_commands;
} device_update_packet_t;
*/

// Init function
void sensor_emulation_init();

#endif /* THREADS_SENSOR_EMULATION_H */
