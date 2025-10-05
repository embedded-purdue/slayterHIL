#include "threads/dut_interface.h"
#include "threads/sensor_emulation.h" // for sensor_bus_q
#include "threads/orchestrator_comms.h" // for orchestrator_send_q
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(dut_interface_command_q, DUT_INTERFACE_COMMAND_QUEUE_PACKET_SIZE, DUT_INTERFACE_COMMAND_QUEUE_LEN, 1);

// Create interrupts for interfaces. When received data, put it in sensor_bus_q or orchestrator_send_q

static void dut_interface_thread(void *, void *, void *) {
    DutInterfaceCommandPacket command;

    while(1) {
        // Get commands from dut_interface_command_q
        k_msgq_get(&dut_interface_command_q, &command, K_FOREVER);

        // Output on peripherals - ideally non-blocking
        if (command.bus == DUT_INTERFACE_BUS_I2C0) {
            
        }
    }
}

K_THREAD_STACK_DEFINE(dut_interface_stack, DUT_INTERFACE_STACK_SIZE);
struct k_thread dut_interface_data;

// Initialize and start thread
void dut_interface_init() {
    // Any initialization

    k_thread_create(&dut_interface_data, dut_interface_stack, 
        K_THREAD_STACK_SIZEOF(dut_interface_stack),
        dut_interface_thread,
        NULL, NULL, NULL,
        DUT_INTERFACE_PRIORITY, 0, K_NO_WAIT);
}