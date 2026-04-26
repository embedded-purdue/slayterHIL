/**
 * @file imu_emulator.c  [TEST-INSTRUMENTED VERSION]
 * @brief BNO055 IMU emulator — ping-pong double buffer + diagnostics.
 *
 * Additions vs base:
 *   + hil_diag_inc_imu_read() in read_requested_cb.
 *   + hil_diag_set_last_euler_x() stores the euler_x value being served.
 *   + LOG_DBG in read_requested_cb (deferred — safe in ISR).
 *   + LOG_INF in stop_cb to log end of each transaction.
 */

#include "threads/imu_emulator_test.h"
#include "threads/sensor_emulation_test.h"
#include "hil_diag.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(imu_emulator, LOG_LEVEL_DBG);

/* ── Ping-Pong Double Buffer ─────────────────────────────────────────────── */
static imu_data_t imu_buf[2];
static atomic_t   active_idx = ATOMIC_INIT(0);

/* ── I2C Transaction State ────────────────────────────────────────────────── */
static uint8_t imu_current_reg  = 0x00;
static int     imu_snap_buf_idx = 0;

/* ── Register byte servant ───────────────────────────────────────────────── */
static uint8_t serve_imu_byte(int buf_idx, uint8_t reg)
{
    const imu_data_t *d = &imu_buf[buf_idx];
    const uint8_t *raw;
    uint8_t offset;

    switch (reg) {
    case IMU_REG_GYRO_X_LSB ... IMU_REG_GYRO_Z_MSB:
        offset = reg - IMU_REG_GYRO_X_LSB;
        raw    = (const uint8_t *)&d->gyro;
        return raw[offset];
    case IMU_REG_EUL_X_LSB ... IMU_REG_EUL_Z_MSB:
        offset = reg - IMU_REG_EUL_X_LSB;
        raw    = (const uint8_t *)&d->euler_angles;
        return raw[offset];
    case IMU_REG_LIA_X_LSB ... IMU_REG_LIA_Z_MSB:
        offset = reg - IMU_REG_LIA_X_LSB;
        raw    = (const uint8_t *)&d->linear_acceleration;
        return raw[offset];
    default:
        return 0xFF;
    }
}

/* ── Public write API (thread context) ───────────────────────────────────── */
void imu_emulator_update_data(const imu_data_t *new_data)
{
    int active   = (int)atomic_get(&active_idx);
    int inactive = 1 - active;
    memcpy(&imu_buf[inactive], new_data, sizeof(imu_data_t));
    __DMB();
    atomic_set(&active_idx, (atomic_val_t)inactive);
}

/* ── I2C Callbacks (ISR context) ─────────────────────────────────────────── */
static int imu_write_requested_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    return 0;
}

static int imu_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
    ARG_UNUSED(config);
    imu_current_reg = val;
    LOG_DBG("[IMU] DUT set reg=0x%02X", val);
    return 0;
}

static int imu_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);

    /* Snapshot the active buffer index for this entire transaction. */
    imu_snap_buf_idx = (int)atomic_get(&active_idx);
    *val = serve_imu_byte(imu_snap_buf_idx, imu_current_reg);

    hil_diag_inc_imu_read();

    /* Store euler_x (LSB + MSB pair at 0x1A, 0x1B) for diagnostic display. */
    if (imu_current_reg == IMU_REG_EUL_X_LSB) {
        int16_t ex = (int16_t)(
            (uint16_t)serve_imu_byte(imu_snap_buf_idx, IMU_REG_EUL_X_LSB) |
            ((uint16_t)serve_imu_byte(imu_snap_buf_idx, IMU_REG_EUL_X_MSB) << 8));
        hil_diag_set_last_euler_x((int32_t)ex);
    }

    LOG_DBG("[IMU READ_REQ] buf=%d reg=0x%02X val=0x%02X",
            imu_snap_buf_idx, imu_current_reg, *val);

    imu_current_reg++;
    return 0;
}

static int imu_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);
    *val = serve_imu_byte(imu_snap_buf_idx, imu_current_reg);
    LOG_DBG("[IMU READ_PROC] buf=%d reg=0x%02X val=0x%02X",
            imu_snap_buf_idx, imu_current_reg, *val);
    imu_current_reg++;
    return 0;
}

static int imu_stop_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    /* Log registers that were read during this transaction */
    uint8_t start_reg = imu_current_reg;   /* already incremented past last byte */
    LOG_DBG("[IMU STOP] transaction ended. Last reg=0x%02X", start_reg - 1);
    imu_current_reg  = 0x00;
    imu_snap_buf_idx = 0;
    return 0;
}

/* ── Registration ────────────────────────────────────────────────────────── */
static struct i2c_target_callbacks imu_callbacks = {
    .write_requested = imu_write_requested_cb,
    .write_received  = imu_write_received_cb,
    .read_requested  = imu_read_requested_cb,
    .read_processed  = imu_read_processed_cb,
    .stop            = imu_stop_cb,
};

static struct i2c_target_config imu_config = {
    .address   = IMU_ADDRESS,
    .callbacks = &imu_callbacks,
};

void imu_emulator_init(const struct device *i2c_dev)
{
    memset(imu_buf, 0, sizeof(imu_buf));
    atomic_set(&active_idx, 0);

    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("IMU I2C device not ready");
        return;
    }
    if (i2c_target_register(i2c_dev, &imu_config) < 0) {
        LOG_ERR("Failed to register IMU at 0x%02X", IMU_ADDRESS);
        return;
    }
    LOG_INF("IMU target registered at 0x%02X", IMU_ADDRESS);
}
