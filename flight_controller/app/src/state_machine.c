#include <zephyr/kernel.h>
#include <stddef.h>
#include <stdalign.h>
#include "state_machine.h"

#define MESSAGES_PER_QUEUE 16
//These are arbitrary sizes for now, may need to be revisited
#define IMU_CAPACITY       32
#define LIDAR_CAPACITY     32
#define COMMAND_CAPACITY   16

#define IMU_SIZE_BYTES     (IMU_CAPACITY * sizeof(imu_state_t))
#define LIDAR_SIZE_BYTES   (LIDAR_CAPACITY * sizeof(lidar_state_t))
#define COMMAND_SIZE_BYTES (COMMAND_CAPACITY * sizeof(command_t))

//global state transition events
system_events_t event;

// define message queue

K_MSGQ_DEFINE(event_msgq, sizeof(system_events_t), MESSAGES_PER_QUEUE, alignof(system_events_t));

//ring buffer for imu, lidar, and user commands
/*Note: zephyr ring buffers are default "byte mode"
which means that when you call something like
ring_buf_put(&imu_buffer, (uint8_t *)&imu_state_t, sizeof(imu_state_t))
it is copying sizeof(imu_state_t) bytes starting from the second arg mem addr
and then the same in reverse when you call 
ring_buf_get(&imu_buffer, (uint8_t *)&imu_val, sizeof(imu_state_t))
*/
RING_BUF_DECLARE(imu_buffer, IMU_SIZE_BYTES);
RING_BUF_DECLARE(lidar_buffer, LIDAR_SIZE_BYTES);
RING_BUF_DECLARE(command_queue, COMMAND_SIZE_BYTES);

void error(void) {
    //shutdown motors and abort flight
}

void init_hw(void) {
    // Initialize hardware components here, push an init_done msg at completion
}

void ascend(void) {
    // ascend and then push an ascend_done msg
}

void land(void) {
    //land and then push a land_done msg
}

void hover(void) {
    // Implementation for hover functionality
}

void controlled_flight(void) {
    //handles all user commands and then 
}

// define queue of system events
void state_machine_handler(system_events_t event) {
    switch(event) {
        case INIT_HW:
            init_hw();
            break;
        case ASCEND:
            ascend();
            break;
        case ASCEND_DONE:
            hover();
            break;
        case LAND:
            land();
            break;
        case LAND_DONE:
            break;
        case ERROR:
            error();
            break;
        case CTRL_COMMAND:
            controlled_flight();
            break;
        case CTRL_QUEUE_EMPTY:
            hover();
            break;
        default:
            break;
    }
}

void state_machine_thread(void *p1, void *p2, void *p3) {
    for(;;) {
        // Thread loop code here
        while (k_msgq_get(&event_msgq, &event, K_FOREVER) == 0) {
            state_machine_handler(event);
        }
    }
}
