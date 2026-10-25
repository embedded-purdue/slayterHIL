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
#include <stdio.h>

LOG_MODULE_REGISTER(gnss_emulator, LOG_LEVEL_INF);

/* Placeholder until we know what the real chip sends */
static const char test_msg[] = "GNSS emulator test\r\n";

static const struct device *gnss_uart;

K_THREAD_STACK_DEFINE(gnss_stack, GNSS_EMULATOR_STACK_SIZE);
static struct k_thread gnss_thread_data;

/* Send len bytes, one at a time */
void gps_update_location(const gps_data_t * packet)
{
    char gps_string[160];
    int len = snprintf(gps_string, sizeof(gps_string),
        "$GPGGA,%s,%010.5f,%c,%011.5f,%c,%d,%d,%.2f,%.3f,%c,,,,",
        packet->time,
        packet->latitude,
        packet->lat_dir,
        packet->longitude,
        packet->lon_dir,
        /* If valid is nonzero, use fix_quality; otherwise send 0 (no GPS fix).
         * GGA reports validity through fix quality, not a separate field. */
        packet->valid ? packet->fix_quality : 0,
        packet->satellites,
        packet->hdop,
        packet->altitude,
        packet->alt_units);

    if (len < 0 || (size_t)len >= sizeof(gps_string)){
        return;
    }

    unsigned int checksum = 0;
    for (int i = 1; i < len; i++) {
        checksum ^= (unsigned char)gps_string[i];
    }

    int added = snprintf(gps_string + len,
                         sizeof(gps_string) - (size_t)len,
                         "*%02X\r\n", checksum);
    if (added < 0 ||
        (size_t)added >= sizeof(gps_string) - (size_t)len) {
        return;
    }
    len += added;

    for (size_t i = 0; i < (size_t)len; i++) {
        uart_poll_out(gnss_uart, (unsigned char)gps_string[i]);
    }
}

static void gnss_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        // gnss_send(test_msg, sizeof(test_msg) - 1);  /* -1 skips the hidden '\0' */
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
