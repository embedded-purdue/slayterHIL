/** Raspberry Pi -> STM32 sensor-stream MVP. Explicit IMMEDIATE injection. */
#include "threads/scheduler_test.h"
#include "threads/sensor_emulation_test.h"
#include "hil_diag.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
extern atomic_t spi_err_count;
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
    const struct device *spi = DEVICE_DT_GET(DT_ALIAS(spi_orchestrator));
    const struct device *lidar = DEVICE_DT_GET(DT_ALIAS(i2c_lidar));
    const struct device *imu = DEVICE_DT_GET(DT_ALIAS(i2c_imu));
    if (!device_is_ready(spi) || !device_is_ready(lidar) || !device_is_ready(imu)) {
        LOG_ERR("Required SPI/I2C device is not ready");
        return -ENODEV;
    }
    int err = sensor_emulation_init(lidar, imu);
    if (err) {
        LOG_ERR("Sensor emulation initialization failed: %d", err);
        return err;
    }
    bool led_ready = gpio_is_ready_dt(&led) && gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) == 0;
    scheduler_init();
    LOG_INF("MVP ready on %s: IMMEDIATE injection, no clock sync", CONFIG_BOARD);
    hil_diag_snapshot_t prev = {0};
    int64_t previous_ms = k_uptime_get();
    while (1) {
        k_sleep(K_SECONDS(2));
        hil_diag_snapshot_t cur;
        hil_diag_get_snapshot(&cur);
        const int64_t now_ms = k_uptime_get();
        const uint32_t elapsed_ms = (uint32_t)(now_ms - previous_ms);
        printk("rx=%u applied=%u rx_hz=%u apply_hz=%u decode_fail=%u evict=%u spi_err=%u sim_us=%u\n",
               cur.spi_rx_count, cur.timer_inject_count,
               (cur.spi_rx_count - prev.spi_rx_count) * 1000 / elapsed_ms,
               (cur.timer_inject_count - prev.timer_inject_count) * 1000 / elapsed_ms,
               cur.decode_fail_count, cur.scheduler_q_evict, (uint32_t)atomic_get(&spi_err_count),
               cur.last_inject_ts_us);
        if (led_ready) gpio_pin_toggle_dt(&led);
        prev = cur;
        previous_ms = now_ms;
    }
}
