/**
 * @file scheduler.c
 * @brief Scheduler thread — full-duplex SPI, clock-sync handshake, framed
 *        protobuf decode, and hardware-counter-based causality gating.
 *
 * Changes from the previous revision (see ANALYSIS.md for rationale):
 * ────────────────────────────────────────────────────────────────────
 *
 * [W4 fix] STM32-initiated clock-sync handshake via full-duplex MISO
 *   The Pi cannot provide a deterministic timestamp because Linux scheduling
 *   jitter means the actual "send time" is unknown.  Instead, the STM32
 *   broadcasts its own hardware clock on MISO at every SPI frame — captured
 *   at DMA-arm time (nanosecond-accurate) — and the Pi uses this to compute
 *   its epoch offset.  No GPIO wire required.
 *
 *   Protocol:
 *     MISO bytes 0–7: stm32_sync_frame_t { magic, stm32_now_us }
 *     Pi computes:    pi_epoch_offset = pi_now_us() - stm32_now_us
 *     Pi sends:       timestamp_us = sim_time_us - pi_epoch_offset
 *     Pi sets:        has_sync_confirmed = true in first real data proto
 *   The scheduler thread discards all packets until sync_confirmed is set.
 *
 * [W5 fix] SPI framing: 2-byte little-endian length prefix
 *   MOSI frame: [LEN_LO][LEN_HI][protobuf payload...][zero-pad]
 *   Prevents pb_decode from consuming garbage padding bytes.
 *
 * [W1 fix] Hardware counter for sub-µs injection timing
 *   Replaces k_timer (1 ms resolution at default tick rate) with Zephyr's
 *   counter driver API backed by a free-running hardware timer (TIM6 on
 *   STM32F4).  counter_us_to_ticks() converts µs delays to exact timer ticks.
 *   Alarm fires in counter ISR context — same ISR-safe API constraints apply.
 *
 * [S4 preserved] SPI full-duplex double-buffer
 *   spi_transceive_async replaces spi_read_async.  Each DMA arm provides
 *   both a pre-loaded TX buffer (sync frame) and a free RX buffer.  The
 *   immediate re-arm before decode is preserved; the re-arm gap remains
 *   ~pointer-swap-time (~ns), not decode-time (~µs).
 *
 * Concurrency summary
 * ───────────────────
 *  scheduler_thread   (priority 5, thread context)
 *    → writes scheduler_q (64 slots), reads decode_buf, writes prep_tx_buf
 *  injection_alarm_cb (counter ISR context)
 *    → reads scheduler_q, writes sensor_update_q via irq_lock/purge/put
 *    → calls try_arm_counter() to re-arm for next packet
 *  sensor_emulation_thread (priority 7)
 *    → peek-reads sensor_update_q, calls emulator update fns
 *  I2C ISRs
 *    → atomic/ping-pong reads from emulator state
 */

#include "threads/scheduler.h"
#include "threads/sensor_emulation.h"

#include <sensor_data.pb.h>   /* nanopb-generated from your .proto */
#include <pb_decode.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(scheduler, LOG_LEVEL_INF);

/* ══════════════════════════════════════════════════════════════════════════
 * Device handles
 * ══════════════════════════════════════════════════════════════════════════ */

#define SPI_DEV_NODE       DT_ALIAS(spi_orchestrator)
#define COUNTER_DEV_NODE   DT_ALIAS(injection_timer)

static const struct device *const spi_dev     = DEVICE_DT_GET(SPI_DEV_NODE);
static const struct device *const counter_dev = DEVICE_DT_GET(COUNTER_DEV_NODE);

/* ══════════════════════════════════════════════════════════════════════════
 * SPI full-duplex double-buffer
 * ══════════════════════════════════════════════════════════════════════════
 *
 * RX side (Pi → STM32, MOSI):
 *   dma_rx_buf   — being filled by DMA hardware right now.
 *   decode_rx_buf — last completed frame; being parsed by the thread.
 *
 * TX side (STM32 → Pi, MISO):
 *   dma_tx_buf   — being clocked out by DMA hardware right now.
 *   prep_tx_buf  — free for the thread to populate with the next sync frame.
 *
 * Swap happens atomically (pointer exchange) the instant DMA signals done,
 * then the new pair is immediately re-armed before decode begins.
 */
static uint8_t rx_buf_a[SPI_BUF_SIZE];
static uint8_t rx_buf_b[SPI_BUF_SIZE];
static uint8_t tx_buf_a[SPI_BUF_SIZE];
static uint8_t tx_buf_b[SPI_BUF_SIZE];

static uint8_t *dma_rx_buf   = rx_buf_a;
static uint8_t *decode_rx_buf = rx_buf_b;
static uint8_t *dma_tx_buf   = tx_buf_a;
static uint8_t *prep_tx_buf  = tx_buf_b;

/* SPI buffer descriptors — .buf pointers are updated in-place before each arm */
static struct spi_buf     spi_rx_desc = { .buf = NULL, .len = SPI_BUF_SIZE };
static struct spi_buf_set spi_rx_set  = { .buffers = &spi_rx_desc, .count = 1 };

static struct spi_buf     spi_tx_desc = { .buf = NULL, .len = SPI_BUF_SIZE };
static struct spi_buf_set spi_tx_set  = { .buffers = &spi_tx_desc, .count = 1 };

/* SPI slave config: 10 MHz, MSB-first, 8-bit words.
 * NOTE: frequency is ignored by the STM32 SPI slave hardware (Pi sets clock).
 *       It is kept for driver-layer completeness. */
static const struct spi_config spi_cfg = {
    .frequency = 10000000U,
    .operation = SPI_OP_MODE_SLAVE | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
};

/* ══════════════════════════════════════════════════════════════════════════
 * Staging ring buffer (scheduler → counter ISR)
 * ══════════════════════════════════════════════════════════════════════════ */
K_MSGQ_DEFINE(scheduler_q,
              sizeof(device_update_packet_t),
              SCHEDULER_Q_LEN,
              4);

/* ══════════════════════════════════════════════════════════════════════════
 * Synchronisation primitives
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * Binary semaphore: spi_rx_callback gives it, scheduler thread takes it.
 * Max count = 2: the double-buffer scheme can produce two completions in
 * rapid succession (the immediate re-arm fires while we are still decoding
 * the first frame).
 */
static K_SEM_DEFINE(spi_rx_sem, 0, 2);

/*
 * Clock-sync gate.
 * 0 = Pi has not yet confirmed its epoch offset; drop all incoming packets.
 * 1 = Sync handshake complete; packets flow normally into scheduler_q.
 *
 * Set to 1 when the scheduler thread sees has_sync_confirmed = true in an
 * incoming SensorPacket proto.  Never cleared after being set.
 */
static atomic_t sync_confirmed = ATOMIC_INIT(0);

/*
 * Counter alarm armed flag.
 * 0 = counter alarm is idle.
 * 1 = counter alarm is pending.
 * Managed with atomic_cas to guarantee at-most-one-arming from both the
 * scheduler thread and the counter ISR (injection_alarm_cb).
 */
static atomic_t alarm_armed = ATOMIC_INIT(0);

/* Counter alarm config struct — reused across arm calls. */
static struct counter_alarm_cfg injection_alarm_cfg;

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: STM32 hardware time in µs
 * ══════════════════════════════════════════════════════════════════════════ */
static inline uint32_t now_us(void)
{
    return k_cyc_to_us_near32(k_cycle_get_32());
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: populate a TX buffer with the current sync frame
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Called immediately before arming each DMA transfer.  Capturing now_us()
 * at DMA-arm time (not at CS-assert time) is what makes the timestamp
 * deterministic — the Pi's CS-assert latency is irrelevant.
 */
static void populate_sync_frame(uint8_t *tx_buf)
{
    memset(tx_buf, 0, SPI_BUF_SIZE);
    stm32_sync_frame_t *frame = (stm32_sync_frame_t *)tx_buf;
    frame->magic        = STM32_SYNC_MAGIC;
    frame->stm32_now_us = now_us();
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: arm the counter alarm for the next pending packet
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Safe to call from both thread context (scheduler_thread) and ISR context
 * (injection_alarm_cb).  The atomic_cas on alarm_armed ensures the alarm is
 * set at most once even if both callers race.
 *
 * Forward declaration needed because injection_alarm_cb (defined below) and
 * try_arm_counter both reference each other.
 */
static void injection_alarm_cb(const struct device *dev, uint8_t chan_id,
                               uint32_t ticks, void *user_data);

static void try_arm_counter(void)
{
    /* Peek at the next packet without consuming it. */
    device_update_packet_t next;
    if (k_msgq_peek(&scheduler_q, &next) != 0) {
        /* Queue is empty — nothing to arm for. */
        return;
    }

    /* CAS: only the first caller wins the right to arm the alarm. */
    if (!atomic_cas(&alarm_armed, 0, 1)) {
        return;  /* already armed by a concurrent caller */
    }

    int32_t delay_us = (int32_t)(next.timestamp_us - now_us());
    if (delay_us < 1) {
        delay_us = 1;  /* clamp: fire on the very next counter tick */
    }

    /*
     * counter_us_to_ticks converts a microsecond delay to counter ticks
     * at the device's native frequency (e.g. 1 MHz for TIM6 at 168 MHz
     * with a /168 prescaler).  The result is a RELATIVE offset from now —
     * no COUNTER_ALARM_CFG_ABSOLUTE flag is set.
     */
    injection_alarm_cfg.ticks     = counter_us_to_ticks(counter_dev,
                                                         (uint64_t)delay_us);
    injection_alarm_cfg.callback  = injection_alarm_cb;
    injection_alarm_cfg.user_data = NULL;
    injection_alarm_cfg.flags     = 0;  /* relative mode */

    int err = counter_set_channel_alarm(counter_dev, 0, &injection_alarm_cfg);
    if (err != 0) {
        LOG_ERR("counter_set_channel_alarm failed: %d", err);
        atomic_set(&alarm_armed, 0);  /* release so the next call can retry */
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Counter alarm callback — "The Timekeeper"  (ISR context)
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Fires when the STM32 hardware counter has advanced by the delay computed
 * in try_arm_counter(), meaning the real-world clock has reached (or passed)
 * the packet's timestamp_us.  The packet is now causally safe to inject.
 *
 * Sequence:
 *   1. Clear armed flag so try_arm_counter() can re-arm after us.
 *   2. Pop the due packet from the staging ring.
 *   3. Atomically overwrite the 1-slot sensor_update_q gate:
 *        irq_lock → purge → put → irq_unlock
 *   4. Immediately arm the counter for the next pending packet.
 *
 * Constraints: ISR context.  No blocking, no sleeping, no mutex operations.
 * All k_msgq operations used here are on Zephyr's ISR-approved API list.
 */
static void injection_alarm_cb(const struct device *dev, uint8_t chan_id,
                               uint32_t ticks, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(chan_id);
    ARG_UNUSED(ticks);
    ARG_UNUSED(user_data);

    /* Release the armed flag before try_arm_counter so re-arming can proceed. */
    atomic_set(&alarm_armed, 0);

    device_update_packet_t pkt;

    /* Pop the packet whose physical time has arrived. */
    if (k_msgq_get(&scheduler_q, &pkt, K_NO_WAIT) != 0) {
        /* scheduler_q drained unexpectedly — nothing to inject. */
        return;
    }

    /* ── Atomic gate update ───────────────────────────────────────────── */
    /*
     * irq_lock on Cortex-M sets BASEPRI (or PRIMASK), masking all lower-
     * priority configurable IRQs.  The purge→put sequence must be atomic
     * against any other ISR that could race on sensor_update_q.
     *
     * Duration: k_msgq_purge + k_msgq_put on a 1-slot queue ≈ 50–150 CPU
     * cycles at 168 MHz ≈ 0.3–0.9 µs.  This is within I2C clock-stretch
     * tolerance at 400 kHz (one bit period = 2.5 µs).  See ANALYSIS.md W3.
     */
    unsigned int irq_key = irq_lock();
    k_msgq_purge(&sensor_update_q);
    (void)k_msgq_put(&sensor_update_q, &pkt, K_NO_WAIT);
    irq_unlock(irq_key);

    /* Immediately arm the counter for the next pending packet, if any. */
    try_arm_counter();
}

/* ══════════════════════════════════════════════════════════════════════════
 * SPI async callback  (ISR context)
 * ══════════════════════════════════════════════════════════════════════════ */
static void spi_rx_callback(const struct device *dev, int result, void *userdata)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(userdata);

    if (result != 0) {
        LOG_ERR("SPI DMA error: %d", result);
        /* Still give the semaphore so the scheduler thread can re-arm. */
    }
    k_sem_give(&spi_rx_sem);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: push packet to scheduler_q with oldest-eviction semantics
 * ══════════════════════════════════════════════════════════════════════════ */
static void scheduler_q_put_overwrite(const device_update_packet_t *pkt)
{
    if (k_msgq_put(&scheduler_q, pkt, K_NO_WAIT) == 0) {
        return;  /* fast path: space available */
    }

    /*
     * Queue full — evict the oldest (most stale) entry and insert the newest.
     * The physics engine is deterministic; a skipped intermediate state is
     * less damaging than perpetually stale data.
     */
    device_update_packet_t dropped;
    if (k_msgq_get(&scheduler_q, &dropped, K_NO_WAIT) == 0) {
        LOG_WRN("scheduler_q full: evicting ts=%u, inserting ts=%u",
                dropped.timestamp_us, pkt->timestamp_us);
        (void)k_msgq_put(&scheduler_q, pkt, K_NO_WAIT);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: arm one full-duplex SPI transfer
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Always populates dma_tx_buf with a fresh sync frame immediately before
 * calling spi_transceive_async, so stm32_now_us reflects the actual DMA-arm
 * moment with nanosecond accuracy.
 *
 * Returns 0 on success, negative errno on failure.
 */
static int arm_spi_transfer(void)
{
    /* Stamp the TX buffer with the current STM32 clock. */
    populate_sync_frame(dma_tx_buf);

    spi_tx_desc.buf = dma_tx_buf;
    spi_tx_desc.len = SPI_BUF_SIZE;
    spi_rx_desc.buf = dma_rx_buf;
    spi_rx_desc.len = SPI_BUF_SIZE;

    return spi_transceive_async(spi_dev, &spi_cfg,
                                &spi_tx_set, &spi_rx_set,
                                spi_rx_callback, NULL);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: swap RX and TX buffer pairs after a DMA completion
 * ══════════════════════════════════════════════════════════════════════════ */
static void swap_buffers(void)
{
    uint8_t *tmp;

    /* RX: decode_rx_buf ← dma_rx_buf (just filled), dma_rx_buf now free */
    tmp          = dma_rx_buf;
    dma_rx_buf   = decode_rx_buf;
    decode_rx_buf = tmp;

    /* TX: dma_tx_buf ← prep_tx_buf (to be pre-loaded), old dma_tx_buf free */
    tmp         = dma_tx_buf;
    dma_tx_buf  = prep_tx_buf;
    prep_tx_buf = tmp;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: parse and validate the 2-byte SPI length prefix
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Returns the payload length on success, 0 if the frame is invalid.
 */
static uint16_t parse_frame_length(const uint8_t *buf)
{
    uint16_t len = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);

    if (len == 0 || len > MAX_PROTO_PAYLOAD) {
        LOG_WRN("Invalid SPI frame length: %u (max %u)", len, MAX_PROTO_PAYLOAD);
        return 0;
    }
    return len;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Helper: map a decoded SensorPacket proto → device_update_packet_t
 * ══════════════════════════════════════════════════════════════════════════
 *
 * ADAPT the field names below to your actual .proto schema.
 * The split below assumes the proto has:
 *   uint32 timestamp_us
 *   bool   has_sync_confirmed   ← set by Pi once epoch offset is stable
 *   bool   has_imu
 *   ImuPayload imu { int32 gyro_x/y/z, euler_x/y/z, lin_acc_x/y/z }
 *   uint32 lidar_mm  (0 if not present this tick)
 *
 * Values are Q4 fixed-point signed integers matching BNO055 register format.
 */
static void proto_to_packet(const SensorPacket *proto,
                            device_update_packet_t *pkt)
{
    memset(pkt, 0, sizeof(*pkt));
    pkt->timestamp_us = proto->timestamp_us;

    if (proto->has_imu) {
        pkt->sensor_id = IMU_DEVICE_ID;

        /* Gyro 0x14–0x19 */
        pkt->imu_data.gyro.x.lsb = (int8_t)(proto->imu.gyro_x        & 0xFF);
        pkt->imu_data.gyro.x.msb = (int8_t)((proto->imu.gyro_x >> 8) & 0xFF);
        pkt->imu_data.gyro.y.lsb = (int8_t)(proto->imu.gyro_y        & 0xFF);
        pkt->imu_data.gyro.y.msb = (int8_t)((proto->imu.gyro_y >> 8) & 0xFF);
        pkt->imu_data.gyro.z.lsb = (int8_t)(proto->imu.gyro_z        & 0xFF);
        pkt->imu_data.gyro.z.msb = (int8_t)((proto->imu.gyro_z >> 8) & 0xFF);

        /* Euler 0x1A–0x1F */
        pkt->imu_data.euler_angles.x.lsb = (int8_t)(proto->imu.euler_x        & 0xFF);
        pkt->imu_data.euler_angles.x.msb = (int8_t)((proto->imu.euler_x >> 8) & 0xFF);
        pkt->imu_data.euler_angles.y.lsb = (int8_t)(proto->imu.euler_y        & 0xFF);
        pkt->imu_data.euler_angles.y.msb = (int8_t)((proto->imu.euler_y >> 8) & 0xFF);
        pkt->imu_data.euler_angles.z.lsb = (int8_t)(proto->imu.euler_z        & 0xFF);
        pkt->imu_data.euler_angles.z.msb = (int8_t)((proto->imu.euler_z >> 8) & 0xFF);

        /* Linear accel 0x28–0x2D */
        pkt->imu_data.linear_acceleration.x.lsb = (int8_t)(proto->imu.lin_acc_x        & 0xFF);
        pkt->imu_data.linear_acceleration.x.msb = (int8_t)((proto->imu.lin_acc_x >> 8) & 0xFF);
        pkt->imu_data.linear_acceleration.y.lsb = (int8_t)(proto->imu.lin_acc_y        & 0xFF);
        pkt->imu_data.linear_acceleration.y.msb = (int8_t)((proto->imu.lin_acc_y >> 8) & 0xFF);
        pkt->imu_data.linear_acceleration.z.lsb = (int8_t)(proto->imu.lin_acc_z        & 0xFF);
        pkt->imu_data.linear_acceleration.z.msb = (int8_t)((proto->imu.lin_acc_z >> 8) & 0xFF);
    } else {
        pkt->sensor_id        = LIDAR_DEVICE_ID;
        pkt->lidar_distance_mm = (uint16_t)(proto->lidar_mm & 0xFFFFU);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Scheduler thread
 * ══════════════════════════════════════════════════════════════════════════ */
K_THREAD_STACK_DEFINE(scheduler_stack, SCHEDULER_STACK_SIZE);
static struct k_thread scheduler_data;

static void scheduler_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    /* ── Pre-flight checks ──────────────────────────────────────────────── */
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI device not ready — scheduler cannot start");
        return;
    }
    if (!device_is_ready(counter_dev)) {
        LOG_ERR("Injection counter device not ready — scheduler cannot start");
        return;
    }

    /* Start the free-running counter.  It begins counting from 0 and wraps
     * at counter_get_top_value().  The alarm is always relative (no absolute
     * flag), so wrap does not affect correctness. */
    if (counter_start(counter_dev) != 0) {
        LOG_ERR("Failed to start injection counter");
        return;
    }

    LOG_INF("Scheduler thread started.");
    LOG_INF("  SPI: full-duplex double-buffer, %u-byte frames (%u payload)",
            SPI_BUF_SIZE, MAX_PROTO_PAYLOAD);
    LOG_INF("  Staging queue: %u slots", SCHEDULER_Q_LEN);
    LOG_INF("  Waiting for clock-sync handshake from Pi...");

    /* ════════════════════════════════════════════════════════════════════
     * Main receive-decode loop
     *
     * Iteration structure (double-buffer pipeline):
     *
     *   ┌─[arm A]──────────────────────────────────────────────────────┐
     *   │  1. arm_spi_transfer()          ← uses dma_rx=A, dma_tx=A'  │
     *   │  2. sem_take                    ← A filled, A' sent          │
     *   │  3. swap_buffers()              ← decode=A, dma=B, prep=A'  │
     *   │  4. arm_spi_transfer()          ← re-arm: uses dma=B, tx=A' │
     *   │     (closes re-arm gap)                                      │
     *   │  5. parse length prefix from decode(=A)                      │
     *   │  6. pb_decode from decode+2                                  │
     *   │  7. check sync / enqueue / arm counter                       │
     *   │  8. sem_take                    ← B filled (from step 4)     │
     *   │  9. swap_buffers()              ← decode=B, dma=A, prep=B'  │
     *   └──────────────────────────── [loop: arm A again] ─────────────┘
     * ════════════════════════════════════════════════════════════════ */

    while (1) {

        /* ── Step 1: Arm first DMA transfer ─────────────────────────── */
        int err = arm_spi_transfer();
        if (err != 0) {
            LOG_ERR("spi_transceive_async failed: %d — retrying in 1 ms", err);
            k_sleep(K_MSEC(1));
            continue;
        }

        /* ── Step 2: Wait for DMA completion ────────────────────────── */
        k_sem_take(&spi_rx_sem, K_FOREVER);

        /* ── Step 3: Swap buffers ────────────────────────────────────── */
        /*
         * After swap:
         *   decode_rx_buf → holds the newly received MOSI data
         *   dma_rx_buf    → free, will be filled by the next DMA
         *   dma_tx_buf    → will be populated with next sync frame
         *   prep_tx_buf   → old dma_tx_buf, safe to reuse
         */
        swap_buffers();

        /* ── Step 4: Immediate re-arm (closes re-arm gap) ────────────── */
        /*
         * CRITICAL: arm the next DMA transfer BEFORE decoding the current
         * frame.  This shrinks the dead-band to ~pointer-swap-time (~ns).
         * arm_spi_transfer stamps dma_tx_buf with now_us() at this moment —
         * accurate for the upcoming Pi transaction, not the current one.
         */
        err = arm_spi_transfer();
        if (err != 0) {
            LOG_ERR("spi_transceive_async re-arm failed: %d", err);
            /* Fall through — decode the current frame even if re-arm failed. */
        }

        /* ── Step 5: Parse 2-byte SPI length prefix ──────────────────── */
        uint16_t payload_len = parse_frame_length(decode_rx_buf);
        if (payload_len == 0) {
            /* Invalid frame — wait for the re-armed transfer to complete. */
            goto wait_for_rearmed_transfer;
        }

        /* ── Step 6: Protobuf decode ─────────────────────────────────── */
        SensorPacket proto = SensorPacket_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(
            decode_rx_buf + SPI_FRAME_HEADER_SIZE, payload_len);

        if (!pb_decode(&stream, SensorPacket_fields, &proto)) {
            LOG_WRN("pb_decode failed: %s", PB_GET_ERROR(&stream));
            goto wait_for_rearmed_transfer;
        }

        /* ── Step 7a: Clock-sync handshake gate ──────────────────────── */
        /*
         * The Pi drives the sync handshake by setting has_sync_confirmed
         * once it has computed a stable pi_epoch_offset from the MISO sync
         * frames.  Until that flag arrives, all physics packets are silently
         * discarded.  The MISO sync frames continue uninterrupted — the Pi
         * can keep refining its offset estimate across multiple frames before
         * sending its first real data packet.
         */
        if (!atomic_get(&sync_confirmed)) {
            if (proto.has_sync_confirmed && proto.sync_confirmed) {
                atomic_set(&sync_confirmed, 1);
                LOG_INF("Clock-sync handshake complete. Streaming begins.");
            } else {
                /* Still waiting — keep broadcasting sync frames on MISO. */
                goto wait_for_rearmed_transfer;
            }
        }

        /* ── Step 7b: Map proto → internal packet ────────────────────── */
        device_update_packet_t pkt;
        proto_to_packet(&proto, &pkt);

        /* ── Step 7c: Enqueue and arm counter ────────────────────────── */
        scheduler_q_put_overwrite(&pkt);
        try_arm_counter();

wait_for_rearmed_transfer:
        /* ── Step 8: Wait for the second DMA completion (from step 4) ── */
        /*
         * The re-arm in step 4 fired concurrently with decode (steps 5–7).
         * Block here for that transfer's completion before looping.
         *
         * If step 4's arm_spi_transfer() failed, spi_rx_sem will never be
         * given.  In production, add a timeout and error-recovery path here.
         */
        k_sem_take(&spi_rx_sem, K_FOREVER);

        /* ── Step 9: Final swap — restore buffer invariant for next iter */
        swap_buffers();

    } /* while (1) */
}

/* ══════════════════════════════════════════════════════════════════════════
 * Public initialisation
 * ══════════════════════════════════════════════════════════════════════════ */
void scheduler_init(void)
{
    k_thread_create(&scheduler_data,
                    scheduler_stack,
                    K_THREAD_STACK_SIZEOF(scheduler_stack),
                    scheduler_thread,
                    NULL, NULL, NULL,
                    SCHEDULER_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&scheduler_data, "scheduler");
    LOG_INF("Scheduler thread created at priority %d", SCHEDULER_PRIORITY);
}
