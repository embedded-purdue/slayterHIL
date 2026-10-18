/** SPI receiver for the MVP: one 256-byte frame, applied on receipt.
 * Simulation-relative timestamps are metadata, never local-clock deadlines.
 * Rebuild both endpoints from sensor_data.proto before running this sender.
 */
#include "threads/scheduler_test.h"
#include "threads/sensor_emulation_test.h"
#include "sensor_frame.h"
#include "hil_diag.h"
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(scheduler, LOG_LEVEL_INF);
static const struct device *const spi_dev = DEVICE_DT_GET(DT_ALIAS(spi_orchestrator));
static uint8_t rx_buf[256];
static struct spi_buf rx_desc = { .buf = rx_buf, .len = sizeof(rx_buf) };
static struct spi_buf_set rx_set = { .buffers = &rx_desc, .count = 1 };
static const struct spi_config spi_cfg = {
    .frequency = 500000U,
    .operation = SPI_OP_MODE_SLAVE | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
};
atomic_t spi_err_count = ATOMIC_INIT(0);
K_THREAD_STACK_DEFINE(scheduler_stack, SCHEDULER_STACK_SIZE);
static struct k_thread scheduler_data;

static i2c_imu_data_16_t pack(int32_t value)
{
    const uint16_t bits = (uint16_t)value;
    return (i2c_imu_data_16_t){ .lsb = (int8_t)(bits & 0xff), .msb = (int8_t)(bits >> 8) };
}

static void scheduler_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    LOG_INF("SPI receiver ready: 256 bytes, mode 0, IMMEDIATE injection");
    uint32_t received = 0;
    while (1) {
        const int err = spi_read(spi_dev, &spi_cfg, &rx_set);
        // Some slave drivers return the positive byte count on success.
        if (err < 0 || (err > 0 && err != sizeof(rx_buf))) {
            atomic_inc(&spi_err_count);
            k_sleep(K_MSEC(1));
            continue;
        }
        SensorPacket proto;
        const char *error;
        if (!sensor_frame_decode(rx_buf, sizeof(rx_buf), &proto, &error)) {
            hil_diag_inc_decode_fail();
            LOG_WRN("Rejected frame: %s", error);
            continue;
        }
        device_update_packet_t pkt = {0};
        pkt.timestamp_us = proto.timestamp_us;
        pkt.lidar_distance_mm = (uint16_t)proto.lidar_mm;
        pkt.rc_commands.rc_vert = (int8_t)proto.rc.vertical;
        pkt.rc_commands.rc_horiz = (int8_t)proto.rc.horizontal;
        pkt.imu_data.euler_angles = (i2c_imu_triplet_t){
            pack(proto.imu.euler_x), pack(proto.imu.euler_y), pack(proto.imu.euler_z)};
        pkt.imu_data.gyro = (i2c_imu_triplet_t){
            pack(proto.imu.gyro_x), pack(proto.imu.gyro_y), pack(proto.imu.gyro_z)};
        pkt.imu_data.linear_acceleration = (i2c_imu_triplet_t){
            pack(proto.imu.lin_acc_x), pack(proto.imu.lin_acc_y), pack(proto.imu.lin_acc_z)};
        hil_diag_inc_spi_rx();
        // Keep only the newest sample if the sensor thread falls behind.
        unsigned int key = irq_lock();
        if (k_msgq_num_used_get(&sensor_update_q)) hil_diag_inc_scheduler_q_evict();
        k_msgq_purge(&sensor_update_q);
        (void)k_msgq_put(&sensor_update_q, &pkt, K_NO_WAIT);
        irq_unlock(key);
        if (++received == 1 || received % 60 == 0) {
            LOG_INF("RX #%u sim_us=%u euler=(%d,%d,%d) gyro=(%d,%d,%d) acc=(%d,%d,%d)",
                    received, proto.timestamp_us,
                    proto.imu.euler_x, proto.imu.euler_y, proto.imu.euler_z,
                    proto.imu.gyro_x, proto.imu.gyro_y, proto.imu.gyro_z,
                    proto.imu.lin_acc_x, proto.imu.lin_acc_y, proto.imu.lin_acc_z);
        }
    }
}

void scheduler_init(void)
{
    k_thread_create(&scheduler_data, scheduler_stack, K_THREAD_STACK_SIZEOF(scheduler_stack),
                    scheduler_thread, NULL, NULL, NULL, SCHEDULER_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&scheduler_data, "scheduler");
}
