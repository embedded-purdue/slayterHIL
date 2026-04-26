/**
 * @file lidar_emulator.h
 * @brief Emulates a LIDAR-Lite v3 distance sensor as an I2C target device.
 *
 * Synchronisation strategy: atomic_t backing store + per-transaction snapshot
 * ─────────────────────────────────────────────────────────────────────────────
 * The simulated distance is held in a 32-bit atomic_t (lower 16 bits used).
 * The sensor emulation thread writes with atomic_set(); the I2C ISR reads
 * with atomic_get().  Both operations are single LDREX/STREX instructions on
 * Cortex-M — fully atomic, no masking required.
 *
 * Two-byte tearing prevention (transaction snapshot cache):
 *   LIDAR-Lite distance spans two consecutive registers:
 *     0x0F  FULL_DELAY_HIGH  — MSB, DUT reads this FIRST
 *     0x10  FULL_DELAY_LOW   — LSB, DUT reads this SECOND
 *
 *   If the backing atomic were read independently for each byte, a write
 *   between the two reads would produce a corrupted measurement (MSB from
 *   distance N, LSB from distance N+1).
 *
 *   Fix: the ISR freezes the distance into transaction_distance_cache when
 *   it serves FULL_DELAY_HIGH (the first byte of the pair).  The LOW byte
 *   is then served from this frozen cache — both bytes always belong to the
 *   same sample, regardless of how many writes occur in between.
 *
 * LIDAR-Lite v3 measurement sequence (as performed by the DUT)
 * ─────────────────────────────────────────────────────────────
 *   1. Write 0x04 to reg 0x00 (ACQ_COMMAND)  — trigger measurement
 *   2. Write 0x01 (set reg pointer to STATUS)
 *   3. Read  1 byte                           — poll until bit 0 = 0 (ready)
 *   4. Write 0x0F (set reg pointer to HIGH)
 *   5. Read  2 bytes                          — [HIGH][LOW] via auto-increment
 *   6. distance_mm = (HIGH << 8) | LOW
 *
 * Emulation of steps 1–3:
 *   The physics engine provides a continuous stream, so STATUS always returns
 *   0x00 (bit 0 = 0, "ready").  ACQ_COMMAND writes are silently absorbed.
 *
 * Emulation of steps 4–6:
 *   read_requested_cb serves HIGH and snapshots the cache.
 *   read_processed_cb auto-increments the register pointer to LOW and serves
 *   from the same frozen cache.
 */

#ifndef THREADS_LIDAR_EMULATOR_H
#define THREADS_LIDAR_EMULATOR_H

#include <zephyr/drivers/i2c.h>
#include <stdint.h>

/* ── LIDAR-Lite v3 register addresses (subset emulated) ───────────────────── */

/** Write 0x04 here to trigger a measurement.  Absorbed silently. */
#define LIDAR_REG_ACQ_COMMAND      0x00

/** Bit 0 = device busy.  Always returns 0x00 (ready). */
#define LIDAR_REG_STATUS           0x01

/** Distance MSB — snapshot point.  Read this first. */
#define LIDAR_REG_FULL_DELAY_HIGH  0x0F

/** Distance LSB — served from frozen snapshot cache.  Read this second. */
#define LIDAR_REG_FULL_DELAY_LOW   0x10

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the LiDAR emulator and register the I2C target.
 *
 * Clears the backing store and snapshot cache to 0, then calls
 * i2c_target_register() on i2c_dev at address LIDAR_ADDRESS (0x62).
 *
 * @param i2c_dev  Zephyr I2C device handle (from DT_ALIAS(i2c_lidar) in main.c).
 */
void lidar_emulator_init(const struct device *i2c_dev);

/**
 * @brief Update the simulated distance.
 *
 * Writes new_distance_mm into the atomic backing store.  The I2C ISR will
 * serve this value (or a later one) on the DUT's next measurement read.
 *
 * This function is ISR-safe — atomic_set() is a single STREX instruction.
 * It may be called from either thread or ISR context.
 *
 * @param new_distance_mm  Distance in millimetres (0 – 65535).
 */
void lidar_emulator_update_distance(uint16_t new_distance_mm);

#endif /* THREADS_LIDAR_EMULATOR_H */
