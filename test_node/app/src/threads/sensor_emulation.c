#include "threads/sensor_emulation.h"
#include "threads/rc.h"
#include "threads/imu_emulator.h"
#include "threads/lidar_emulator.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

static const struct device *i2c_lidar = DEVICE_DT_GET(DT_ALIAS(i2c_lidar));
static const struct device *i2c_imu = DEVICE_DT_GET(DT_ALIAS(i2c_imu));
static const struct device *uart_rc = DEVICE_DT_GET(DT_ALIAS(uart_rc));

// register logging module
LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

// Initialize and start thread
void sensor_emulation_init() {
    // Any initialization
    lidar_emulation_init(i2c_lidar);
    imu_emulator_init(i2c_imu);

    if(rc_init(uart_rc) < 0) {
        LOG_ERR("Failed to initialize RC UART");
        return;
    }
}
