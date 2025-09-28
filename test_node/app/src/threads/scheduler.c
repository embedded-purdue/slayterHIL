#include "threads/scheduler.h"
#include "threads/orchestrator_comms.h" // for orchestrator_received_q
#include "threads/sensor_emulation.h" // for sensor data update queue
// #include <sensor.pb.h> // protobuf struct definition header
// #include <pb_decode.h> // protobuf decode header
#include <zephyr/kernel.h>

static void scheduler_thread(void) {
    while(1) {
        // Get data from orchestrator_received_q
        // Decode using protobuf
        // Schedule sensor data updates (using heap?)
        // At appropriate times, put sensor data updates into sensor data update queue
    }
}

K_THREAD_DEFINE(scheduler_tid, SCHEDULER_STACK_SIZE, scheduler_thread, NULL, NULL, NULL,
                SCHEDULER_PRIORITY, 0, -1); // Delay of -1 so thread does not start automatically

// Initialize and start thread
void scheduler_init() {
    // Any initialization

    k_thread_start(scheduler_tid); // Start thread
}