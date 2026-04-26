/**
 * @file main.c
 * @brief HIL Bridge — Nucleo STM32 entry point.
 *
 * Boot sequence
 * ─────────────
 *   1. Verify all required devices are ready.
 *   2. Perform the SPI clock-sync handshake with the Pi.
 *      The Pi sends a special SYNC frame; the STM32 records its own now_us()
 *      at the moment it receives it.  All subsequent packet timestamps are
 *      relative to that moment.
 *   3. Start sensor_emulation subsystem (registers I2C targets, starts thread).
 *   4. Start scheduler subsystem (starts SPI DMA loop + injection timer).
 *   5. Run diagnostic thread forever, printing stats every 2 s.
 *
 * ── CLOCK SYNC PROTOCOL ──────────────────────────────────────────────────────
 *
 *   Pi sends first (SYNC marker):
 *     [0xFF][0xFF][SYNC_MAGIC_0][SYNC_MAGIC_1][SYNC_MAGIC_2][SYNC_MAGIC_3]
 *     where SYNC_MAGIC = 0xC0FFEEEE (little-endian)
 *
 *   STM32 receives the 6-byte sync frame over SPI.
 *   STM32 records epoch_stm32_us = now_us() at reception.
 *   STM32 sends back 4 bytes = epoch_stm32_us (little-endian) on the next
 *   Pi-clocked SPI transaction.  (The Pi must initiate a 4-byte read.)
 *
 *   Pi receives the 4 bytes, computes:
 *     epoch_offset_us = epoch_stm32_us - pi_monotonic_us_at_sync
 *   Then all subsequent timestamp_us fields in SensorPacket are:
 *     (pi_sim_elapsed_us + epoch_stm32_us_at_sync)
 *
 *   For the test harness (pi_simulator.py), a simplified version is used:
 *   timestamps start at 0 and the STM32 clock-gates against its own elapsed
 *   time since main() started.  This is sufficient to validate the flow.
 */

#include "threads/sensor_emulation_test.h"
#include "threads/scheduler_test.h"
#include "hil_diag.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <string.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* ── Device handles ─────────────────────────────────────────────────────── */
/* static const struct device *i2c_lidar = DEVICE_DT_GET(DT_ALIAS(i2c_lidar)); */
/* static const struct device *i2c_imu   = DEVICE_DT_GET(DT_ALIAS(i2c_imu)); */
/* static const struct device *spi_dev   = DEVICE_DT_GET(DT_ALIAS(spi_orchestrator)); */

/* ── Clock sync ──────────────────────────────────────────────────────────── */
#define SYNC_MAGIC        0xC0FFEEEEUL
#define SYNC_FRAME_MARKER 0xFFFF

/*
 * hil_epoch_stm32_us: the STM32's now_us() value at the moment the Pi's
 * SYNC frame was received.  All packet timestamps are relative to this.
 *
 * For the simplified test harness, this is set to 0 (Pi timestamps start at
 * 0 and the delay calculation still works because both sides count up from 0
 * simultaneously).
*/

extern atomic_t spi_err_count; //from scheduler_test.c
static const struct gpio_dt_spec led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);


static uint32_t hil_epoch_stm32_us = 0;

static inline uint32_t now_us(void)
{
    return k_cyc_to_us_near32(k_cycle_get_32());
}

/**
 * @brief Perform the SPI clock-sync handshake.
 *
 * This function blocks until the Pi sends the SYNC magic frame.
 * In the test harness, the Pi sends it as the very first SPI transaction.
 *
 * For the simplified test: if SYNC_SKIP is defined, we skip the handshake
 * and use epoch=0.  Define it in prj.conf for quick bring-up without a Pi.
 */
/* static void clock_sync_handshake(void) */
/* { */
/* #ifdef CONFIG_HIL_SKIP_CLOCK_SYNC */
/*     hil_epoch_stm32_us = now_us(); */
/*     LOG_WRN("Clock sync SKIPPED (CONFIG_HIL_SKIP_CLOCK_SYNC=y)."); */
/*     LOG_WRN("Timestamps will be relative to STM32 boot."); */
/*     return; */
/* #endif */

/*     static uint8_t sync_rx_buf[8]; */
/*     static struct spi_buf sync_spi_buf      = { .buf = sync_rx_buf, .len = 8 }; */
/*     static struct spi_buf_set sync_spi_set  = { .buffers = &sync_spi_buf, .count = 1 }; */

/*     static const struct spi_config sync_cfg = { */
/*         .frequency = 10000000U, */
/*         .operation = SPI_OP_MODE_SLAVE | SPI_TRANSFER_MSB | SPI_WORD_SET(8), */
/*     }; */

/*     LOG_INF("Waiting for Pi SYNC frame..."); */

/*     /1* Spin until we receive the magic sync pattern. *1/ */
/*     while (1) { */
/*         memset(sync_rx_buf, 0, sizeof(sync_rx_buf)); */
/*         if (spi_read(spi_dev, &sync_cfg, &sync_spi_set) != 0) { */
/*             continue; */
/*         } */

/*         uint16_t marker = (uint16_t)sync_rx_buf[0] | ((uint16_t)sync_rx_buf[1] << 8); */
/*         uint32_t magic  = (uint32_t)sync_rx_buf[2] */
/*                         | ((uint32_t)sync_rx_buf[3] << 8) */
/*                         | ((uint32_t)sync_rx_buf[4] << 16) */
/*                         | ((uint32_t)sync_rx_buf[5] << 24); */

/*         if (marker == SYNC_FRAME_MARKER && magic == SYNC_MAGIC) { */
/*             hil_epoch_stm32_us = now_us(); */
/*             LOG_INF("SYNC received! STM32 epoch = %u µs", hil_epoch_stm32_us); */
/*             break; */
/*         } */
/*         LOG_DBG("Waiting for SYNC... got marker=0x%04X magic=0x%08X", */
/*                 marker, magic); */
/*     } */

/*     /1* Send our epoch back to the Pi so it can compute the offset. *1/ */
/*     static uint8_t sync_tx_buf[4]; */
/*     static struct spi_buf sync_tx_spi_buf  = { .buf = sync_tx_buf, .len = 4 }; */
/*     static struct spi_buf_set sync_tx_set  = { .buffers = &sync_tx_spi_buf, .count = 1 }; */

/*     sync_tx_buf[0] = (uint8_t)( hil_epoch_stm32_us        & 0xFF); */
/*     sync_tx_buf[1] = (uint8_t)((hil_epoch_stm32_us >>  8) & 0xFF); */
/*     sync_tx_buf[2] = (uint8_t)((hil_epoch_stm32_us >> 16) & 0xFF); */
/*     sync_tx_buf[3] = (uint8_t)((hil_epoch_stm32_us >> 24) & 0xFF); */

/*     /1* Pi will clock these 4 bytes out. *1/ */
/*     (void)spi_write(spi_dev, &sync_cfg, &sync_tx_set); */
/*     LOG_INF("Epoch sent to Pi: %u µs", hil_epoch_stm32_us); */
/* } */

/* ── Diagnostic thread ───────────────────────────────────────────────────── */
#define DIAG_STACK_SIZE  2048U
#define DIAG_PRIORITY    10     /* lowest priority: runs only when nothing else is ready */
#define DIAG_PERIOD_MS   2000

K_THREAD_STACK_DEFINE(diag_stack, DIAG_STACK_SIZE);
static struct k_thread diag_thread_data;


static void diag_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    hil_diag_snapshot_t prev = {0};
    hil_diag_snapshot_t cur  = {0};

    uint32_t prev_spi_err = 0;

    while (1) {
        k_sleep(K_MSEC(DIAG_PERIOD_MS));

        hil_diag_get_snapshot(&cur);
        uint32_t cur_spi_err = (uint32_t)atomic_get(&spi_err_count);

        /* ── Rates over last 2 s ───────────────────────────── */
        uint32_t rx_rate    = (cur.spi_rx_count      - prev.spi_rx_count)
                              * 1000 / DIAG_PERIOD_MS;
        uint32_t inj_rate   = (cur.timer_inject_count - prev.timer_inject_count)
                              * 1000 / DIAG_PERIOD_MS;
        uint32_t imu_rate   = (cur.imu_read_count     - prev.imu_read_count)
                              * 1000 / DIAG_PERIOD_MS;
        uint32_t lidar_rate = (cur.lidar_read_count   - prev.lidar_read_count)
                              * 1000 / DIAG_PERIOD_MS;
        uint32_t err_delta  = cur_spi_err - prev_spi_err;

        /* ── Single line: easy to read at speed ────────────── */
        printk("[t=%5us] SPI rx=%u/s inj=%u/s I2C imu=%u/s lidar=%u/s "
               "| fails=%u evict=%u spi_err=%u\n",
               k_uptime_get_32() / 1000,
               rx_rate, inj_rate, imu_rate, lidar_rate,
               cur.decode_fail_count,
               cur.scheduler_q_evict,
               cur_spi_err);

        /* ── LED blink pattern encodes health ──────────────── */
        /* Green = healthy: slow 1s blink                       */
        /* Problem: fast 100ms blink (SPI errors or no data)    */
        bool healthy = (rx_rate > 0) && (err_delta == 0);
        gpio_pin_set_dt(&led, 1);
        k_sleep(K_MSEC(healthy ? 900 : 80));
        gpio_pin_set_dt(&led, 0);
        /* (the k_sleep here is inside a 2s cycle so it's fine) */

        prev          = cur;
        prev_spi_err  = cur_spi_err;
    }
}


/* static void diag_thread(void *p1, void *p2, void *p3) */
/* { */
/*     ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3); */

/*     hil_diag_snapshot_t prev = {0}; */
/*     hil_diag_snapshot_t cur  = {0}; */

/*     /1* Header printed once *1/ */
/*     printk("\n"); */
/*     printk("╔══════════════════════════════════════════════════════════════╗\n"); */
/*     printk("║          HIL NUCLEO — LIVE DIAGNOSTIC (2 s interval)        ║\n"); */
/*     printk("╚══════════════════════════════════════════════════════════════╝\n"); */

/*     while (1) { */
/*         k_sleep(K_MSEC(DIAG_PERIOD_MS)); */

/*         hil_diag_get_snapshot(&cur); */

/*         uint32_t delta_spi    = cur.spi_rx_count       - prev.spi_rx_count; */
/*         uint32_t delta_dec_f  = cur.decode_fail_count  - prev.decode_fail_count; */
/*         uint32_t delta_inj    = cur.timer_inject_count  - prev.timer_inject_count; */
/*         uint32_t delta_evict  = cur.scheduler_q_evict  - prev.scheduler_q_evict; */
/*         uint32_t delta_imu    = cur.imu_read_count      - prev.imu_read_count; */
/*         uint32_t delta_lidar  = cur.lidar_read_count    - prev.lidar_read_count; */

/*         /1* Rates per second *1/ */
/*         uint32_t spi_hz   = delta_spi   * 1000 / DIAG_PERIOD_MS; */
/*         uint32_t inj_hz   = delta_inj   * 1000 / DIAG_PERIOD_MS; */
/*         uint32_t imu_hz   = delta_imu   * 1000 / DIAG_PERIOD_MS; */
/*         uint32_t lidar_hz = delta_lidar * 1000 / DIAG_PERIOD_MS; */

/*         printk("\n──────────────── t=%u ms ────────────────\n", */
/*                k_uptime_get_32()); */

/*         printk("[SPI ← Pi]   frames/s: %3u  | total: %6u  | decode_fail: %u\n", */
/*                spi_hz, cur.spi_rx_count, cur.decode_fail_count); */

/*         printk("[TIMER]      injects/s: %3u | total: %6u  | sched_q evict: %u\n", */
/*                inj_hz, cur.timer_inject_count, cur.scheduler_q_evict); */

/*         printk("[I2C → DUT]  imu/s: %3u  lidar/s: %3u  | imu_total: %u  lidar_total: %u\n", */
/*                imu_hz, lidar_hz, cur.imu_read_count, cur.lidar_read_count); */

/*         printk("[LAST INJECTED]  ts=%u µs  lidar=%d mm  euler_x=%d (1/16 deg)\n", */
/*                cur.last_inject_ts_us, */
/*                cur.last_lidar_mm, */
/*                cur.last_euler_x); */

/*         if (delta_dec_f > 0) { */
/*             printk("⚠ WARNING: %u decode failures in last interval!\n", delta_dec_f); */
/*         } */
/*         if (delta_evict > 0) { */
/*             printk("⚠ WARNING: %u scheduler_q evictions — Pi faster than timer!\n", */
/*                    delta_evict); */
/*         } */
/*         if (delta_inj == 0 && cur.spi_rx_count > 10) { */
/*             printk("⚠ WARNING: Timer not injecting! Check clock sync / timestamps.\n"); */
/*         } */
/*         if (delta_imu == 0 && delta_lidar == 0 && cur.timer_inject_count > 5) { */
/*             printk("⚠ WARNING: DUT not requesting data! Check I2C wiring.\n"); */
/*         } */

/*         prev = cur; */
/*     } */
/* } */

/* ── Entry point ─────────────────────────────────────────────────────────── */
int main(void)
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    printk("fuckoyu");
    const struct device *i2c_lidar = DEVICE_DT_GET(DT_ALIAS(i2c_lidar));
    const struct device *i2c_imu = DEVICE_DT_GET(DT_ALIAS(i2c_imu));
    const struct device *spi_dev = DEVICE_DT_GET(DT_ALIAS(spi_orchestrator));
    while(1) {
    printk("\nHIL Nucleo HIL Bridge v1.0\n");
    LOG_INF("HIL Nucleo bridge starting...");
    printk("Board: %s\n", CONFIG_BOARD);

    /* ── Device readiness checks ──────────────────────────────────────── */
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI device not ready — halting");
        return -ENODEV;
    }
    LOG_INF("SPI (orchestrator) ready");

    if (!device_is_ready(i2c_lidar)) {
        LOG_ERR("I2C LiDAR bus not ready — halting");
        return -ENODEV;
    }
    LOG_INF("I2C LiDAR bus ready");

    if (!device_is_ready(i2c_imu)) {
        LOG_ERR("I2C IMU bus not ready — halting");
        return -ENODEV;
    }
    LOG_INF("I2C IMU bus ready");

    /* ── Clock sync ─────────────────────────────────────────────────────── */
    //clock_sync_handshake();

    /* ── Subsystem init ─────────────────────────────────────────────────── */
    /*
     * sensor_emulation MUST be started before the scheduler so that the I2C
     * target callbacks are registered before the first timer injection fires.
     */
    sensor_emulation_init(i2c_lidar, i2c_imu);
    LOG_INF("Sensor emulation subsystem started");

    scheduler_init();
    LOG_INF("Scheduler subsystem started");

    /* ── Diagnostic thread ──────────────────────────────────────────────── */
    k_thread_create(&diag_thread_data,
                    diag_stack,
                    K_THREAD_STACK_SIZEOF(diag_stack),
                    diag_thread,
                    NULL, NULL, NULL,
                    DIAG_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&diag_thread_data, "hil_diag");
    LOG_INF("Diagnostic thread started (prints every %d ms)", DIAG_PERIOD_MS);

    /*
     * main() returns normally — Zephyr's main thread goes idle.
     * All work happens in the three subsystem threads + timer ISR + I2C ISRs.
     */
    LOG_INF("Init complete. Bridge is live.");
    return 0;
    }
}
