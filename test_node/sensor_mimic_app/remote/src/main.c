/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ipm.h>

static const struct device *ipm_dev;
static char resp[64];
struct k_sem sync;

// WAITS FOR SPI TRASNCV
// AFTER, SENDS IPC MESSAGE
int main(void)
{

	ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm0));
	if (!ipm_dev) {
		printk("Failed to get IPM device.\n\r");
		return 0;
	}

	while (1) {
		snprintf(resp, sizeof(resp), "APP_CPU uptime ticks %lli\n", k_uptime_ticks());
		ipm_send(ipm_dev, -1, sizeof(resp), &resp, sizeof(resp));
		k_sleep(K_MSEC(10000));
	}

	return 0;
}
