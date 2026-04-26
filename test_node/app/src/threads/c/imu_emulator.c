#include "threads/imu_emulator.h"
#include "threads/sensor_emulation.h"
#include <zephyr/logging/log.h>
#define REG_DEFAULT 0

// register logging module
LOG_MODULE_REGISTER(imu_emulator, LOG_LEVEL_INF);

// latest sensor data variables
static imu_data_t latest_imu_data;

// current register that master is reading from for IMU
static uint8_t current_imu_read_reg = REG_DEFAULT;

// helper function to get IMU field
static uint8_t get_imu_data_field(uint8_t reg) {
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
        // start of euler angle registers
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
             offset = reg - 0x1A;
             field_value = *((uint8_t*)&latest_imu_data.euler_angles + offset);
             break;
        // start of linear accel data registers (convenienty right after quaternion registers)
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
            offset = reg - 0x28;
            field_value = *((uint8_t*)&latest_imu_data.linear_acceleration + offset);
            break;
        // start of quaternion data registers
        default:
            LOG_ERR("Unexpected IMU register: 0x%02x\n", reg);
            break;
    }

    return field_value;
}


// i2c callbacks

/* Callback which is called when a write request is received from the master. */
/* for our case, nothing needs to be done */
int imu_write_requested_cb(struct i2c_target_config *config)
{
	return 0;
}

/* Callback which is called when a write is received from the master */
int imu_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
    // master is writing to IMU registers to request data, so update current_imu_read_reg
    current_imu_read_reg = val;
    return 0;
}

int imu_read_cb(struct i2c_target_config *config, uint8_t *val)
{
    // master is requesting to read from IMU register, so get the appropriate field value and return it
	if(current_imu_read_reg != REG_DEFAULT) {
    	*val = get_imu_data_field(current_imu_read_reg);
    	current_imu_read_reg++;
	}
	return 0;
}

/* Callback which is called when the master sends a stop condition. */
int imu_stop_cb(struct i2c_target_config *config)
{
	printk("imu stop callback\n");
    current_imu_read_reg = REG_DEFAULT;
	return 0;
}

static struct i2c_target_callbacks imu_callbacks = {
    .write_requested = imu_write_requested_cb,
    .write_received = imu_write_received_cb,
    .read_requested = imu_read_cb,
    .read_processed = imu_read_cb,
    .stop = imu_stop_cb,
};

static struct i2c_target_config imu_config = {
    .address = IMU_ADDRESS,
    .callbacks = &imu_callbacks,
};

void imu_emulator_init(const struct device* i2c_dev) {
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("IMU I2C device not ready");
        return;
    }
    if (i2c_target_register(i2c_dev, &imu_config) < 0) {
        LOG_ERR("Failed to register IMU I2C target");
    } else {
        LOG_INF("IMU I2C target registered at address 0x%02x", LIDAR_ADDRESS);
    }
}

void imu_emulator_update_data(imu_data_t new_data){
    latest_imu_data = new_data;
}
