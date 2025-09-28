#ifndef THREADS_SCHEDULER_H
#define THREADS_SCHEDULER_H

// Decodes protobuf messages from orchestrator. Queues actions based on timestamps and sends commands to the sensor emulation thread.

#define SCHEDULER_STACK_SIZE (10 * 1024)
#define SCHEDULER_PRIORITY 5

// Init function
void scheduler_init();

#endif /* THREADS_SCHEDULER_H */