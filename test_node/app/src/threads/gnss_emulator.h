/**
 * @file gnss_emulator.h
 * @brief GNSS emulator: UART link to the flight controller (comms only, no GPS data yet).
 */

#ifndef THREADS_GNSS_EMULATOR_H
#define THREADS_GNSS_EMULATOR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define GNSS_SEND_PERIOD_MS       1000
#define GNSS_EMULATOR_STACK_SIZE  1024
#define GNSS_EMULATOR_PRIORITY    8     /* below scheduler (5) and sensor emulation (7) */

int gnss_emulator_init(const struct device *uart_dev);

#endif /* THREADS_GNSS_EMULATOR_H */
