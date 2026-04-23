/**
 * @file scheduler.h
 * @brief Scheduler thread: SPI async receive, protobuf decode, and timer-based
 *        causality gating of physics engine updates.
 *
 * Responsibilities
 * ────────────────
 *   1. Receive 256-byte framed SPI packets from the Pi (DMA-backed async).
 *   2. Decode each frame's protobuf payload (SensorPacket — combined IMU +
 *      LiDAR + RC, no sensor_id).
 *   3. Map proto fields → device_update_packet_t and push to scheduler_q.
 *   4. Fire the injection timer when the STM32 hardware clock reaches a
 *      packet's timestamp_us, enforcing the "Arrow of Time" constraint:
 *      the DUT is never served physics state from the future.
 *
 * Data flow
 * ─────────
 *   Pi (SPI Master, 10 MHz)
 *     │  DMA transfer → spi_rx_callback (ISR) → spi_rx_sem
 *     ▼
 *   scheduler_thread  (priority SCHEDULER_PRIORITY)
 *     │  2-byte LE length-prefix framing parse
 *     │  nanopb pb_decode → SensorPacket
 *     │  map all fields → device_update_packet_t
 *     │  scheduler_q_put_overwrite()  ← evicts oldest if ring is full
 *     │  try_arm_timer()
 *     ▼
 *   injection_timer_isr  (k_timer expiry — SysTick ISR context)
 *     │  fires when now_us() ≥ packet.timestamp_us
 *     │  irq_lock → k_msgq_purge(sensor_update_q) → k_msgq_put → irq_unlock
 *     ▼
 *   sensor_update_q  (1-slot gate queue — defined in sensor_emulation.h)
 *
 * Wire frame format
 * ─────────────────
 *   [LEN_LOW][LEN_HIGH][protobuf payload ... ][zero-pad to 256 bytes]
 *   LEN = uint16 little-endian, value = length of protobuf payload only.
 *   Maximum payload = 254 bytes (256 - 2 byte header).
 *
 * Clock epoch
 * ───────────
 *   packet.timestamp_us MUST be in the STM32's k_cyc_to_us_near32 epoch.
 *   The clock-sync handshake in main.c establishes this before streaming
 *   starts.  Use CONFIG_HIL_SKIP_CLOCK_SYNC=y (prj.conf) during bring-up
 *   to bypass the handshake and use STM32 boot time as epoch 0.
 */

#ifndef THREADS_SCHEDULER_H
#define THREADS_SCHEDULER_H

#include "threads/sensor_emulation_test.h"
#include <zephyr/kernel.h>

/* ── Thread configuration ─────────────────────────────────────────────────── */
#define SCHEDULER_STACK_SIZE  (8192U)

/**
 * Must be numerically LESS than SENSOR_EMULATION_PRIORITY so the scheduler
 * can always preempt the sensor thread.
 *
 *   SCHEDULER_PRIORITY (5) < SENSOR_EMULATION_PRIORITY (7)
 *   → scheduler runs first when both are ready.
 */
#define SCHEDULER_PRIORITY    (5)

/* ── Staging ring buffer ──────────────────────────────────────────────────── */

/**
 * @brief Depth of the ring buffer between the scheduler thread and the
 *        injection timer ISR.
 *
 * The Pi may transmit faster than the DUT polls (oversampling).  This ring
 * absorbs those bursts.  When full, the OLDEST packet is evicted and the
 * newest is inserted so the ISR always works toward the most recent physics
 * state rather than running perpetually behind.
 *
 * Memory cost: SCHEDULER_Q_LEN × sizeof(device_update_packet_t)
 *   = 64 × ~26 bytes ≈ 1664 bytes of static RAM.
 *
 * Reduce if RAM is constrained; increase if Pi burst rate is very high.
 */
#define SCHEDULER_Q_LEN  (64U)

/**
 * @brief The staging ring buffer.
 *
 * Defined in scheduler.c.  Exposed here so injection_timer_isr (also in
 * scheduler.c) can peek at the next-due packet without making it a global.
 * External code should not write to this queue directly.
 */
extern struct k_msgq scheduler_q;

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Create and start the scheduler thread.
 *
 * Initialises the injection timer and spawns the scheduler thread.  The
 * thread immediately begins waiting for SPI DMA completions.
 *
 * Call AFTER sensor_emulation_init() to guarantee the I2C targets are
 * registered and sensor_update_q exists before the first timer injection.
 */
void scheduler_init(void);

#endif /* THREADS_SCHEDULER_H */
