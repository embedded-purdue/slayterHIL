#include "threads/sensor_emulation.h"
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(sensor_update_q, SENSOR_UPDATE_QUEUE_PACKET_SIZE, SENSOR_UPDATE_QUEUE_LEN, 1);
K_MSGQ_DEFINE(sensor_bus_q, SENSOR_BUS_QUEUE_PACKET_SIZE, SENSOR_BUS_QUEUE_LEN, 1);

static void sensor_emulation_thread(void) {
    while(1) {
        // Get data from sensor_update_q or sensor_bus_q
        // Update sensor data or respond to bus message
        // Put communication peripheral commands to dut_interface thread
    }
}

K_THREAD_DEFINE(sensor_emulation_tid, SENSOR_EMULATION_STACK_SIZE, sensor_emulation_thread, NULL, NULL, NULL,
                SENSOR_EMULATION_PRIORITY, 0, -1); // Delay of -1 so thread does not start automatically

// Initialize and start thread
void sensor_emulation_init() {
    // Any initialization

    k_thread_start(sensor_emulation_tid); // Start thread
}