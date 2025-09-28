#ifndef THREADS_ORCHESTRATOR_COMMS_H
#define THREADS_ORCHESTRATOR_COMMS_H

// Handles SPI communication with orchestrator. Write to received queue, receives on send queue.
// Priorities, stack sizes, queue packet sizes and lengths can all be changed.

#include <stdint.h>

#define ORCHESTRATOR_COMMS_STACK_SIZE (10 * 1024)
#define ORCHESTRATOR_COMMS_PRIORITY 5

typedef struct _OrchestratorCommsPacket {
    uint8_t len;
    uint8_t data[256];
} OrchestratorCommsPacket;

#define ORCHESTRATOR_RECEIVE_QUEUE_PACKET_SIZE (sizeof(OrchestratorCommsPacket))
#define ORCHESTRATOR_RECEIVE_QUEUE_LEN (10)
extern struct k_msgq orchestrator_receive_q;

#define ORCHESTRATOR_SEND_QUEUE_PACKET_SIZE (sizeof(OrchestratorCommsPacket))
#define ORCHESTRATOR_SEND_QUEUE_LEN (10)
extern struct k_msgq orchestrator_send_q;

// Init function
void orchestrator_comms_init();

#endif /* THREADS_ORCHESTRATOR_COMMS_H */