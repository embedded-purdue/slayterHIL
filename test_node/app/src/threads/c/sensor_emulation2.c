/**
 * @file sensor_emulation.c
 * @brief Sensor emulation thread: peek-loop that routes timer-injected packets
 *        to the individual hardware emulators.
 *
 * Threading model
 * ───────────────
 *  • k_msgq_peek  — reads the 1-slot gate queue WITHOUT consuming the item.
 *    This leaves the slot "full" permanently so the timer ISR's purge+put
 *    sequence always has a valid target to overwrite.
 *
 *  • k_sleep(K_USEC(500)) — yields the CPU between peeks.  500 µs is a
 *    reasonable balance: tight enough to pick up new packets soon after the
 *    timer fires, loose enough to not starve the scheduler thread or the idle
 *    thread.  Tune this to ≤ half your minimum inter-packet interval.
 *
 *  • The emulator update functions (lidar_emulator_update_distance,
 *    imu_emulator_update_data) are the ONLY path from this thread to the I2C
 *    ISR.  They use atomic writes / ping-pong buffers — no mutexes, no
 *    blocking.
 */

#include "threads/sensor_emulation.h"
#include "threads/imu_emulator.h"
#include "threads/lidar_emulator.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_emulation, LOG_LEVEL_INF);

/* ── 1-slot ISR gate queue ─────────────────────────────────────────────────
 *
 * Aligned to 4 bytes for efficient 32-bit word copies inside k_msgq internals.
 */
K_MSGQ_DEFINE(sensor_update_q,
              sizeof(device_update_packet_t),
              SENSOR_UPDATE_QUEUE_LEN,
              4);

/* ── Thread resources ───────────────────────────────────────────────────── */
K_THREAD_STACK_DEFINE(sensor_emulation_stack, SENSOR_EMULATION_STACK_SIZE);
static struct k_thread sensor_emulation_data;

/* ── Thread entry point ─────────────────────────────────────────────────── */
static void sensor_emulation_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    device_update_packet_t pkt;

    LOG_INF("Sensor emulation thread running (peek-loop, yield=500us)");

    while (1) {
        /*
         * PEEK — not get.  We intentionally leave the item in the queue so
         * the slot is always occupied, allowing the timer ISR to call
         * k_msgq_purge() safely at any time without an empty-queue edge case.
         *
         * If the queue is empty (no packet has been injected yet on startup),
         * k_msgq_peek returns -ENOMSG and we simply sleep and retry.
         */
        if (k_msgq_peek(&sensor_update_q, &pkt) == 0) {
            switch (pkt.sensor_id) {
            case LIDAR_DEVICE_ID:
                lidar_emulator_update_distance(pkt.lidar_distance_mm);
                break;

            case IMU_DEVICE_ID:
                imu_emulator_update_data(&pkt.imu_data);
                break;

            case RC_DEVICE_ID:
                /* TODO: RC command forwarding */
                break;

            default:
                LOG_WRN("Unknown sensor_id=0x%02X dropped", pkt.sensor_id);
                break;
            }
        }

        /*
         * Yield to scheduler thread and I2C ISRs.
         * k_sleep with a non-zero duration is a cooperative yield that
         * reschedules immediately if a higher-priority thread is ready.
         * Do NOT use k_yield() (busy-spin) or K_NO_WAIT here.
         */
        k_sleep(K_USEC(500));
    }
}

/* ── Public initialisation ──────────────────────────────────────────────── */
void sensor_emulation_init(const struct device *i2c_lidar,
                           const struct device *i2c_imu)
{
    LOG_INF("Initialising sensor emulation subsystem");

    /* Initialise emulators first; they register their I2C target configs. */
    lidar_emulator_init(i2c_lidar);
    imu_emulator_init(i2c_imu);

    k_thread_create(&sensor_emulation_data,
                    sensor_emulation_stack,
                    K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
                    sensor_emulation_thread,
                    NULL, NULL, NULL,
                    SENSOR_EMULATION_PRIORITY,
                    0,
                    K_NO_WAIT);

    k_thread_name_set(&sensor_emulation_data, "sensor_emu");
    LOG_INF("Sensor emulation thread created at priority %d",
            SENSOR_EMULATION_PRIORITY);
}
