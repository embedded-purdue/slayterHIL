/**
 * @file hil_diag.h
 * @brief Lightweight diagnostic counters for the HIL bridge.
 *
 * All counters are atomic_t — safe to increment from ISR context.
 * The diagnostic thread in main.c polls them at 2 Hz and prints a summary.
 *
 * Usage in other modules:
 *   #include "hil_diag.h"
 *   hil_diag_inc_spi_rx();          // call after successful SPI decode
 *   hil_diag_inc_timer_inject();    // call in injection_timer_isr
 *   hil_diag_inc_imu_read();        // call in imu_read_requested_cb
 *   hil_diag_inc_lidar_read();      // call in lidar_read_requested_cb
 *   hil_diag_set_last_lidar(mm);    // record last served LiDAR value
 *   hil_diag_set_last_euler_x(v);   // record last served IMU euler_x
 *   hil_diag_set_last_inject_ts(t); // record last injected timestamp
 */

#ifndef HIL_DIAG_H
#define HIL_DIAG_H

#include <zephyr/sys/atomic.h>
#include <stdint.h>

/* ── Snapshot struct (returned by hil_diag_get_snapshot) ─────────────────── */
typedef struct {
    uint32_t spi_rx_count;        /**< total successfully decoded SPI frames  */
    uint32_t decode_fail_count;   /**< total nanopb decode failures           */
    uint32_t timer_inject_count;  /**< total timer ISR injections             */
    uint32_t scheduler_q_evict;   /**< total scheduler_q oldest-evictions     */
    uint32_t imu_read_count;      /**< total I2C read_requested on IMU        */
    uint32_t lidar_read_count;    /**< total I2C read_requested on LiDAR      */
    uint32_t last_inject_ts_us;   /**< timestamp_us of last injected packet   */
    int32_t  last_lidar_mm;       /**< last distance served to I2C bus        */
    int32_t  last_euler_x;        /**< last IMU euler_x served to I2C bus     */
} hil_diag_snapshot_t;

/* ── Increment helpers (ISR-safe) ────────────────────────────────────────── */
void hil_diag_inc_spi_rx(void);
void hil_diag_inc_decode_fail(void);
void hil_diag_inc_timer_inject(void);
void hil_diag_inc_scheduler_q_evict(void);
void hil_diag_inc_imu_read(void);
void hil_diag_inc_lidar_read(void);

/* ── Value setters (ISR-safe) ────────────────────────────────────────────── */
void hil_diag_set_last_inject_ts(uint32_t ts_us);
void hil_diag_set_last_lidar(int32_t mm);
void hil_diag_set_last_euler_x(int32_t val);

/* ── Snapshot reader (thread context only) ───────────────────────────────── */
void hil_diag_get_snapshot(hil_diag_snapshot_t *out);

#endif /* HIL_DIAG_H */
