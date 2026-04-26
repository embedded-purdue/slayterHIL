#ifndef THREADS_SCHEDULER_H
#define THREADS_SCHEDULER_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

#define SCHEDULER_STACK_SIZE (10 * 1024)
#define SCHEDULER_PRIORITY (5)

void scheduler_init(void);

#endif /* THREADS_SCHEDULER_H */
