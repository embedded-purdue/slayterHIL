/**
 * @file lidar_emulator.c
 * @brief LIDAR-Lite v3 emulator: atomic distance store + snapshot cache.
 *
 * LIDAR-Lite v3 measurement read sequence (as performed by the ESP32 DUT):
 *   1. Write 0x04 to reg 0x00 (ACQ_COMMAND) — trigger measurement.
 *   2. Poll reg 0x01 (STATUS) until bit 0 = 0 (measurement ready).
 *   3. Read reg 0x0F (FULL_DELAY_HIGH) — MSB.
 *   4. Read reg 0x10 (FULL_DELAY_LOW)  — LSB.
 *
 * Emulation of steps 1–2:
 *   Our physics engine provides a continuous distance stream, so we always
 *   return STATUS = 0x00 (bit 0 = 0, "ready"), bypassing the polling loop.
 *   We silently absorb any writes to ACQ_COMMAND.
 *
 * Emulation of steps 3–4 (the critical path):
 *   The snapshot cache (transaction_distance_cache) is refreshed on the HIGH
 *   byte read (0x0F).  The LOW byte (0x10) is always served from this cache.
 *   This guarantees both bytes belong to the same 16-bit sample, regardless
 *   of how fast the sensor thread is updating the backing atomic.
 *
 * All I2C callbacks execute in ISR context.  Zero blocking operations.
 */

#include "threads/lidar_emulator.h"
#include "threads/sensor_emulation.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lidar_emulator, LOG_LEVEL_INF);

/* ── Atomic distance backing store ───────────────────────────────────────── */
/*
 * Zephyr's atomic_t is a typedef for atomic_val_t, which is always 32 bits.
 * We store the uint16_t distance in the lower 16 bits; the upper 16 are zero.
 * atomic_get/set use LDREX/STREX on Cortex-M — guaranteed atomic, no IRQ
 * masking required.
 */
static atomic_t simulated_distance_mm = ATOMIC_INIT(0);

/* ── Transaction snapshot cache ─────────────────────────────────────────── */
/*
 * This variable is ONLY accessed from within I2C ISR callbacks, which are
 * serialised by the I2C hardware (only one active transaction at a time).
 * It therefore needs no additional protection.
 */
static uint16_t transaction_distance_cache = 0;

/* ── I2C state machine (ISR-local) ───────────────────────────────────────── */
static uint8_t active_register          = LIDAR_REG_ACQ_COMMAND;
static bool    next_byte_is_reg_address = true;

/* ── Register byte servant ───────────────────────────────────────────────── */
/**
 * @brief Serve one byte from the emulated LIDAR-Lite register map.
 *
 * Snapshot semantics:
 *   When the DUT reads FULL_DELAY_HIGH (0x0F), we freeze the current
 *   distance into transaction_distance_cache.  The HIGH byte is served
 *   from this snapshot.  When the DUT reads FULL_DELAY_LOW (0x10), the
 *   LOW byte is served from the SAME snapshot.
 *
 *   Consequence: if the sensor thread updates simulated_distance_mm between
 *   the HIGH and LOW reads, the DUT still receives a coherent 16-bit value.
 *
 * @param reg  Register address requested by the DUT.
 * @return     Byte value to place on the I2C bus.
 */
static uint8_t serve_register_byte(uint8_t reg)
{
    switch (reg) {

    case LIDAR_REG_STATUS:
        /*
         * Bit 0 = "device busy".  Returning 0x00 tells the DUT the last
         * measurement is valid and ready to read.  This skips the polling
         * loop the ESP32 firmware would otherwise run.
         */
        return 0x00;

    case LIDAR_REG_FULL_DELAY_HIGH:
        /*
         * First byte of the distance pair — SNAPSHOT HERE.
         *
         * atomic_get is a single LDREX instruction: ISR-safe, no masking.
         * The snapshot is valid for the lifetime of this transaction.
         */
        transaction_distance_cache =
            (uint16_t)(atomic_get(&simulated_distance_mm) & 0xFFFFU);
        return (uint8_t)((transaction_distance_cache >> 8) & 0xFFU);

    case LIDAR_REG_FULL_DELAY_LOW:
        /*
         * Second byte of the distance pair — serve from FROZEN CACHE.
         *
         * Never call atomic_get here.  If the DUT jumped directly to 0x10
         * without reading 0x0F first, transaction_distance_cache holds the
         * value from the previous transaction's HIGH read (initialised to 0
         * at boot), which is the safest fallback.
         */
        return (uint8_t)(transaction_distance_cache & 0xFFU);

    default:
        return 0xFF;   /* undefined register — I2C convention */
    }
}

/* ── I2C Callbacks (ISR context — no blocking, no mutexes) ──────────────── */

static int lidar_write_requested_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    /*
     * Master is beginning a write phase (register address phase).
     * The next byte received via write_received_cb will be the register addr.
     */
    next_byte_is_reg_address = true;
    return 0;
}

static int lidar_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
    ARG_UNUSED(config);

    if (next_byte_is_reg_address) {
        active_register          = val;
        next_byte_is_reg_address = false;
    } else {
        /*
         * Data byte following the register address.
         * e.g. the DUT writes 0x04 to ACQ_COMMAND to trigger a measurement.
         * We absorb this silently — our physics engine provides continuous data.
         * Auto-increment the register pointer for multi-byte write sequences.
         */
        active_register++;
    }
    return 0;
}

static int lidar_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);
    /*
     * Master has issued a repeated-START and is clocking in the first byte.
     * active_register was set by the preceding write_received_cb.
     */
    *val = serve_register_byte(active_register);
    return 0;
}

static int lidar_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
    ARG_UNUSED(config);
    /*
     * Master ACKed the last byte and wants the next one.
     * Auto-increment the register pointer (LIDAR-Lite hardware behaviour).
     */
    active_register++;
    *val = serve_register_byte(active_register);
    return 0;
}

static int lidar_stop_cb(struct i2c_target_config *config)
{
    ARG_UNUSED(config);
    /*
     * STOP condition received.  Reset state for the next transaction.
     * Do NOT reset transaction_distance_cache: it remains valid until the
     * next HIGH byte read naturally refreshes it.
     */
    next_byte_is_reg_address = true;
    return 0;
}

/* ── I2C Target registration ─────────────────────────────────────────────── */

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
        LOG_ERR("Failed to register LiDAR I2C target at 0x%02X", LIDAR_ADDRESS);
        return;
    }
    LOG_INF("LiDAR I2C target registered at 0x%02X", LIDAR_ADDRESS);
}

void lidar_emulator_update_distance(uint16_t new_distance_mm)
{
    atomic_set(&simulated_distance_mm, (atomic_val_t)new_distance_mm);
}
