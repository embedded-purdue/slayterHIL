/**
 * @file scheduler.c
 * @brief Scheduler thread: SPI receive, protobuf decode, and timer-based
 *        causality gating of physics engine updates.
 *
 * SPI approach — synchronous spi_read() in a dedicated thread
 * ────────────────────────────────────────────────────────────
 * The STM32 SPI driver (spi_ll_stm32.c) does not reliably support async
 * transfers in slave mode — it uses a 1-second timeout internally and
 * reconfigures the peripheral on every transaction, making spi_read_signal /
 * spi_read_async unsuitable.
 *
 * Instead we call blocking spi_read() from a dedicated Zephyr thread.
 * While this thread sleeps waiting for the Pi to clock in a frame, Zephyr's
 * scheduler runs all other threads (sensor emulation, I2C ISRs, timer ISR)
 * normally.  There is no system-wide block.
 *
 * The re-arm gap (window where a Pi frame can be missed) equals the protobuf
 * decode time ≈ 15–30 µs at 240 MHz.  At a Pi send rate of 20–100 Hz this
 * is a 0.03–0.3% miss window — acceptable for HIL.
 *
 * Packet model — combined frame
 * ──────────────────────────────
 * Every SPI frame contains IMU + LiDAR + RC together.  There is no sensor_id
 * tag and no union.  All fields in device_update_packet_t are always valid.
 *
 * Wire frame format (2-byte LE length prefix)
 * ────────────────────────────────────────────
 *   [LEN_LOW][LEN_HIGH][protobuf payload ... ][zero-pad to 256 bytes]
 *   Max payload = 254 bytes.
 *
 * Clock sync
 * ──────────
 * packet.timestamp_us is in the STM32's k_cyc_to_us_near32 epoch.
 * main.c runs the SPI clock-sync handshake before this thread starts.
 * Set CONFIG_HIL_SKIP_CLOCK_SYNC=y in prj.conf to skip during bring-up.
 *
 * Required prj.conf options
 * ─────────────────────────
 *   CONFIG_SPI=y
 *   CONFIG_SPI_SLAVE=y
 *   CONFIG_LOG=y
 *   CONFIG_LOG_MODE_DEFERRED=y   (needed for LOG_DBG inside timer ISR)
 */

#include "threads/scheduler_test.h"
#include "threads/sensor_emulation_test.h"
#include "hil_diag.h"

#include <sensor_data.pb.h>
#include <pb_decode.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>        /* CLAMP */
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(scheduler, LOG_LEVEL_DBG);

/* ── Device tree ──────────────────────────────────────────────────────────── */
#define SPI_DEV_NODE  DT_ALIAS(spi_orchestrator)
static const struct device *const spi_dev = DEVICE_DT_GET(SPI_DEV_NODE);

/* ── SPI buffer ───────────────────────────────────────────────────────────── */
/*
 * Single static buffer — no double-buffering needed with synchronous spi_read.
 * spi_read() returns only after the Pi has clocked in a complete 256-byte
 * frame, so the buffer is safe to decode from the moment the call returns.
 */
#define SPI_BUF_SIZE        256U
#define SPI_FRAME_HDR_SIZE  2U                          /* 2-byte LE length prefix */
#define SPI_MAX_PAYLOAD     (SPI_BUF_SIZE - SPI_FRAME_HDR_SIZE)  /* 254 bytes */

static uint8_t rx_buf[SPI_BUF_SIZE];

static struct spi_buf     spi_rx_desc = { .buf = rx_buf, .len = SPI_BUF_SIZE };
static struct spi_buf_set spi_rx_set  = { .buffers = &spi_rx_desc, .count = 1 };

/*
 * SPI slave configuration.
 * frequency is ignored in slave mode on most STM32 drivers — the master
 * (Pi) controls the clock.  It is retained here for documentation and in
 * case a future driver version validates it.
 */
static const struct spi_config spi_cfg = {
    .frequency = 10000000U,
    .operation = SPI_OP_MODE_SLAVE | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
};

/* ── Staging ring buffer ──────────────────────────────────────────────────── */
/*
 * N-slot ring buffer between the scheduler thread and the injection timer ISR.
 * When full, the OLDEST packet is evicted so the ISR always sees the most
 * recent physics state rather than running perpetually behind the Pi.
 */
K_MSGQ_DEFINE(scheduler_q, sizeof(device_update_packet_t), SCHEDULER_Q_LEN, 4);

/* ── Injection timer and armed flag ───────────────────────────────────────── */
static struct k_timer injection_timer;

/*
 * atomic_t flag: 0 = timer idle, 1 = timer running.
 * Used by try_arm_timer() to prevent a race where both the scheduler thread
 * and the timer ISR attempt to start the timer simultaneously.
 * atomic_cas (compare-and-swap) ensures only one caller succeeds.
 */
static atomic_t timer_armed = ATOMIC_INIT(0);

atomic_t spi_err_count = ATOMIC_INIT(0);


/* ── Time helper ──────────────────────────────────────────────────────────── */
static inline uint32_t now_us(void)
{
    return k_cyc_to_us_near32(k_cycle_get_32());
}

/* ── Injection timer ISR ──────────────────────────────────────────────────── */
/**
 * Fires when the STM32 hardware clock has caught up to a packet's
 * timestamp_us — meaning that packet is no longer "in the future" and is
 * causally safe to serve to the DUT.
 *
 * Sequence:
 *   1. Pop the due packet from the staging ring.
 *   2. irq_lock  — make purge+put atomic against any peer ISR.
 *   3. Purge the 1-slot gate (sensor_update_q) — evict the stale entry.
 *   4. Put the fresh packet.
 *   5. irq_unlock.
 *   6. Peek at the next packet and re-arm for its timestamp.
 *
 * Context: k_timer expiry — executes in SysTick ISR.
 * Constraints: no blocking, no mutexes, no sleeping.
 */
static void injection_timer_isr(struct k_timer *timer_id)
{
    ARG_UNUSED(timer_id);

    /* Clear the armed flag; we re-set it below if we re-arm. */
    atomic_set(&timer_armed, 0);

    device_update_packet_t pkt;
    if (k_msgq_get(&scheduler_q, &pkt, K_NO_WAIT) != 0) {
        /* Ring drained unexpectedly — nothing to inject. */
        return;
    }

    /* ── Atomic gate update ─────────────────────────────────────────────── */
    /*
     * irq_lock sets PRIMASK=1 on Cortex-M, masking all configurable
     * interrupts.  The purge→put window is O(1) ≈ 10–20 CPU cycles
     * (< 100 ns at 240 MHz).  Any pending I2C ISR is deferred by this
     * amount, which is well within I2C clock-stretch tolerance.
     */
    unsigned int irq_key = irq_lock();
    k_msgq_purge(&sensor_update_q);
    (void)k_msgq_put(&sensor_update_q, &pkt, K_NO_WAIT);
    irq_unlock(irq_key);

    /* Update diagnostics (atomic — ISR-safe) */
    hil_diag_inc_timer_inject();
    hil_diag_set_last_inject_ts(pkt.timestamp_us);

    /* Reconstruct euler_x int16 from the LSB/MSB pair for the diag display. */
    int16_t ex = (int16_t)(
        (uint16_t)pkt.imu_data.euler_angles.x.lsb |
        ((uint16_t)pkt.imu_data.euler_angles.x.msb << 8));
    hil_diag_set_last_euler_x((int32_t)ex);

    /*
     * Deferred LOG is safe inside a k_timer ISR when CONFIG_LOG_MODE_DEFERRED=y.
     * The message is written to a ring buffer; the logging thread flushes it.
     */
    LOG_DBG("[INJECT @%u µs] lidar=%u mm  euler_x=%d  rc=(%d,%d)",
            pkt.timestamp_us,
            pkt.lidar_distance_mm,
            (int)ex,
            pkt.rc_commands.rc_vert,
            pkt.rc_commands.rc_horiz);

    /* Schedule the timer for the next pending packet. */
    device_update_packet_t next;
    if (k_msgq_peek(&scheduler_q, &next) == 0) {
        int32_t delay = (int32_t)(next.timestamp_us - now_us());
        if (delay < 1) {
            delay = 1;  /* Clamp — prevents uint32 underflow wrap */
        }
        k_timer_start(&injection_timer, K_USEC(delay), K_NO_WAIT);
        atomic_set(&timer_armed, 1);
    }
    /* If the ring is empty, the timer stays idle until the scheduler thread
     * pushes the next packet and calls try_arm_timer(). */
}

/* ── Timer arm helper (scheduler thread context) ──────────────────────────── */
/**
 * Arms the injection timer for the oldest pending packet in scheduler_q.
 * Uses atomic_cas to ensure only one caller (thread or ISR) starts the timer.
 * If the timer is already armed, this is a no-op.
 */
static void try_arm_timer(void)
{
    /* CAS: only proceed if timer_armed is 0 — atomically set it to 1. */
    if (!atomic_cas(&timer_armed, 0, 1)) {
        return;   /* Already armed by ISR or a prior call. */
    }

    device_update_packet_t peek;
    if (k_msgq_peek(&scheduler_q, &peek) != 0) {
        /* Queue is empty — undo the flag we just set. */
        atomic_set(&timer_armed, 0);
        return;
    }

    int32_t delay = (int32_t)(peek.timestamp_us - now_us());
    if (delay < 1) {
        delay = 1;
    }
    k_timer_start(&injection_timer, K_USEC(delay), K_NO_WAIT);
    LOG_DBG("[TIMER ARM] next_ts=%u  delay=%d µs", peek.timestamp_us, delay);
}

/* ── scheduler_q put with oldest-eviction ────────────────────────────────── */
/**
 * Inserts pkt into scheduler_q.  If the queue is full, evicts the oldest
 * entry (the one furthest in the past) and inserts the newest.  This keeps
 * the ring current rather than perpetually stale.
 */
static void scheduler_q_put_overwrite(const device_update_packet_t *pkt)
{
    if (k_msgq_put(&scheduler_q, pkt, K_NO_WAIT) == 0) {
        return;   /* Fast path — queue had space. */
    }

    device_update_packet_t dropped;
    if (k_msgq_get(&scheduler_q, &dropped, K_NO_WAIT) == 0) {
        LOG_WRN("[EVICT] dropped ts=%u  inserting ts=%u",
                dropped.timestamp_us, pkt->timestamp_us);
        hil_diag_inc_scheduler_q_evict();
        (void)k_msgq_put(&scheduler_q, pkt, K_NO_WAIT);
    }
}

/* ── Framing helpers ──────────────────────────────────────────────────────── */

/**
 * @brief Parse the 2-byte LE length prefix from a raw frame buffer.
 * @return Payload length in bytes, or 0 if the header is invalid.
 */
static uint16_t frame_parse_len(const uint8_t *buf)
{
    uint16_t len = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    if (len == 0 || len > SPI_MAX_PAYLOAD) {
        return 0;
    }
    return len;
}

/**
 * @brief Return true if buf contains the Pi's clock-sync magic frame.
 *
 * Sync frame layout:
 *   bytes [0..1] = 0xFFFF  (marker)
 *   bytes [2..5] = 0xC0FFEEEE  (little-endian magic)
 */
static bool frame_is_sync(const uint8_t *buf)
{
    uint16_t marker = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    uint32_t magic  = (uint32_t)buf[2]
                    | ((uint32_t)buf[3] << 8)
                    | ((uint32_t)buf[4] << 16)
                    | ((uint32_t)buf[5] << 24);
    return (marker == 0xFFFFU && magic == 0xC0FFEEEEUL);
}

/* ── Scheduler thread ─────────────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(scheduler_stack, SCHEDULER_STACK_SIZE);
static struct k_thread scheduler_data;

static void scheduler_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI device not ready — scheduler thread exiting");
        return;
    }

    k_timer_init(&injection_timer, injection_timer_isr, NULL);

    LOG_INF("Scheduler running  buf=%u B  max_payload=%u B  ring=%u slots",
            SPI_BUF_SIZE, SPI_MAX_PAYLOAD, SCHEDULER_Q_LEN);

    uint32_t loop = 0;

    while (1) {
        /* ── Step 1: Block until the Pi clocks in a full 256-byte frame ─────
         *
         * spi_read() suspends this thread at the Zephyr scheduler level.
         * All other threads — sensor emulation, I2C ISRs, timer ISR — run
         * normally while we wait.  There is no busy-wait and no CPU burn.
         *
         * The STM32 SPI slave hardware latches the NSS pin and fills the
         * RX FIFO under DMA as the Pi clocks bytes.  spi_read() returns
         * after exactly SPI_BUF_SIZE bytes have been received.
         */
        int err = spi_read(spi_dev, &spi_cfg, &spi_rx_set);
        /* if (err != 0) { */
        /*     LOG_ERR("[#%u] spi_read error: %d — retrying in 1 ms", loop, err); */
        /*     k_sleep(K_MSEC(1)); */
        /*     continue; */
        /* } */


        if (err !=0) {
            atomic_inc(&spi_err_count);
            k_sleep(K_MSEC(1));
            continue;
        }

        loop++;

        /* ── Step 2: Reject clock-sync frames ───────────────────────────── */
        /*
         * The Pi sends a sync frame as its very first transmission.
         * main.c handles the two-way handshake before starting this thread,
         * but a re-sync or a late-arriving sync frame is silently ignored here.
         */
        if (frame_is_sync(rx_buf)) {
            LOG_INF("[#%u] Sync frame received — ignoring in stream mode", loop);
            continue;
        }

        /* ── Step 3: Parse 2-byte LE length prefix ───────────────────────── */
        uint16_t payload_len = frame_parse_len(rx_buf);
        if (payload_len == 0) {
            LOG_WRN("[#%u] Invalid frame header [%02X %02X] — skipping",
                    loop, rx_buf[0], rx_buf[1]);
            hil_diag_inc_decode_fail();
            continue;
        }

        /* ── Step 4: Decode protobuf payload ─────────────────────────────── */
        SensorPacket proto = SensorPacket_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(
            rx_buf + SPI_FRAME_HDR_SIZE, payload_len);

        if (!pb_decode(&stream, SensorPacket_fields, &proto)) {
            LOG_WRN("[#%u] pb_decode failed (plen=%u): %s",
                    loop, payload_len, PB_GET_ERROR(&stream));
            hil_diag_inc_decode_fail();
            continue;
        }

        hil_diag_inc_spi_rx();

        /* ── Step 5: Log the decoded combined packet ─────────────────────── */
        LOG_INF("[RX #%u] ts=%6u µs | "
                "euler(%d,%d,%d)  gyro(%d,%d,%d)  la(%d,%d,%d) | "
                "lidar=%u mm | rc(%d,%d)",
                loop,
                proto.timestamp_us,
                proto.imu.euler_x,    proto.imu.euler_y,    proto.imu.euler_z,
                proto.imu.gyro_x,     proto.imu.gyro_y,     proto.imu.gyro_z,
                proto.imu.lin_acc_x,  proto.imu.lin_acc_y,  proto.imu.lin_acc_z,
                proto.lidar_mm,
                proto.rc.vertical,    proto.rc.horizontal);

        hil_diag_set_last_lidar((int32_t)proto.lidar_mm);
        hil_diag_set_last_euler_x(proto.imu.euler_x);

        /* ── Step 6: Map proto fields → device_update_packet_t ───────────── */
        /*
         * Combined packet — IMU + LiDAR + RC all present in every frame.
         * No sensor_id, no union, no has_imu flag.
         */
        device_update_packet_t pkt;
        memset(&pkt, 0, sizeof(pkt));

        pkt.timestamp_us      = proto.timestamp_us;
        pkt.lidar_distance_mm = (uint16_t)(proto.lidar_mm & 0xFFFFU);

        /* RC — clamp proto sint32 → int8 to guard against out-of-range values */
        pkt.rc_commands.rc_vert  = (int8_t)CLAMP(proto.rc.vertical,   -127, 127);
        pkt.rc_commands.rc_horiz = (int8_t)CLAMP(proto.rc.horizontal, -127, 127);

        /* ── IMU field mapping ──────────────────────────────────────────────
         *
         * Each proto sint32 value is a signed 16-bit integer in BNO055
         * register format.  We split it into the two consecutive register
         * bytes that the I2C ISR will serve directly to the DUT.
         *
         * Bit operations:
         *   lsb = value & 0xFF         (lower 8 bits)
         *   msb = (value >> 8) & 0xFF  (upper 8 bits, sign-extended by >>)
         *
         * Casting to int8_t reinterprets the bit pattern without UB because
         * we have already masked to 8 bits.
         */

        /* Euler angles — BNO055 regs 0x1A–0x1F */
        pkt.imu_data.euler_angles.x.lsb = (int8_t)( proto.imu.euler_x        & 0xFF);
        pkt.imu_data.euler_angles.x.msb = (int8_t)((proto.imu.euler_x  >> 8) & 0xFF);
        pkt.imu_data.euler_angles.y.lsb = (int8_t)( proto.imu.euler_y        & 0xFF);
        pkt.imu_data.euler_angles.y.msb = (int8_t)((proto.imu.euler_y  >> 8) & 0xFF);
        pkt.imu_data.euler_angles.z.lsb = (int8_t)( proto.imu.euler_z        & 0xFF);
        pkt.imu_data.euler_angles.z.msb = (int8_t)((proto.imu.euler_z  >> 8) & 0xFF);

        /* Linear acceleration — BNO055 regs 0x28–0x2D */
        pkt.imu_data.linear_acceleration.x.lsb = (int8_t)( proto.imu.lin_acc_x        & 0xFF);
        pkt.imu_data.linear_acceleration.x.msb = (int8_t)((proto.imu.lin_acc_x  >> 8) & 0xFF);
        pkt.imu_data.linear_acceleration.y.lsb = (int8_t)( proto.imu.lin_acc_y        & 0xFF);
        pkt.imu_data.linear_acceleration.y.msb = (int8_t)((proto.imu.lin_acc_y  >> 8) & 0xFF);
        pkt.imu_data.linear_acceleration.z.lsb = (int8_t)( proto.imu.lin_acc_z        & 0xFF);
        pkt.imu_data.linear_acceleration.z.msb = (int8_t)((proto.imu.lin_acc_z  >> 8) & 0xFF);

        /* Gyroscope — BNO055 regs 0x14–0x19 */
        pkt.imu_data.gyro.x.lsb = (int8_t)( proto.imu.gyro_x        & 0xFF);
        pkt.imu_data.gyro.x.msb = (int8_t)((proto.imu.gyro_x  >> 8) & 0xFF);
        pkt.imu_data.gyro.y.lsb = (int8_t)( proto.imu.gyro_y        & 0xFF);
        pkt.imu_data.gyro.y.msb = (int8_t)((proto.imu.gyro_y  >> 8) & 0xFF);
        pkt.imu_data.gyro.z.lsb = (int8_t)( proto.imu.gyro_z        & 0xFF);
        pkt.imu_data.gyro.z.msb = (int8_t)((proto.imu.gyro_z  >> 8) & 0xFF);

        /* ── Step 7: Push to staging ring ────────────────────────────────── */
        scheduler_q_put_overwrite(&pkt);

        /* ── Step 8: Arm the injection timer if it is currently idle ─────── */
        try_arm_timer();

        /*
         * Loop back to spi_read() immediately.  The only gap between the end
         * of decode and the start of the next spi_read() is steps 6–8 above
         * (memset + field assignments + queue push + CAS) ≈ 15–30 µs.
         * At Pi send rates ≤ 1 kHz this causes negligible frame loss.
         */
    }
}

/* ── Public initialisation ────────────────────────────────────────────────── */
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
    LOG_INF("Scheduler thread created (priority %d)", SCHEDULER_PRIORITY);
}
