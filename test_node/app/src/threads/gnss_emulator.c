
/**
 * @file gnss_emulator.c
 * @brief GNSS emulator: sends NMEA RMC messages over UART.
 */

#include "threads/gnss_emulator.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

LOG_MODULE_REGISTER(gnss_emulator, LOG_LEVEL_INF);

static const struct device *gnss_uart;

K_THREAD_STACK_DEFINE(gnss_stack, GNSS_EMULATOR_STACK_SIZE);
static struct k_thread gnss_thread_data;

void gps_update_location(const gps_data_t *packet)
{
    if (packet == NULL || gnss_uart == NULL)
    {
        return;
    }

    for (int p = 0; p < 2; p++)
    {
        char gps_string[160];

        double speed = (double)packet->speed + (p * 0.1);
        double course = (double)packet->course + (p * 0.5);

        if (course >= 360.0)
        {
            course -= 360.0;
        }

        int len = snprintf(gps_string, sizeof(gps_string),
                           "$GPRMC,%.10s,%c,%010.5f,%c,%011.5f,%c,%.1f,%.1f,%.6s,%.2f,%c,%c,%c",
                           packet->time,
                           packet->status,
                           packet->latitude,
                           packet->lat_dir,
                           packet->longitude,
                           packet->lon_dir,
                           speed,
                           course,
                           packet->date,
                           (double)packet->variation,
                           packet->var_dir,
                           packet->mode,
                           packet->nav_status);

        if (len < 0 || (size_t)len >= sizeof(gps_string))
        {
            LOG_ERR("Failed to format GPS message");
            return;
        }

        uint8_t checksum = 0;

        for (int i = 1; i < len; i++)
        {
            checksum ^= (uint8_t)gps_string[i];
        }

        int added = snprintf(gps_string + len, sizeof(gps_string) - (size_t)len, "*%02X\r\n", checksum);

        if (added < 0 || (size_t)added >= sizeof(gps_string) - (size_t)len)
        {
            LOG_ERR("Failed to append GPS checksum");
            return;
        }

        len += added;

        for (int i = 0; i < len; i++)
        {
            uart_poll_out(gnss_uart, (unsigned char)gps_string[i]);
        }
    }
}

static void gnss_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1)
    {
        k_msleep(GNSS_SEND_PERIOD_MS);
    }
}

int gnss_emulator_init(const struct device *uart_dev)
{
    if (uart_dev == NULL || !device_is_ready(uart_dev))
    {
        LOG_ERR("GNSS UART not ready");
        return -ENODEV;
    }

    gnss_uart = uart_dev;

    k_tid_t tid = k_thread_create(
        &gnss_thread_data,
        gnss_stack,
        K_THREAD_STACK_SIZEOF(gnss_stack),
        gnss_thread,
        NULL, NULL, NULL,
        GNSS_EMULATOR_PRIORITY,
        0, K_NO_WAIT);

    k_thread_name_set(tid, "gnss_emu");

    LOG_INF("GNSS emulator started on %s", uart_dev->name);

    return 0;
}
