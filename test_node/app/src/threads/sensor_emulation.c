#include "threads/sensor_emulation.h"
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(sensor_update_q, SENSOR_UPDATE_QUEUE_PACKET_SIZE, SENSOR_UPDATE_QUEUE_LEN, 1);
K_MSGQ_DEFINE(sensor_bus_q, SENSOR_BUS_QUEUE_PACKET_SIZE, SENSOR_BUS_QUEUE_LEN, 1);

static void sensor_emulation_thread(void *, void *, void *) {
    while(1) {
        // Get data from sensor_update_q or sensor_bus_q
        // Update sensor data or respond to bus message
        // Put communication peripheral commands to dut_interface thread
    }
}

K_THREAD_STACK_DEFINE(sensor_emulation_stack, SENSOR_EMULATION_STACK_SIZE);
struct k_thread sensor_emulation_data;

// Initialize and start thread
void sensor_emulation_init() {
    // Any initialization

    k_thread_create(&sensor_emulation_data, sensor_emulation_stack, 
        K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
        sensor_emulation_thread,
        NULL, NULL, NULL,
        SENSOR_EMULATION_PRIORITY, 0, K_NO_WAIT);
}