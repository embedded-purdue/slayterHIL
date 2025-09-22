#include "threads/example_producer.h"
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(example_q, EXAMPLE_QUEUE_PACKET_SIZE, EXAMPLE_QUEUE_LEN, 1);

static void example_producer_thread(void)
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

K_THREAD_DEFINE(example_producer_tid, EXAMPLE_PRODUCER_STACK_SIZE, example_producer_thread, NULL, NULL, NULL,
                EXAMPLE_PRODUCER_PRIORITY, 0, -1);


void example_producer_init() {
    // Fill in any initialization
    // If threads need to be initialized in a certain order, these functions can trigger the start of a statically defined thread.

    k_thread_start(example_producer_tid);
}