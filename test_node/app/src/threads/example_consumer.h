#ifndef THREADS_EXAMPLE_CONSUMER_H
#define THREADS_EXAMPLE_CONSUMER_H

// Consumes data coming from the producer thread via a message queue

#define EXAMPLE_CONSUMER_STACK_SIZE 1024
#define EXAMPLE_CONSUMER_PRIORITY 5

// Init function
void example_consumer_init();

#endif /* THREADS_EXAMPLE_CONSUMER_H */