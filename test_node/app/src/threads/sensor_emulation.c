#include "threads/sensor_emulation.h"
#include "threads/rc.h"
#include "threads/lidar_emulator.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

static const struct device *i2c_lidar = DEVICE_DT_GET(DT_ALIAS(i2c_lidar));
static const struct device *i2c_imu = DEVICE_DT_GET(DT_ALIAS(i2c_imu));
static const struct device *uart_rc = DEVICE_DT_GET(DT_ALIAS(uart_rc));

// register logging module
LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

K_MSGQ_DEFINE(sensor_update_q, SENSOR_UPDATE_QUEUE_PACKET_SIZE, SENSOR_UPDATE_QUEUE_LEN, 1);

// define mutexes for sensor data
K_MUTEX_DEFINE(imu_data_mutext);
K_MUTEX_DEFINE(lidar_data_mutex);
K_MUTEX_DEFINE(rc_data_mutex); // may not need later on, need to flesh out the RC command handling system
// TODO: maybe add mutex for current imu reg

// latest sensor data variables
static imu_data_t latest_imu_data;

// current register that master is reading from for IMU
static uint8_t current_imu_read_reg = 0;

// helper function to get IMU field
static uint8_t get_imu_data_field(uint8_t reg) {
    k_mutex_lock(&imu_data_mutext, K_FOREVER);

    uint8_t field_value = 0;
    uint8_t offset = 0;

    switch(reg) {
        // start of gyro data registers
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
             offset = reg - 0x14;
             field_value = *((uint8_t*)&latest_imu_data.gyro + offset);
             break;
        // start of quaternion registers
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        // start of linear accel data registers (convenienty right after quaternion registers)
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        // start of grav data registers (conveniently right after linear accel registers)
        case 0x2E:
        case 0x2F:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
            offset = reg - 0x20;
            field_value = *((uint8_t*)&latest_imu_data.quaternion + offset);
            break;
        // start of quaternion data registers
        default:
            LOG_ERR("Unexpected IMU register: 0x%02x\n", reg);
            break;
    }

    k_mutex_unlock(&imu_data_mutext);
    return field_value;
}

/* Callback which is called when a write request is received from the master. */
/* for our case, nothing needs to be done */
int i2c_target_write_requested_cb(struct i2c_target_config *config)
{
	return 0;
}

/* Callback which is called when a write is received from the master */
int i2c_target_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
    if(config->address == IMU_ADDRESS) {
        // master is writing to IMU registers to request data, so update current_imu_read_reg
        current_imu_read_reg = val;
    } 
	
    return 0;
}

// TODO: for read_* callbacks, need to figure out clock stretching and how to keep clock stretched if mutex is being used
/* Callback which is called when a read request is received from the master. */
int i2c_target_read_requested_cb(struct i2c_target_config *config, uint8_t *val)
{
	if(config->address == IMU_ADDRESS) {
        // master is requesting to read from IMU register, so get the appropriate field value and return it
        *val = get_imu_data_field(current_imu_read_reg);
    } else if (config->address == LIDAR_ADDRESS) {
        // master is requesting to read from LIDAR register, so return latest lidar distance
        // TODO: kaan write this part
    }
	return 0;
}

/* Callback which is called when a read is processed from the master. */
int i2c_target_read_processed_cb(struct i2c_target_config *config, uint8_t *val)
{
	printk("sample target read processed: 0x%02x\n", *val);
	*val = 0x43;
	return 0;
}

/* Callback which is called when the master sends a stop condition. */
int i2c_target_stop_cb(struct i2c_target_config *config)
{
	printk("sample target stop callback\n");
	return 0;
}

static struct i2c_target_callbacks sample_target_callbacks = {
	.write_requested = i2c_target_write_requested_cb,
	.write_received = i2c_target_write_received_cb,
	.read_requested = i2c_target_read_requested_cb,
	.read_processed = i2c_target_read_processed_cb,
	.stop = i2c_target_stop_cb,
};

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
            if(update_packet.sensor_id == RC_DEVICE_ID) {
                rc_command_received(update_packet.rc_command);
            }

            scheduler_event.state = K_POLL_STATE_NOT_READY;
        }

    }
}

K_THREAD_STACK_DEFINE(sensor_emulation_stack, SENSOR_EMULATION_STACK_SIZE);
struct k_thread sensor_emulation_data;

// Initialize and start thread
void sensor_emulation_init() {
    // dummy printing for sanity
    lidar_emulator_init(i2c_lidar);


    // Any initialization
	if (i2c_target_register(i2c_lidar, &lidar_cfg) < 0) {
		printk("Failed to register target\n");
        return;
	}

    if (i2c_target_register(i2c_imu, &imu_cfg) < 0) {
		printk("Failed to register target\n");
        return;
	}

    if(rc_init(uart_rc) < 0) {
        LOG_ERR("Failed to initialize RC UART");
        return;
    }

    k_thread_create(&sensor_emulation_data, sensor_emulation_stack, 
        K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
        sensor_emulation_thread,
        NULL, NULL, NULL,
        SENSOR_EMULATION_PRIORITY, 0, K_NO_WAIT);
}
