/**
 * @file imu_emulator.h
 * @brief Emulates a BNO055 IMU as an I2C target device.
 *
 * Synchronisation strategy: Ping-Pong Double Buffer
 * ──────────────────────────────────────────────────
 * Two complete copies of the 18-byte imu_data_t register image are kept.
 * At any moment one buffer is "active" (safe for the I2C ISR to read) and
 * the other is "inactive" (safe for the sensor thread to write).
 *
 * Write path  (sensor emulation thread — thread context only):
 *   1. inactive = 1 - atomic_get(&active_idx)
 *   2. memcpy new imu_data_t into imu_buf[inactive]
 *   3. __DMB()  — drain Cortex-M store buffer before publishing
 *   4. atomic_set(&active_idx, inactive)  — flip; new buffer is now active
 *
 * Read path  (I2C target callbacks — ISR context):
 *   read_requested_cb:   snap = atomic_get(&active_idx)   ← snapshot ONCE
 *   read_processed_cb:   serve bytes from imu_buf[snap]   ← same snapshot
 *   stop_cb:             reset register pointer
 *
 * Guarantees:
 *   • No struct tearing — ISR reads a single coherent 18-byte snapshot.
 *   • No blocking       — zero mutexes or spinlocks in the ISR path.
 *   • No starvation     — writer never waits for a reader to finish.
 *
 * Known residual race (double-write):
 *   If the sensor thread writes twice between one read_requested_cb and its
 *   matching stop_cb, the second write lands on the buffer the ISR is
 *   currently reading (because flip #1 made it inactive, flip #2 made it
 *   active again, flip #3 makes it inactive again — now the writer targets
 *   the ISR's buffer).  At 400 kHz I2C (18 bytes ≈ 405 µs per transaction)
 *   and 500 µs sensor thread sleep, this window is narrow but non-zero.
 *   See ANALYSIS.md §W2 for the triple-buffer mitigation.
 *
 * BNO055 register map (subset emulated)
 * ──────────────────────────────────────
 *   0x14 – 0x19   GYR_DATA_X/Y/Z   (gyro,  6 bytes)
 *   0x1A – 0x1F   EUL_DATA_X/Y/Z   (euler, 6 bytes)
 *   0x20 – 0x27   QUA_DATA_W/X/Y/Z (quaternion — NOT emulated, returns 0xFF)
 *   0x28 – 0x2D   LIA_DATA_X/Y/Z   (linear accel, 6 bytes)
 *
 * The DUT must issue separate read transactions for register groups that
 * are not contiguous (e.g. euler then linear_accel — there is an 8-byte
 * quaternion gap between 0x1F and 0x28).
 */

#ifndef THREADS_IMU_EMULATOR_H
#define THREADS_IMU_EMULATOR_H

#include "threads/sensor_emulation_test.h"
#include <zephyr/drivers/i2c.h>

/* ── BNO055 register addresses (subset emulated) ──────────────────────────── */

/* Gyroscope data — 6 bytes */
#define IMU_REG_GYRO_X_LSB   0x14
#define IMU_REG_GYRO_X_MSB   0x15
#define IMU_REG_GYRO_Y_LSB   0x16
#define IMU_REG_GYRO_Y_MSB   0x17
#define IMU_REG_GYRO_Z_LSB   0x18
#define IMU_REG_GYRO_Z_MSB   0x19

/* Euler angles — 6 bytes */
#define IMU_REG_EUL_X_LSB    0x1A
#define IMU_REG_EUL_X_MSB    0x1B
#define IMU_REG_EUL_Y_LSB    0x1C
#define IMU_REG_EUL_Y_MSB    0x1D
#define IMU_REG_EUL_Z_LSB    0x1E
#define IMU_REG_EUL_Z_MSB    0x1F

/* Quaternion 0x20–0x27 — NOT emulated, serve_imu_byte() returns 0xFF */

/* Linear acceleration — 6 bytes */
#define IMU_REG_LIA_X_LSB    0x28
#define IMU_REG_LIA_X_MSB    0x29
#define IMU_REG_LIA_Y_LSB    0x2A
#define IMU_REG_LIA_Y_MSB    0x2B
#define IMU_REG_LIA_Z_LSB    0x2C
#define IMU_REG_LIA_Z_MSB    0x2D

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the IMU emulator and register the I2C target at IMU_ADDRESS.
 *
 * Zero-fills both ping-pong buffers so the DUT reads plausible zeros before
 * the first physics packet arrives.
 *
 * @param i2c_dev  Zephyr I2C device handle (from DT_ALIAS(i2c_imu) in main.c).
 */
void imu_emulator_init(const struct device *i2c_dev);

/**
 * @brief Write a new IMU state into the inactive ping-pong buffer and publish it.
 *
 * After this call returns, the I2C ISR will read the new data on the next
 * transaction (or the current one, if a flip races with an ongoing read —
 * see the double-write note in the file header).
 *
 * @param new_data  Pointer to a complete, valid imu_data_t.  The contents
 *                  are copied into the inactive buffer; the caller may free
 *                  or reuse new_data immediately after this call returns.
 *
 * @note MUST be called from thread context only.  memcpy of 18 bytes is not
 *       safe from ISR context because it is not guaranteed to be atomic and
 *       may be preempted mid-copy on some Cortex-M implementations.
 */
void imu_emulator_update_data(const imu_data_t *new_data);

#endif /* THREADS_IMU_EMULATOR_H */
