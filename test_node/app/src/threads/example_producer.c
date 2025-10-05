#include "threads/example_producer.h"
#include <zephyr/kernel.h>

// Initialize example_q message queue. Exposed in example_producer.h using extern.
K_MSGQ_DEFINE(example_q, EXAMPLE_QUEUE_PACKET_SIZE, EXAMPLE_QUEUE_LEN, 1);

static void example_producer_thread(void *, void *, void *)
{
    uint32_t send_data = 0;

    while (1) {
        k_msgq_put(&example_q, &send_data, K_FOREVER); // Sleep until there is space available in the queue to fill.

        if (!(send_data % 10000)) {
            printk("Produced %d\n", send_data); // Print every 10000 data to prove something is happening.
        }
        send_data++;
    }
}

K_THREAD_STACK_DEFINE(example_producer_stack, EXAMPLE_PRODUCER_STACK_SIZE);
struct k_thread example_producer_data;

// Initialize and start thread
void example_producer_init() {
    // Any initialization

    k_thread_create(&example_producer_data, example_producer_stack, 
        K_THREAD_STACK_SIZEOF(example_producer_stack),
        example_producer_thread,
        NULL, NULL, NULL,
        EXAMPLE_PRODUCER_PRIORITY, 0, K_NO_WAIT);
}