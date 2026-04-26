#include "threads/scheduler.h"
#include "threads/sensor_emulation.h" 
#include <sensor_data.pb.h> 
#include <pb_decode.h> 
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Register logging module
LOG_MODULE_REGISTER(fs_scheduler_thread, LOG_LEVEL_INF);

// Define SPI device from devicetree
#define SPI_DEV_NODE DT_ALIAS(SPI_DEV_NAME)
static const struct device *spi_dev = DEVICE_DT_GET(SPI_DEV_NODE);

// The Ring Buffer (Replacing orchestrator_receive_q)
K_MSGQ_DEFINE(scheduler_q, sizeof(device_update_packet_t), 50, 4);

// SPI Asynchronous Buffers
static uint8_t rx_buffer[256];
static struct spi_buf spi_rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
static struct spi_buf_set spi_rx = { .buffers = &spi_rx_buf, .count = 1 };

// SPI Slave Configuration [cite: 83, 84]
struct spi_config spi_cfg = {
    .frequency = 10000000,
    .operation = SPI_OP_MODE_SLAVE | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
};

// Synchronization Primitives
K_SEM_DEFINE(spi_rx_sem, 0, 1);
struct k_timer injection_timer;

// 1. SPI Async Callback (Executes in ISR context)
static void spi_rx_callback(const struct device *dev, int result, void *data) {
    k_sem_give(&spi_rx_sem);
}

// 2. Hardware Timer ISR (The Timekeeper)
static void injection_timer_isr(struct k_timer *timer_id) {
    device_update_packet_t packet;
    
    // Pop the scheduled packet and push to the hardware emulators [cite: 102]
    if (k_msgq_get(&scheduler_q, &packet, K_NO_WAIT) == 0) {
        k_msgq_put(&sensor_update_q, &packet, K_NO_WAIT);
        
        // Peek at the NEXT packet to schedule the next interrupt
        device_update_packet_t next_packet;
        if (k_msgq_peek(&scheduler_q, &next_packet) == 0) {
            uint32_t current_time_us = k_cyc_to_us_near32(k_cycle_get_32());
            int32_t delay_us = next_packet.timestamp_us - current_time_us;
            
            // Prevent negative delays if we are running slightly behind
            if (delay_us <= 0) {
                delay_us = 1; 
            }
            k_timer_start(&injection_timer, K_USEC(delay_us), K_NO_WAIT);
        }
    }
}

// 3. Main Scheduler Thread (The Producer)
static void scheduler_thread(void *p1, void *p2, void *p3) {
    k_timer_init(&injection_timer, injection_timer_isr, NULL);

    while(1) {
        // Yield CPU and let DMA/SPI hardware receive the payload
        spi_read_async(spi_dev, &spi_cfg, &spi_rx, spi_rx_callback, NULL);
        k_sem_take(&spi_rx_sem, K_FOREVER);

        // Packet completely received. Decode using Protobuf [cite: 90, 91]
        SensorPacket sensorPack = SensorPacket_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(rx_buffer, sizeof(rx_buffer));
        int statusForDecode = pb_decode(&stream, SensorPacket_fields, &sensorPack);

        if (statusForDecode) {
            device_update_packet_t packet;
            memset(&packet, 0, sizeof(device_update_packet_t));

            // Map the fields [cite: 92, 93]
            // Assuming your protobuf schema has timestamp_us mapped correctly
            packet.sensor_id = LIDAR_DEVICE_ID; // Match ID from sensor_emulation.c [cite: 108]
            packet.lidar_distance_mm = sensorPack.lidar_data_mm;
            packet.timestamp_us = sensorPack.timestamp_us; 

            // Push into our scheduling ring buffer
            k_msgq_put(&scheduler_q, &packet, K_NO_WAIT);

            // Kickstart the timer if it is currently idle
            if (k_timer_status_get(&injection_timer) == 0) {
                uint32_t current_time_us = k_cyc_to_us_near32(k_cycle_get_32());
                int32_t delay_us = packet.timestamp_us - current_time_us;
                
                if (delay_us <= 0) {
                    delay_us = 1;
                }
                k_timer_start(&injection_timer, K_USEC(delay_us), K_NO_WAIT);
            }
        } else {
            LOG_ERR("Failed to decode Protobuf in scheduler_thread"); [cite: 91]
        }
    }
}

// Thread Definitions [cite: 96, 97]
K_THREAD_STACK_DEFINE(scheduler_stack, SCHEDULER_STACK_SIZE);
struct k_thread scheduler_data;

// Initialize and start thread [cite: 97, 98]
void scheduler_init() {
    LOG_INF("Scheduler init\n");

    k_thread_create(&scheduler_data, scheduler_stack, 
        K_THREAD_STACK_SIZEOF(scheduler_stack),
        scheduler_thread,
        NULL, NULL, NULL,
        SCHEDULER_PRIORITY, 0, K_NO_WAIT); [cite: 98]
}
