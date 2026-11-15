/* 
#include "threads/gnss_emulator.h"
#include "threads/sensor_emulation_test.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device *uart_gnss = DEVICE_DT_GET(DT_ALIAS(uart_gnss));

int main(void)
{
    printk("starting up\n");
    if (!device_is_ready(uart_gnss))
    {
        LOG_ERR("GNSS UART not ready");
        return -ENODEV;
    }
    printk("device is ready\n");

    LOG_INF("GNSS UART ready: %s", uart_gnss->name);

    if (gnss_emulator_init(uart_gnss) < 0)
    {
        LOG_ERR("Failed to initialize GNSS emulator");
        return -ENODEV;
    }

    printk("successful init\n");

    gps_data_t gps_packet = {
        .time = "123519.00",
        .status = 'A',
        .latitude = 4807.03800,
        .lat_dir = 'N',
        .longitude = 1131.00000,
        .lon_dir = 'E',
        .speed = 12.0f,
        .course = 185.0f,
        .date = "230394",
        .variation = 0.0f,
        .var_dir = 'E',
        .mode = 'A',
        .nav_status = 'S'};

    LOG_INF("GNSS emulator initialized");

    while (1)
    {
        printk("sending packet\n");
        gps_update_location(&gps_packet);
        printk("packet sent\n");
        k_msleep(1000);
    }

    return 0;
}
 */