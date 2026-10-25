#include <zephyr/kernel.h>
#include <stddef.h>
#include <stdalign.h>
#include "state_machine.h"
#include "control.h"
#include "ledCtrl.h"

#define MESSAGES_PER_QUEUE 16
//These are arbitrary sizes for now, may need to be revisited
#define IMU_CAPACITY       32
#define LIDAR_CAPACITY     32
#define COMMAND_CAPACITY   16
//copied from main.c -> LED/motor mappings
#define TOP_LEFT  1
#define TOP_RIGHT  2
#define BOTTOM_LEFT  0
#define BOTTOM_RIGHT  3

// #define IMU_SIZE_BYTES     (IMU_CAPACITY * sizeof(imu_state_t))
// #define LIDAR_SIZE_BYTES   (LIDAR_CAPACITY * sizeof(lidar_state_t))
#define COMMAND_SIZE_BYTES (COMMAND_CAPACITY * sizeof(command_t))

//global state transition events
system_events_t event;
drone_state_t current_state = DRONE_IDLE;

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

// RING_BUF_DECLARE(imu_buffer, IMU_SIZE_BYTES);
// RING_BUF_DECLARE(lidar_buffer, LIDAR_SIZE_BYTES);
RING_BUF_DECLARE(command_queue, COMMAND_SIZE_BYTES);

void error(void) {
    //shutdown motors, abort flight
    set_led_intensity(TOP_RIGHT, 0);
    set_led_intensity(TOP_LEFT, 0);
    set_led_intensity(BOTTOM_LEFT, 0);
    set_led_intensity(BOTTOM_RIGHT, 0);
    //indefinitely loop in error until physical reset
    printk("Error state reached, flight aborted\n");
    while(1){
        k_msleep(1000);
    }
}

void init_hw(void) {
    // Initialize hardware components here, push an init_done msg at completion
    initialize_leds(); 
    printk("Leds initialized\n");
    control_init();
    printk("Control setpoints initialized\n");
    static const system_events_t hw_done = INIT_HW_DONE;
    k_msgq_put(&event_msgq, &hw_done, K_NO_WAIT);
}
//ascend and then push an ascend_done msg
void ascend(void) {
    uint32_t curr_alt;
    float target;
    bool curr_alt_valid = false;
    //make sure LiDAR still giving valid readings
    while (!curr_alt_valid) {
        control_get(NULL, NULL, NULL, &curr_alt, &curr_alt_valid);
        k_msleep(10);
    }

    if (current_state == DRONE_FIRST_ASCEND) {
        control_adjust_altitude(CONTROL_FIRST_ASCENSION_MM - (float)curr_alt);
    } else {
        control_adjust_altitude(CONTROL_ALTITUDE_STEP_MM);
    }

    do {
        control_get(NULL, NULL, &target, &curr_alt, &curr_alt_valid);
        k_msleep(10);
    } while (!curr_alt_valid || (float)curr_alt < target - 50.0f);

    static const system_events_t ascend_done_event = ASCEND_DONE;
    k_msgq_put(&event_msgq, &ascend_done_event, K_NO_WAIT);
}

void land(void) {
    uint32_t curr_alt;
    uint32_t error_timer = 0;
    bool curr_alt_valid;
    //level drone (zero attitude) and setpoint for 
    control_zero_attitude();
    control_adjust_altitude(-CONTROL_ALTITUDE_MAX_MM);

    do {
        control_get(NULL, NULL, NULL, &curr_alt, &curr_alt_valid);
        k_msleep(100);
        if (++error_timer > 300) {
            //goto error if land takes too long
            printk("Forcing ERROR after 30 second land timeout\n");
            static const system_events_t err_event = ERROR;
            k_msgq_put_front(&event_msgq, &err_event);
            return;
        }
    //repeat altitude readings until we are close enough to ground
    } while (!curr_alt_valid || curr_alt > (uint32_t)CONTROL_ALTITUDE_MIN_MM + 50);

    static const system_events_t land_done_event = LAND_DONE;
    k_msgq_put(&event_msgq, &land_done_event, K_NO_WAIT);
}

void hover(void) {
    // Implementation for hover functionality -> stay at current position
    control_zero_attitude();
}

void controlled_flight(void) {
    //handles all user commands from uart thread, returns to hover when done processing
    command_t cmd;
    while(ring_buf_get(&command_queue, (uint8_t *)&cmd, sizeof(command_t)) != 0){
    printk("Cmd in controlled flight: %d\n", (int)cmd);
    switch (cmd){
        case LEFT:
            control_adjust_roll(-CONTROL_ATTITUDE_STEP_DEG);
            break;
        case RIGHT:
            control_adjust_roll(+CONTROL_ATTITUDE_STEP_DEG);
            break;
        case FORWARD:
            control_adjust_pitch(+CONTROL_ATTITUDE_STEP_DEG);
            break;
        case BACK:
            control_adjust_pitch(-CONTROL_ATTITUDE_STEP_DEG);
            break;
        case UP:
            control_adjust_altitude(+CONTROL_ALTITUDE_STEP_MM);
            break;
        case DOWN:
            control_adjust_altitude(-CONTROL_ALTITUDE_STEP_MM);
            break;
        case ABORT:
            //on ABORT, attempt to land the drone
            static const system_events_t land_event = LAND;
            k_msgq_put(&event_msgq, &land_event, K_NO_WAIT);
            return;
        default:
            break;
    }

}
static const system_events_t queue_empty_event = CTRL_QUEUE_EMPTY;
k_msgq_put(&event_msgq, &queue_empty_event, K_NO_WAIT);
}
// define queue of system events
void state_machine_handler(system_events_t event) {
    switch(event) {
        case INIT_HW:
            if (current_state == DRONE_IDLE) {
                current_state = DRONE_START_INIT;
                init_hw();
            }
            break;
        case INIT_HW_DONE:
            if (current_state == DRONE_START_INIT) current_state = DRONE_DONE_INIT;
            break;
        case ASCEND: //start initial ascension after hardware is initialized
            if (current_state == DRONE_DONE_INIT){
                current_state = DRONE_FIRST_ASCEND;
                ascend();
            }
            break;
        case ASCEND_DONE: //finished initial ascension
            if (current_state == DRONE_FIRST_ASCEND){
            current_state = DRONE_HOVER;   
            hover();
            }
            break;
        case LAND: //begin landing from hovering state OR controlled flight on ABORT ('B') UART command
            if (current_state == DRONE_HOVER || current_state == DRONE_CONTROLLED_FLIGHT){
                current_state = DRONE_LAND;
                land();
            }
            break;
        case LAND_DONE: //return to DRONE_IDLE after landing
            if (current_state == DRONE_LAND){
                current_state = DRONE_IDLE;
            }
            break;
        case ERROR:
            current_state = DRONE_ERROR;
            error();
            break;
        case CTRL_COMMAND:
            if (current_state == DRONE_HOVER){
                current_state = DRONE_CONTROLLED_FLIGHT;
                controlled_flight();
            }
            break;
        case CTRL_QUEUE_EMPTY:
            if (current_state == DRONE_CONTROLLED_FLIGHT){
                current_state = DRONE_HOVER;   
            }
            break;
        default:
            break;
    }
}

void state_machine_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    while(1) {
        // Thread loop code here
        while (k_msgq_get(&event_msgq, &event, K_FOREVER) == 0) {
            printk("Event in event queue. Current State: %d\n", current_state);
            state_machine_handler(event);
            printk("Event processed. Current State: %d\n", current_state);
        }
    }
}
