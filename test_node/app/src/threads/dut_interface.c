#include "threads/dut_interface.h"
#include "threads/sensor_emulation.h" // for sensor_bus_q
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(dut_interface_command_q, DUT_INTERFACE_COMMAND_QUEUE_PACKET_SIZE, DUT_INTERFACE_COMMAND_QUEUE_LEN, 1);

static void dut_interface_thread(void) {
    while(1) {
        // Get data from sensor_update_q or sensor_bus_q
        // Update sensor data or respond to bus message
        // Put communication peripheral commands to dut_interface thread
    }
}

K_THREAD_DEFINE(dut_interface_tid, DUT_INTERFACE_STACK_SIZE, dut_interface_thread, NULL, NULL, NULL,
                DUT_INTERFACE_PRIORITY, 0, -1); // Delay of -1 so thread does not start automatically

// Initialize and start thread
void dut_interface_init() {
    // Any initialization

    k_thread_start(dut_interface_tid); // Start thread
}