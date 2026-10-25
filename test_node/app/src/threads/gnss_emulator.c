/**
 * @file gnss_emulator.c
 * @brief GNSS emulator: sends a test message to the flight controller over UART once a second.
 *
 * This only proves the link works. Real GPS data comes later.
 * Baud rate (9600) and pins are set in the board overlay.
 */

#include "threads/gnss_emulator.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gnss_emulator, LOG_LEVEL_INF);

/* Placeholder until we know what the real chip sends */
static const char test_msg[] = "GNSS emulator test\r\n";

static const struct device *gnss_uart;

K_THREAD_STACK_DEFINE(gnss_stack, GNSS_EMULATOR_STACK_SIZE);
static struct k_thread gnss_thread_data;

/* Send len bytes, one at a time */
static void gnss_send(const char *msg, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(gnss_uart, (unsigned char)msg[i]);
    }
}

static void gnss_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        gnss_send(test_msg, sizeof(test_msg) - 1);  /* -1 skips the hidden '\0' */
        k_msleep(GNSS_SEND_PERIOD_MS);
    }
}

int gnss_emulator_init(const struct device *uart_dev)
{
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("GNSS UART not ready");
        return -ENODEV;
    }
    gnss_uart = uart_dev;

    k_thread_create(&gnss_thread_data, gnss_stack,
                    K_THREAD_STACK_SIZEOF(gnss_stack),
                    gnss_thread, NULL, NULL, NULL,
                    GNSS_EMULATOR_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&gnss_thread_data, "gnss_emu");

    LOG_INF("GNSS emulator started on %s", uart_dev->name);
    return 0;
}
