/*
 * Copyright (c) 2024 Open Pixel Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <stddef.h>
#include <stdint.h>

static const struct device *i2c0 = DEVICE_DT_GET(DT_NODELABEL(i2c0));

#define TARGET_ADDR 0x60

static void dump_buf(const char *prefix, const uint8_t *buf, size_t len)
{
	printk("%s", prefix);
	for (size_t i = 0; i < len; ++i) {
		printk(" %02X", buf[i]);
	}
	printk("\n");
}

static void exercise_i2c(const struct device *dev)
{
	int ret;

	const uint8_t write_pat[] = {0xAA, 0x55, 0x0F, 0xF0};
	ret = i2c_write(dev, write_pat, sizeof(write_pat), TARGET_ADDR);
	printk("i2c_write -> %d\n", ret);

	uint8_t read_buf[sizeof(write_pat)] = {0};
	ret = i2c_read(dev, read_buf, sizeof(read_buf), TARGET_ADDR);
	printk("i2c_read -> %d\n", ret);
	dump_buf("i2c_read data:", read_buf, sizeof(read_buf));

	const uint8_t reg_addr = 0x01;
	uint8_t reg_reply[3] = {0};
	ret = i2c_write_read(dev, TARGET_ADDR, &reg_addr, 1, reg_reply, sizeof(reg_reply));
	printk("i2c_write_read -> %d\n", ret);
	dump_buf("i2c_write_read data:", reg_reply, sizeof(reg_reply));

	const uint8_t burst_payload[] = {0x10, 0x11, 0x12, 0x13};
	ret = i2c_burst_write(dev, TARGET_ADDR, 0x20, burst_payload, sizeof(burst_payload));
	printk("i2c_burst_write -> %d\n", ret);

	uint8_t burst_reply[sizeof(burst_payload)] = {0};
	ret = i2c_burst_read(dev, TARGET_ADDR, 0x20, burst_reply, sizeof(burst_reply));
	printk("i2c_burst_read -> %d\n", ret);
	dump_buf("i2c_burst_read data:", burst_reply, sizeof(burst_reply));

	uint8_t transfer_reg = 0x30;
	uint8_t transfer_resp[4] = {0};
	struct i2c_msg msgs[] = {
		{
			.buf = &transfer_reg,
			.len = 1,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = transfer_resp,
			.len = sizeof(transfer_resp),
			.flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP,
		},
	};

	ret = i2c_transfer(dev, msgs, ARRAY_SIZE(msgs), TARGET_ADDR);
	printk("i2c_transfer -> %d\n", ret);
	dump_buf("i2c_transfer data:", transfer_resp, sizeof(transfer_resp));
}

int main(void)
{
	if (!device_is_ready(i2c0)) {
		printk("i2c0 not ready\n");
		return -1;
	}

	printk("i2c custom controller sample\n");
	//exercise_i2c(i2c0);
	int ret;

	const uint8_t write_pat[] = {0x1, 0x2, 0x3, 0x4};
	ret = i2c_write(i2c0, write_pat, sizeof(write_pat), TARGET_ADDR);
	printk("i2c_write -> %d\n", ret);

	/*uint8_t read_buf[sizeof(write_pat)] = {0};
	ret = i2c_read(i2c0, read_buf, sizeof(read_buf), TARGET_ADDR);
	printk("i2c_read -> %d\n", ret);
	dump_buf("i2c_read data:", read_buf, sizeof(read_buf));*/

	return 0;
}
