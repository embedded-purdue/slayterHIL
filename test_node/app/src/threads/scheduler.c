#include "threads/scheduler.h"
#include "threads/orchestrator_comms.h" // for orchestrator_received_q
#include "threads/sensor_emulation.h" // for sensor data update queue
// #include <sensor_data.pb.h> // protobuf struct definition header
#include <pb_decode.h> // protobuf decode header
#include <zephyr/kernel.h>

static void scheduler_thread(void *, void *, void *) {
    OrchestratorCommsPacket packet;

    while(1) {
        // Get data from orchestrator_receive_q
        k_msgq_get(&orchestrator_receive_q, &packet, K_FOREVER);

        // Decode using protobuf
        // Schedule sensor data updates (using a heap?)
        // At appropriate times, put sensor data updates into sensor data update queue (may need interrupts)
    }
}

K_THREAD_STACK_DEFINE(scheduler_stack, SCHEDULER_STACK_SIZE);
struct k_thread scheduler_data;

// Initialize and start thread
void scheduler_init() {
    // Any initialization

    k_thread_create(&scheduler_data, scheduler_stack, 
        K_THREAD_STACK_SIZEOF(scheduler_stack),
        scheduler_thread,
        NULL, NULL, NULL,
        SCHEDULER_PRIORITY, 0, K_NO_WAIT);
}
