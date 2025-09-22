#ifndef THREADS_EXAMPLE_PRODUCER_H
#define THREADS_EXAMPLE_PRODUCER_H

// Produces data in a message queue (example_q) to be consumed by another thread.

#define EXAMPLE_PRODUCER_STACK_SIZE 1024
#define EXAMPLE_PRODUCER_PRIORITY 5

#include <zephyr/kernel.h>

// Example queue
#define EXAMPLE_QUEUE_PACKET_SIZE (sizeof(uint32_t))
#define EXAMPLE_QUEUE_LEN (3)
extern struct k_msgq example_q;

// Init Function
void example_producer_init();

#endif /* THREADS_EXAMPLE_PRODUCER_H */