#include "threads/sensor_emulation.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

static const struct device *bus = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static char last_byte;

// register logging module
LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

K_MSGQ_DEFINE(sensor_update_q, SENSOR_UPDATE_QUEUE_PACKET_SIZE, SENSOR_UPDATE_QUEUE_LEN, 1);
K_MSGQ_DEFINE(sensor_bus_q, SENSOR_BUS_QUEUE_PACKET_SIZE, SENSOR_BUS_QUEUE_LEN, 1);

// i2c custom target callbacks
/*
 * @brief Callback which is called when a write request is received from the master.
 * @param config Pointer to the target configuration.
 */
int sample_target_write_requested_cb(struct i2c_target_config *config)
{
	printk("sample target write requested\n");
	return 0;
}

/*
 * @brief Callback which is called when a write is received from the master.
 * @param config Pointer to the target configuration.
 * @param val The byte received from the master.
 */
int sample_target_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
	printk("sample target write received: 0x%02x\n", val);
	last_byte = val;
	return 0;
}

/*
 * @brief Callback which is called when a read request is received from the master.
 * @param config Pointer to the target configuration.
 * @param val Pointer to the byte to be sent to the master.
 */
int sample_target_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
	printk("sample target read request: 0x%02x\n", *val);
	*val = 0x42;
	return 0;
}

/*
 * @brief Callback which is called when a read is processed from the master.
 * @param config Pointer to the target configuration.
 * @param val Pointer to the next byte to be sent to the master.
 */
int sample_target_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
	printk("sample target read processed: 0x%02x\n", *val);
	*val = 0x43;
	return 0;
}

/*
 * @brief Callback which is called when the master sends a stop condition.
 * @param config Pointer to the target configuration.
 */
int sample_target_stop_cb(struct i2c_target_config *config)
{
	printk("sample target stop callback\n");
	return 0;
}

static struct i2c_target_callbacks sample_target_callbacks = {
	.write_requested = sample_target_write_requested_cb,
	.write_received = sample_target_write_received_cb,
	.read_requested = sample_target_read_requested_cb,
	.read_processed = sample_target_read_processed_cb,
	.stop = sample_target_stop_cb,
};

static void sensor_emulation_thread(void *, void *, void *) {
    struct k_poll_event events[2];

    k_poll_event_init(&events[0], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &sensor_update_q);
    k_poll_event_init(&events[1], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &sensor_bus_q);

    SensorUpdatePacket update_packet;
    SensorBusPacket bus_packet;

    while (1) {
        // dummy printing for sanity
        LOG_INF("Sensor hello world\n");

        // Get data from sensor_update_q or sensor_bus_q
        k_poll(events, 2, K_FOREVER);
        
        // Received sensor update
        if (events[0].state == K_POLL_TYPE_MSGQ_DATA_AVAILABLE) {
            k_msgq_get(&sensor_update_q, &update_packet, K_FOREVER);

            // Update sensor data

            events[0].state = K_POLL_STATE_NOT_READY;
        }

        // Received bus data
        if (events[1].state == K_POLL_TYPE_MSGQ_DATA_AVAILABLE) {
            k_msgq_get(&sensor_bus_q, &bus_packet, K_FOREVER);

            // Respond to bus message

            events[1].state = K_POLL_STATE_NOT_READY;
        }
    }
}

K_THREAD_STACK_DEFINE(sensor_emulation_stack, SENSOR_EMULATION_STACK_SIZE);
struct k_thread sensor_emulation_data;

// Initialize and start thread
void sensor_emulation_init() {
    // dummy printing for sanity
    LOG_INF("Sensor init\n");

    // Any initialization

    struct i2c_target_config lidar_cfg = {
		.address = 0x62,
		.callbacks = &sample_target_callbacks,
	};

    struct i2c_target_config imu_cfg = {
		.address = 0x29,
		.callbacks = &sample_target_callbacks,
	};

	if (i2c_target_register(bus, &lidar_cfg) < 0) {
		printk("Failed to register target\n");
        return;
	}

    if (i2c_target_register(bus, &imu_cfg) < 0) {
		printk("Failed to register target\n");
        return;
	}

    k_thread_create(&sensor_emulation_data, sensor_emulation_stack, 
        K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
        sensor_emulation_thread,
        NULL, NULL, NULL,
        SENSOR_EMULATION_PRIORITY, 0, K_NO_WAIT);
}
