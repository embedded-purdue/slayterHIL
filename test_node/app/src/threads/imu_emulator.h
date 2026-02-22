#include "threads/sensor_emulation.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

void imu_emulator_init(const struct device* i2c_dev);
void imu_emulator_update_data(imu_data_t new_data);