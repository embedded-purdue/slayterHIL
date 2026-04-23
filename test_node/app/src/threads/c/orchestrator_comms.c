#include "threads/orchestrator_comms.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// register logging module
LOG_MODULE_REGISTER(orchestrator_thread, LOG_LEVEL_INF);

K_MSGQ_DEFINE(orchestrator_receive_q, ORCHESTRATOR_RECEIVE_QUEUE_PACKET_SIZE, ORCHESTRATOR_RECEIVE_QUEUE_LEN, 1);
K_MSGQ_DEFINE(orchestrator_send_q, ORCHESTRATOR_SEND_QUEUE_PACKET_SIZE, ORCHESTRATOR_SEND_QUEUE_LEN, 1);

static void orchestrator_comms_thread(void *, void *, void *) {
    OrchestratorCommsPacket send_packet;

    while(1) {
        // dummy printing for sanity
        LOG_INF("Orchestrator hello world\n");

        // Put received messages in orchestrator_received_q   

        // Send all messages that appear in orchestrator_send_q
        k_msgq_get(&orchestrator_send_q, &send_packet, K_FOREVER);
    }
}

K_THREAD_STACK_DEFINE(orchestrator_comms_stack, ORCHESTRATOR_COMMS_STACK_SIZE);
struct k_thread orchestrator_comms_data;

// Initialize and start thread
void orchestrator_comms_init() {
    // dummy printing for sanity
    LOG_INF("Orchestrator init\n");

    // Any initialization

    k_thread_create(&orchestrator_comms_data, orchestrator_comms_stack, 
        K_THREAD_STACK_SIZEOF(orchestrator_comms_stack),
        orchestrator_comms_thread,
        NULL, NULL, NULL,
        ORCHESTRATOR_COMMS_PRIORITY, 0, K_NO_WAIT);
}
