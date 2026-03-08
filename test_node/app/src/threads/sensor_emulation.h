#ifndef THREADS_SENSOR_EMULATION_H
#define THREADS_SENSOR_EMULATION_H

// Decodes protobuf messages from orchestrator. Queues actions based on timestamps and sends commands to the sensor emulation thread.

#include <stdint.h>

#define SENSOR_EMULATION_STACK_SIZE (10 * 1024)
#define SENSOR_EMULATION_PRIORITY 5
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

// assert size matches expected size of 24 bytes
_Static_assert(sizeof(imu_data_t) == 18, "imu_data_t size does not match expected size of 26 bytes");

typedef struct {
    uint8_t sensor_id;
    union {
        imu_data_t imu_data;
<<<<<<< HEAD
        uint16_t lidar_distance_mm;
        char rc_command;     
=======
        uint16_t lidar_distance_cm;
        char rc_command[MAX_RC_COMMAND_SIZE];
>>>>>>> Flightsim/FS/protobuf
    };
} device_update_packet_t;

#define SENSOR_UPDATE_QUEUE_PACKET_SIZE (sizeof(device_update_packet_t))
#define SENSOR_UPDATE_QUEUE_LEN (10)
extern struct k_msgq sensor_update_q;

// Init function
void sensor_emulation_init();

#endif /* THREADS_SENSOR_EMULATION_H */
