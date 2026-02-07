/*
 * Copyright (c) 2024 Open Pixel Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

static const struct device *i2c0 = DEVICE_DT_GET(DT_NODELABEL(i2cbb0));
static char last_byte;

/*
 * @brief Callback which is called when a write request is received from the master.
 * @param config Pointer to the target configuration.
 */
int sample_target_write_requested_cb(struct i2c_target_config *config)
{
	printk("\nsample target write requested\n");
	return 0;
}

/*
 * @brief Callback which is called when a write is received from the master.
 * @param config Pointer to the target configuration.
 * @param val The byte received from the master.
 */
int sample_target_write_received_cb(struct i2c_target_config *config, uint8_t val)
{
	printk("\nsample target write received: 0x%02x\n", val);
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
	printk("\nsample target read request: 0x%02x\n", *val);
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
	printk("\nsample target read processed: 0x%02x\n", *val);
	*val = 0x43;
	return 0;
}

/*
 * @brief Callback which is called when the master sends a stop condition.
 * @param config Pointer to the target configuration.
 */
int sample_target_stop_cb(struct i2c_target_config *config)
{
	printk("\nsample target stop callback\n");
	return 0;
}

static struct i2c_target_callbacks sample_target_callbacks = {
	.write_requested = sample_target_write_requested_cb,
	.write_received = sample_target_write_received_cb,
	.read_requested = sample_target_read_requested_cb,
	.read_processed = sample_target_read_processed_cb,
	.stop = sample_target_stop_cb,
};

static struct i2c_target_config target_cfg = {
	.address = 0x60,
	.callbacks = &sample_target_callbacks,
};

int main(void)
{
	

	printk("i2c custom target sample\n");

	if (i2c_target_register(i2c0, &target_cfg) != 0) {
		printk("Failed to register target\n");
		return -1;
	}

	return 0;
}
