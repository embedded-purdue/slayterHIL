#include "threads/example_consumer.h"
#include "threads/example_producer.h" // Included for example_q message queue
#include <zephyr/kernel.h>

static void example_consumer_thread(void *, void *, void *) {
    uint32_t receive_data;

    while(1) {
        k_msgq_get(&example_q, &receive_data, K_FOREVER); // Sleep until something appears in the queue

        if (!(receive_data % 10000)) {
            printk("Consumed %d\n", receive_data); // Print every 10000 data to prove something is happening
        }
    }
}

K_THREAD_STACK_DEFINE(example_consumer_stack, EXAMPLE_CONSUMER_STACK_SIZE);
struct k_thread example_consumer_data;

// Initialize and start thread
void example_consumer_init() {
    // Any initialization

    k_thread_create(&example_consumer_data, example_consumer_stack, 
        K_THREAD_STACK_SIZEOF(example_consumer_stack),
        example_consumer_thread,
        NULL, NULL, NULL,
        EXAMPLE_CONSUMER_PRIORITY, 0, K_NO_WAIT);
}