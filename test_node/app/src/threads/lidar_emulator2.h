/**
 * @file lidar_emulator.h
 * @brief Emulates a LIDAR-Lite v3 distance sensor as an I2C target device.
 *
 * Synchronisation strategy: atomic_t + transaction snapshot cache
 * ────────────────────────────────────────────────────────────────
 * The backing store for the simulated distance is a 32-bit atomic_t.
 * The sensor emulation thread writes to it with atomic_set(); the I2C ISR
 * reads it with atomic_get().  This is lock-free and ISR-safe.
 *
 * Multi-byte tearing prevention:
 *   The LIDAR-Lite reports distance across TWO consecutive registers:
 *     0x0F  FULL_DELAY_HIGH  (MSB, read first)
 *     0x10  FULL_DELAY_LOW   (LSB, read second)
 *
 *   Without a snapshot, a background update between the HIGH and LOW reads
 *   would corrupt the measurement (e.g. HIGH byte from distance 1000 mm,
 *   LOW byte from distance 1001 mm).
 *
 *   The ISR freezes the distance into `transaction_distance_cache` when it
 *   serves the HIGH byte (the first byte of the pair).  The LOW byte is then
 *   served from this frozen cache, not from the live atomic.
 */

#ifndef THREADS_LIDAR_EMULATOR_H
#define THREADS_LIDAR_EMULATOR_H

#include <zephyr/drivers/i2c.h>
#include <stdint.h>

/* ── LIDAR-Lite v3 register map (subset emulated) ─────────────────────────── */
#define LIDAR_REG_ACQ_COMMAND      0x00  /**< Write: trigger measurement      */
#define LIDAR_REG_STATUS           0x01  /**< Bit 0 = busy flag               */
#define LIDAR_REG_FULL_DELAY_HIGH  0x0F  /**< Distance MSB (read this first)  */
#define LIDAR_REG_FULL_DELAY_LOW   0x10  /**< Distance LSB (read this second) */

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the LiDAR emulator and register the I2C target.
 * @param i2c_dev  Handle for the Zephyr I2C device (from DT_ALIAS(i2c_lidar)).
 */
void lidar_emulator_init(const struct device *i2c_dev);

/**
 * @brief Update the simulated distance.  ISR-safe, lock-free.
 *
 * May be called from thread or ISR context.  atomic_set is a single
 * load/store-exclusive sequence on Cortex-M — fully atomic.
 *
 * @param new_distance_mm  New distance value in millimetres.
 */
void lidar_emulator_update_distance(uint16_t new_distance_mm);

#endif /* THREADS_LIDAR_EMULATOR_H */
