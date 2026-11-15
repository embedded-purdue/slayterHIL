#define GPS_SEND 1

#if GPS_SEND == 1
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

#else
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#define UART_RX_NODE DT_ALIAS(uart_gnss)

#if !DT_NODE_EXISTS(UART_RX_NODE)
#error "uart_gnss alias is not defined in the devicetree"
#endif

static const struct device *uart_dev = DEVICE_DT_GET(UART_RX_NODE);

int main(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return -ENODEV;
    }

    printk("UART receiver started on %s\n", uart_dev->name);

    uint8_t c;

    while (1) {
        if (uart_poll_in(uart_dev, &c) == 0) {
            printk("%c", c);
        } else {
            k_msleep(1);
        }
    }

    return 0;
}
#endif