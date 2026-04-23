/**
 * @file hil_diag.c
 * @brief Diagnostic counter storage and snapshot implementation.
 */

#include "hil_diag.h"
#include <zephyr/sys/atomic.h>

/* ── Backing atomics ─────────────────────────────────────────────────────── */
static atomic_t diag_spi_rx          = ATOMIC_INIT(0);
static atomic_t diag_decode_fail     = ATOMIC_INIT(0);
static atomic_t diag_timer_inject    = ATOMIC_INIT(0);
static atomic_t diag_sched_q_evict   = ATOMIC_INIT(0);
static atomic_t diag_imu_read        = ATOMIC_INIT(0);
static atomic_t diag_lidar_read      = ATOMIC_INIT(0);
static atomic_t diag_last_inject_ts  = ATOMIC_INIT(0);
static atomic_t diag_last_lidar_mm   = ATOMIC_INIT(0);
static atomic_t diag_last_euler_x    = ATOMIC_INIT(0);

/* ── Increment helpers ───────────────────────────────────────────────────── */
void hil_diag_inc_spi_rx(void)           { atomic_inc(&diag_spi_rx);        }
void hil_diag_inc_decode_fail(void)      { atomic_inc(&diag_decode_fail);   }
void hil_diag_inc_timer_inject(void)     { atomic_inc(&diag_timer_inject);  }
void hil_diag_inc_scheduler_q_evict(void){ atomic_inc(&diag_sched_q_evict); }
void hil_diag_inc_imu_read(void)         { atomic_inc(&diag_imu_read);      }
void hil_diag_inc_lidar_read(void)       { atomic_inc(&diag_lidar_read);    }

/* ── Value setters ───────────────────────────────────────────────────────── */
void hil_diag_set_last_inject_ts(uint32_t ts_us) {
    atomic_set(&diag_last_inject_ts, (atomic_val_t)ts_us);
}
void hil_diag_set_last_lidar(int32_t mm) {
    atomic_set(&diag_last_lidar_mm, (atomic_val_t)mm);
}
void hil_diag_set_last_euler_x(int32_t val) {
    atomic_set(&diag_last_euler_x, (atomic_val_t)val);
}

/* ── Snapshot ────────────────────────────────────────────────────────────── */
void hil_diag_get_snapshot(hil_diag_snapshot_t *out)
{
    out->spi_rx_count       = (uint32_t)atomic_get(&diag_spi_rx);
    out->decode_fail_count  = (uint32_t)atomic_get(&diag_decode_fail);
    out->timer_inject_count = (uint32_t)atomic_get(&diag_timer_inject);
    out->scheduler_q_evict  = (uint32_t)atomic_get(&diag_sched_q_evict);
    out->imu_read_count     = (uint32_t)atomic_get(&diag_imu_read);
    out->lidar_read_count   = (uint32_t)atomic_get(&diag_lidar_read);
    out->last_inject_ts_us  = (uint32_t)atomic_get(&diag_last_inject_ts);
    out->last_lidar_mm      = (int32_t) atomic_get(&diag_last_lidar_mm);
    out->last_euler_x       = (int32_t) atomic_get(&diag_last_euler_x);
}
