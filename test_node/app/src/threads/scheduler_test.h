#pragma once
#include <zephyr/kernel.h>

// Synchronous SPI receive in a dedicated thread; no shared-clock dependency.
#define SCHEDULER_STACK_SIZE (8192U)
#define SCHEDULER_PRIORITY (5)
void scheduler_init(void);
