#include "threads/example_consumer.h"
#include "threads/example_producer.h" // Included for example_q message queue
#include <zephyr/kernel.h>

static void example_consumer_thread(void) {
    uint32_t receive_data;

    while(1) {
        k_msgq_get(&example_q, &receive_data, K_FOREVER); // Sleep until something appears in the queue

        if (!(receive_data % 10000)) {
            printk("Consumed %d\n", receive_data); // Print every 10000 data to prove something is happening
        }
    }
}

// Statically allocate thread
K_THREAD_DEFINE(example_consumer_tid, EXAMPLE_CONSUMER_STACK_SIZE, example_consumer_thread, NULL, NULL, NULL,
                EXAMPLE_CONSUMER_PRIORITY, 0, -1); // Delay of -1 so thread does not start automatically

// Initialize and start thread
void example_consumer_init() {
    // Fill in any initialization
    // If threads need to be initialized in a certain order, these functions can trigger the start of a statically defined thread.

    k_thread_start(example_consumer_tid);
}