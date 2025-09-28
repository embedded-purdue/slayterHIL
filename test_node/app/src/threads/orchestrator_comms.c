#include "threads/orchestrator_comms.h"
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(orchestrator_receive_q, ORCHESTRATOR_RECEIVE_QUEUE_PACKET_SIZE, ORCHESTRATOR_RECEIVE_QUEUE_LEN, 1);
K_MSGQ_DEFINE(orchestrator_send_q, ORCHESTRATOR_SEND_QUEUE_PACKET_SIZE, ORCHESTRATOR_SEND_QUEUE_LEN, 1);

static void orchestrator_comms_thread(void) {
    while(1) {
        // Put received messages in orchestrator_received_q   
        // Send all messages that appear in orchestrator_send_q
    }
}

K_THREAD_DEFINE(orchestrator_comms_tid, ORCHESTRATOR_COMMS_STACK_SIZE, orchestrator_comms_thread, NULL, NULL, NULL,
                ORCHESTRATOR_COMMS_PRIORITY, 0, -1); // Delay of -1 so thread does not start automatically

// Initialize and start thread
void orchestrator_comms_init() {
    // Any initialization

    k_thread_start(orchestrator_comms_tid); // Start thread
}