#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <zephyr/sys/ring_buffer.h> //for ring_buffer struct
#include <zephyr/drivers/sensor.h> //for sensor_value struct: https://docs.zephyrproject.org/apidoc/latest/sensor_8h_source.html

// types
typedef enum {
    INIT_HW, //set by a user command
    INIT_HW_DONE, //set by init_hw thread upon completion
    ASCEND, //set by a user command
    ASCEND_DONE, //set by ascend thread upon completion
    LAND, //set by a user command
    LAND_DONE, //set by land thread upon completion
    ERROR, //set by user command or any handler threads 
    CTRL_COMMAND, //set by user command
    CTRL_QUEUE_EMPTY //set by controlled flight thread upon command queue completion
} system_events_t;

typedef enum {
    DRONE_IDLE, //power on or after LAND_DONE event
    DRONE_START_INIT, //INIT_HW event from IDLE
    DRONE_DONE_INIT, //INIT_HW_DONE event from control thread
    DRONE_FIRST_ASCEND, //ASCEND user command sent after HW_DONE_INIT
    DRONE_HOVER, //ASCEND_DONE or CTRL_QUEUE_EMPTY event has finished
    DRONE_LAND, //LAND user command after HOVER
    DRONE_CONTROLLED_FLIGHT, //CTRL_COMMAND sent by user after HOVER
    DRONE_ERROR //certain failures force manual reset
} drone_state_t;

typedef enum {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    BACK,
    FORWARD,
    ABORT
} command_t;

typedef struct { //may need to be changed to match the zephyr mpu 6050 driver
    struct sensor_value accel[3]; //x,y,z acceleration
    struct sensor_value gyro[3]; //roll pitch and yaw 
} imu_state_t;

typedef struct { //may need to be changed to match alex's lidar lite driver
    uint16_t distance;
} lidar_state_t;

// function declarations
void state_machine_handler(system_events_t event);
void state_machine_thread(void *p1, void *p2, void *p3);

#endif