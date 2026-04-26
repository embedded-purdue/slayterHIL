/**
 * @file scheduler.h
 * @brief Scheduler thread: SPI full-duplex async, protobuf decode, counter-based
 *        causality gating, and STM32-initiated clock synchronisation handshake.
 *
 * Data flow
 * ─────────
 *   Pi (SPI Master)
 *     │  10 MHz SPI, DMA-backed full-duplex, async callback
 *     │  MOSI: [LEN_LO][LEN_HI][protobuf payload]
 *     │  MISO: [stm32_sync_frame_t]  ← STM32 clock epoch on every frame
 *     ▼
 *   spi_rx_callback  (ISR)  →  signals spi_rx_sem
 *     │
 *     ▼
 *   scheduler_thread  (thread, priority SCHEDULER_PRIORITY)
 *     │  ── SYNC PHASE ──────────────────────────────────────────
 *     │  Transceive: MISO = sync frame, MOSI = handshake/dummy
 *     │  Wait until Pi sends proto with has_sync_confirmed = true
 *     │  ── STREAMING PHASE ────────────────────────────────────
 *     │  Parse 2-byte length prefix → pb_decode
 *     │  k_msgq_put(scheduler_q, K_NO_WAIT) — drops oldest if full
 *     │
 *     ▼
 *   injection_alarm_cb  (counter alarm ISR)
 *     │  fires when STM32 hardware counter ≥ packet.timestamp_us
 *     │  irq_lock → k_msgq_purge(sensor_update_q) → k_msgq_put → irq_unlock
 *     │
 *     ▼
 *   sensor_update_q  (1-slot gate — see sensor_emulation.h)
 *
 * SPI Wire Protocol
 * ─────────────────
 *  Every SPI transaction is exactly SPI_BUF_SIZE bytes on both MOSI and MISO.
 *
 *  MOSI frame (Pi → STM32):
 *    Byte 0:    payload length low byte  (little-endian uint16)
 *    Byte 1:    payload length high byte
 *    Bytes 2…N: nanopb-encoded SensorPacket proto (N = payload length)
 *    Remaining: zero-padded to SPI_BUF_SIZE
 *
 *  MISO frame (STM32 → Pi):
 *    Bytes 0–7: stm32_sync_frame_t  (magic + STM32 µs timestamp)
 *    Remaining: zero-padded to SPI_BUF_SIZE
 *
 * Clock Sync Handshake (W4 fix)
 * ──────────────────────────────
 *  The STM32 is the SPI slave so it cannot assert CS.  Instead it uses the
 *  full-duplex MISO channel — pre-loaded at DMA-arm time — to broadcast its
 *  hardware clock on every frame.  The Pi reads MISO, computes:
 *
 *    pi_epoch_offset = pi_now_us() - stm32_sync_frame.stm32_now_us
 *
 *  and thereafter sends all timestamp_us fields as:
 *
 *    timestamp_us = pi_simulation_time_us - pi_epoch_offset
 *
 *  making packet.timestamp_us directly comparable to now_us() on the STM32.
 *  Once the Pi has a stable offset it sets has_sync_confirmed = true in its
 *  next proto.  The scheduler thread discards all packets until that flag
 *  arrives and sets the sync_complete atomic flag.
 *
 * DTS Requirements
 * ─────────────────
 *  In your board overlay add:
 *
 *    / {
 *        aliases {
 *            spi-orchestrator = &spi2;      // SPI bus connected to Pi
 *            injection-timer  = &timer6;    // Free-running hardware timer
 *            sync-request-gpio = &gpioa;    // Optional — for Option A fallback
 *        };
 *    };
 *
 *    &timer6 {
 *        status = "okay";
 *        // counter driver will use this as a free-running 1 MHz counter
 *    };
 *
 *  NVIC priorities (lower number = higher priority on Cortex-M):
 *    I2C target ISR  : 1   (highest — DUT timing is hard real-time)
 *    SPI DMA ISR     : 2
 *    Counter/TIM6 ISR: 3   (injection_alarm_cb lives here)
 *    Zephyr SysTick  : as configured by CONFIG_CORTEX_M_SYSTICK_INIT_PRIORITY
 *
 *  Set explicitly in DTS overlay to avoid default-priority surprises (W8).
 */

#ifndef THREADS_SCHEDULER_H
#define THREADS_SCHEDULER_H

#include "threads/sensor_emulation.h"
#include <zephyr/kernel.h>
#include <stdint.h>

/* ── Thread configuration ─────────────────────────────────────────────────── */
#define SCHEDULER_STACK_SIZE   (8192U)
/** Must be strictly higher priority (lower number) than SENSOR_EMULATION_PRIORITY */
#define SCHEDULER_PRIORITY     (5)

/* ── SPI wire-protocol constants ──────────────────────────────────────────── */

/**
 * @brief Total SPI frame size in bytes (MOSI and MISO are the same length).
 *
 * Set to MAX_PROTO_PAYLOAD + SPI_FRAME_HEADER_SIZE.
 * Adjust MAX_PROTO_PAYLOAD to match your largest SensorPacket encoding.
 */
#define MAX_PROTO_PAYLOAD      (250U)
#define SPI_FRAME_HEADER_SIZE  (2U)   /**< 2-byte little-endian length prefix */
#define SPI_BUF_SIZE           (MAX_PROTO_PAYLOAD + SPI_FRAME_HEADER_SIZE)

/* ── MISO sync frame ──────────────────────────────────────────────────────── */

/**
 * @brief Magic sentinel placed at the start of every MISO frame.
 *
 * The Pi validates this on every received frame before trusting stm32_now_us.
 * Chosen to be unlikely to appear in random SPI noise.
 */
#define STM32_SYNC_MAGIC  (0xC0FFEE42UL)

/**
 * @brief MISO frame layout: STM32 clock broadcast on every SPI transaction.
 *
 * Packed to ensure no compiler padding alters the on-wire byte positions.
 * Placed at offset 0 of the TX DMA buffer; the rest of the buffer is zeroed.
 *
 * Pi-side usage:
 *   if (frame.magic == STM32_SYNC_MAGIC):
 *       pi_epoch_offset = pi_now_us() - frame.stm32_now_us
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;         /**< Always STM32_SYNC_MAGIC                    */
    uint32_t stm32_now_us;  /**< k_cyc_to_us_near32(k_cycle_get_32()) at
                             *   the moment this TX DMA buffer was armed.   */
} stm32_sync_frame_t;

_Static_assert(sizeof(stm32_sync_frame_t) == 8,
               "stm32_sync_frame_t must be 8 bytes");

/* ── Staging ring buffer ──────────────────────────────────────────────────── */

/**
 * @brief N-slot ring buffer between the scheduler thread and the timer ISR.
 *
 * Sized to absorb the Pi's burst oversampling without stalling.
 * When full, the scheduler thread evicts the OLDEST packet and inserts the
 * newest, so the counter alarm ISR always sees the most recent physics state.
 */
#define SCHEDULER_Q_LEN   (64U)
extern struct k_msgq scheduler_q;

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Create and start the scheduler thread.
 *
 * Prerequisites (must be satisfied before calling):
 *   1. sensor_emulation_init() has been called — sensor_update_q exists and
 *      I2C targets are registered.
 *   2. The counter device (DT_ALIAS(injection_timer)) is ready.
 *   3. The SPI device (DT_ALIAS(spi_orchestrator)) is ready.
 *
 * The thread will not pass packets to the sensor bus until the Pi completes
 * the clock-sync handshake (has_sync_confirmed = true in the SensorPacket).
 */
void scheduler_init(void);

#endif /* THREADS_SCHEDULER_H */
