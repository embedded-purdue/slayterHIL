/**
 * @file lidar_emulator.c  [TEST-INSTRUMENTED VERSION]
 * @brief LIDAR-Lite v3 emulator — atomic + snapshot cache + diagnostics.
 *
 * Additions vs base:
 *   + hil_diag_inc_lidar_read() in read_requested_cb.
 *   + hil_diag_set_last_lidar() stores the distance being served.
 *   + LOG_DBG in read_requested_cb and stop_cb.
 */

#include "threads/lidar_emulator_test.h"
#include "threads/sensor_emulation_test.h"
#include "hil_diag.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lidar_emulator, LOG_LEVEL_DBG);

/* ── Atomic distance backing store ───────────────────────────────────────── */
static atomic_t  simulated_distance_mm     = ATOMIC_INIT(0);
static uint16_t  transaction_distance_cache = 0;

/* ── I2C state machine ───────────────────────────────────────────────────── */
static uint8_t active_register          = LIDAR_REG_ACQ_COMMAND;
static bool    next_byte_is_reg_address = true;

/* ── Register byte servant ───────────────────────────────────────────────── */
static uint8_t serve_register_byte(uint8_t reg)
{
    switch (reg) {
    case LIDAR_REG_STATUS:
        return 0x00;   /* always ready */

    case LIDAR_REG_FULL_DELAY_HIGH:
        /* Snapshot point — freeze distance into cache. */
        transaction_distance_cache =
            (uint16_t)(atomic_get(&simulated_distance_mm) & 0xFFFFU);
        return (uint8_t)((transaction_distance_cache >> 8) & 0xFFU);

    case LIDAR_REG_FULL_DELAY_LOW:
        return (uint8_t)(transaction_distance_cache & 0xFFU);

    default:
        return 0xFF;
    }
}

/* ── I2C Callbacks (ISR context) ─────────────────────────────────────────── */
static int lidar_write_requested_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    next_byte_is_reg_address = true;
    return 0;
}

static int lidar_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
    ARG_UNUSED(config);
    if (next_byte_is_reg_address) {
        active_register          = val;
        next_byte_is_reg_address = false;
        LOG_DBG("[LIDAR] DUT set reg=0x%02X", val);
    } else {
        LOG_DBG("[LIDAR] DUT wrote val=0x%02X to reg=0x%02X (absorbed)",
                val, active_register);
        active_register++;
    }
    return 0;
}

static int lidar_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);
    *val = serve_register_byte(active_register);

    hil_diag_inc_lidar_read();

    /* If this is the HIGH byte, we just snapshotted — log the full distance. */
    if (active_register == LIDAR_REG_FULL_DELAY_HIGH) {
        hil_diag_set_last_lidar((int32_t)transaction_distance_cache);
        LOG_DBG("[LIDAR READ_REQ] reg=0x%02X (HIGH) dist=%u mm val=0x%02X",
                active_register, transaction_distance_cache, *val);
    } else {
        LOG_DBG("[LIDAR READ_REQ] reg=0x%02X val=0x%02X", active_register, *val);
    }

    return 0;
}

static int lidar_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);
    active_register++;
    *val = serve_register_byte(active_register);
    LOG_DBG("[LIDAR READ_PROC] reg=0x%02X val=0x%02X", active_register, *val);
    return 0;
}

static int lidar_stop_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    LOG_DBG("[LIDAR STOP] dist served=%u mm", transaction_distance_cache);
    next_byte_is_reg_address = true;
    return 0;
}

/* ── Registration ────────────────────────────────────────────────────────── */
static struct i2c_target_callbacks lidar_callbacks = {
    .write_requested = lidar_write_requested_cb,
    .write_received  = lidar_write_received_cb,
    .read_requested  = lidar_read_requested_cb,
    .read_processed  = lidar_read_processed_cb,
    .stop            = lidar_stop_cb,
};

static struct i2c_target_config lidar_config = {
    .address   = LIDAR_ADDRESS,
    .callbacks = &lidar_callbacks,
};

/* ── Public API ──────────────────────────────────────────────────────────── */
void lidar_emulator_init(const struct device *i2c_dev)
{
    atomic_set(&simulated_distance_mm, 0);
    transaction_distance_cache = 0;

    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("LiDAR I2C device not ready");
        return;
    }
    if (i2c_target_register(i2c_dev, &lidar_config) < 0) {
        LOG_ERR("Failed to register LiDAR at 0x%02X", LIDAR_ADDRESS);
        return;
    }
    LOG_INF("LiDAR target registered at 0x%02X", LIDAR_ADDRESS);
}

void lidar_emulator_update_distance(uint16_t new_distance_mm)
{
    atomic_set(&simulated_distance_mm, (atomic_val_t)new_distance_mm);
}
