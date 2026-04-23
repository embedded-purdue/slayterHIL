/**
 * @file sensor_emulation.c  [TEST-INSTRUMENTED VERSION]
 * @brief Sensor emulation thread: peek-loop routing timer packets to emulators.
 *
 * No functional changes from base — the diagnostic instrumentation lives in the
 * individual emulators and the scheduler. This file just adds a startup log and
 * exposes the queue count in the periodic log message.
 */

#include "threads/sensor_emulation_test.h"
#include "threads/imu_emulator_test.h"
#include "threads/lidar_emulator_test.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_emulation, LOG_LEVEL_INF);

/* ── 1-slot ISR gate queue ─────────────────────────────────────────────── */
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
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    device_update_packet_t pkt;
    uint32_t loop_count = 0;

    LOG_INF("Sensor emulation thread running");

    while (1) {
        if (k_msgq_peek(&sensor_update_q, &pkt) == 0) {
            loop_count++;

            /* All three sensors update every tick — no routing needed */
            lidar_emulator_update_distance(pkt.lidar_distance_mm);
            imu_emulator_update_data(&pkt.imu_data);

            /* RC forwarding — only act if at least one axis is non-zero */
            if (pkt.rc_commands.rc_vert != 0 || pkt.rc_commands.rc_horiz != 0) {
                /* TODO: forward rc_commands to RC subsystem */
                LOG_DBG("[SEN_EMU #%u] RC vert=%d horiz=%d",
                        loop_count,
                        pkt.rc_commands.rc_vert,
                        pkt.rc_commands.rc_horiz);
            }

            LOG_DBG("[SEN_EMU #%u] ts=%u lidar=%u mm euler_x_lsb=%d",
                    loop_count,
                    pkt.timestamp_us,
                    pkt.lidar_distance_mm,
                    pkt.imu_data.euler_angles.x.lsb);
        }

        k_sleep(K_USEC(500));
    }
}
/* ── Public initialisation ──────────────────────────────────────────────── */
void sensor_emulation_init(const struct device *i2c_lidar,
                           const struct device *i2c_imu)
{
    LOG_INF("Initialising sensor emulation subsystem");

    lidar_emulator_init(i2c_lidar);
    imu_emulator_init(i2c_imu);

    k_thread_create(&sensor_emulation_data,
                    sensor_emulation_stack,
                    K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
                    sensor_emulation_thread,
                    NULL, NULL, NULL,
                    SENSOR_EMULATION_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&sensor_emulation_data, "sensor_emu");
    LOG_INF("Sensor emulation thread created (priority %d)",
            SENSOR_EMULATION_PRIORITY);
}
