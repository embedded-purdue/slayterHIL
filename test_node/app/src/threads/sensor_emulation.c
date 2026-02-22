#include "threads/sensor_emulation.h"
#include "threads/imu_emulator.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

static const struct device *i2c_lidar = DEVICE_DT_GET(DT_ALIAS(i2c_lidar));
static const struct device *i2c_imu = DEVICE_DT_GET(DT_ALIAS(i2c_imu));

// register logging module
LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

K_MSGQ_DEFINE(sensor_update_q, SENSOR_UPDATE_QUEUE_PACKET_SIZE, SENSOR_UPDATE_QUEUE_LEN, 1);

// define mutexes for sensor data
K_MUTEX_DEFINE(lidar_data_mutex);
K_MUTEX_DEFINE(rc_data_mutex); // may not need later on, need to flesh out the RC command handling system
// TODO: maybe add mutex for current imu reg


static void sensor_emulation_thread(void *, void *, void *) {
    struct k_poll_event scheduler_event;

    k_poll_event_init(&scheduler_event, K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &sensor_update_q);

    device_update_packet_t update_packet;

    while (1) {
        // dummy printing for sanity
        LOG_INF("Sensor hello world\n");

        // Get data from sensor_update_q
        k_poll(&scheduler_event, 1, K_FOREVER);
        
        // Received sensor update
        if (scheduler_event.state == K_POLL_TYPE_MSGQ_DATA_AVAILABLE) {
            k_msgq_get(&sensor_update_q, &update_packet, K_FOREVER);

            // Update sensor data
            
            scheduler_event.state = K_POLL_STATE_NOT_READY;
            switch(update_packet.sensor_id) {
                case LIDAR_DEVICE_ID:
                    break;
                case IMU_DEVICE_ID:
                    imu_emulator_update_data(update_packet.imu_data);
                    break;
                case RC_DEVICE_ID:
                    //TODO:
                    break;
                default:
                    LOG_WRN("Unknown sensor ID received: %d", update_packet.sensor_id);
                    break;

            }
        }

    }
}

K_THREAD_STACK_DEFINE(sensor_emulation_stack, SENSOR_EMULATION_STACK_SIZE);
struct k_thread sensor_emulation_data;
/*
struct i2c_target_config lidar_cfg = {
    .address = LIDAR_ADDRESS,
    .callbacks = &sample_target_callbacks,
};
/*
struct i2c_target_config imu_cfg = {
    .address = IMU_ADDRESS,
    .callbacks = &sample_target_callbacks,
};
*/

// Initialize and start thread
void sensor_emulation_init() {
    // dummy printing for sanity
    LOG_INF("Sensor init\n");

    // Any initialization
	if (i2c_target_register(i2c_lidar, &lidar_cfg) < 0) {
		printk("Failed to register target\n");
        return;
	}
    /*
    if (i2c_target_register(i2c_imu, &imu_cfg) < 0) {
		printk("Failed to register target\n");
        return;
	}
    */
    imu_emulator_init(i2c_imu);

    k_thread_create(&sensor_emulation_data, sensor_emulation_stack, 
        K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
        sensor_emulation_thread,
        NULL, NULL, NULL,
        SENSOR_EMULATION_PRIORITY, 0, K_NO_WAIT);
}
