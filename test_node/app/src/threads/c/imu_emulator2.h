/**
 * @file imu_emulator.h
 * @brief Emulates a BNO055 IMU as an I2C target device.
 *
 * Synchronisation strategy: Lock-Free Triple Buffer  (W2 fix)
 * ─────────────────────────────────────────────────────────────
 * Three complete copies of the 18-byte IMU register image are maintained.
 * At any moment each buffer is exclusively owned by one of three logical roles:
 *
 *   writer  — the sensor emulation thread is writing here.
 *   pending — the most recently completed write; waiting to be picked up.
 *   reader  — the I2C ISR is (or last was) reading here.
 *
 * The roles rotate via two atomic-exchange operations:
 *
 *   Write path  (sensor thread, thread context):
 *     1. memcpy new state into imu_buf[writer_idx].
 *     2. __DMB() — drain Cortex-M store buffer before publishing.
 *     3. writer_idx = atomic_set(&pending_idx, writer_idx)
 *          → pending_idx now points to the fresh buffer.
 *          → writer_idx now holds the old pending (stale; safe to overwrite).
 *
 *   Read path  (I2C ISR, read_requested_cb — start of each transaction):
 *     1. reader_idx = atomic_set(&pending_idx, reader_idx)
 *          → reader_idx now points to the freshest complete buffer.
 *          → pending_idx now holds the old reader (stale; writer may take it).
 *     2. All subsequent read_processed_cb calls serve bytes from reader_idx
 *        without touching pending_idx again.
 *
 * Safety invariant
 * ────────────────
 * The multiset { writer_idx, pending_idx, reader_idx } is always a
 * permutation of { 0, 1, 2 }.  Each atomic_set is a single-instruction
 * exchange (LDREX/STREX on Cortex-M) so the invariant is never broken.
 *
 * Consequence: writer_idx ≠ reader_idx at all times.  The sensor thread
 * never writes to the buffer the ISR is reading, even if the writer calls
 * imu_emulator_update_data() many times in rapid succession during a single
 * I2C transaction.  This is the fundamental advantage over the ping-pong
 * double buffer (which failed if the writer updated twice per transaction).
 *
 * Comparison with previous ping-pong (double buffer)
 * ───────────────────────────────────────────────────
 * Double buffer: safe for ONE writer update per I2C transaction.
 *   Risk (ANALYSIS.md §W2): second update during a 405 µs I2C transaction
 *   (at 400 kHz, 18 bytes) writes into the buffer the ISR is currently
 *   reading if the 500 µs sensor-thread sleep elapses twice in that window.
 *
 * Triple buffer: safe for ANY number of writer updates per transaction.
 *   The writer always targets writer_idx.  Only after the write completes
 *   does writer_idx change (via atomic_set) — and it changes to the OLD
 *   pending, never to reader_idx.
 */

#ifndef THREADS_IMU_EMULATOR_H
#define THREADS_IMU_EMULATOR_H

#include "threads/sensor_emulation.h"
#include <zephyr/drivers/i2c.h>

/* ── BNO055 register map (subset emulated) ────────────────────────────────── */
#define IMU_REG_GYRO_X_LSB   0x14
#define IMU_REG_GYRO_X_MSB   0x15
#define IMU_REG_GYRO_Y_LSB   0x16
#define IMU_REG_GYRO_Y_MSB   0x17
#define IMU_REG_GYRO_Z_LSB   0x18
#define IMU_REG_GYRO_Z_MSB   0x19

#define IMU_REG_EUL_X_LSB    0x1A
#define IMU_REG_EUL_X_MSB    0x1B
#define IMU_REG_EUL_Y_LSB    0x1C
#define IMU_REG_EUL_Y_MSB    0x1D
#define IMU_REG_EUL_Z_LSB    0x1E
#define IMU_REG_EUL_Z_MSB    0x1F

#define IMU_REG_LIA_X_LSB    0x28
#define IMU_REG_LIA_X_MSB    0x29
#define IMU_REG_LIA_Y_LSB    0x2A
#define IMU_REG_LIA_Y_MSB    0x2B
#define IMU_REG_LIA_Z_LSB    0x2C
#define IMU_REG_LIA_Z_MSB    0x2D

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Register the I2C target and zero-fill all three triple-buffer slots.
 *
 * Sets the initial buffer ownership:
 *   writer_idx  = 1  (sensor thread will write here first)
 *   pending_idx = 2  (starts "stale" — zeros — ready to be overwritten)
 *   reader_idx  = 0  (ISR reads zeros until first update arrives)
 *
 * @param i2c_dev  Zephyr I2C device handle (from DT_ALIAS(i2c_imu)).
 */
void imu_emulator_init(const struct device *i2c_dev);

/**
 * @brief Push a new IMU state snapshot into the triple buffer (thread context).
 *
 * Lock-free.  May be called any number of times per I2C transaction without
 * risk of corrupting an in-progress ISR read.
 *
 * Must NOT be called from ISR context — memcpy on 18 bytes is not
 * interruptible by the I2C ISR without protection, and the ISR must not block.
 *
 * @param new_data  Pointer to a complete imu_data_t snapshot.
 */
void imu_emulator_update_data(const imu_data_t *new_data);

#endif /* THREADS_IMU_EMULATOR_H */
