/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/device.h>
#include <string.h>

/* constants */
// TODO: put in global .h file later
#define TRANS_SIZE (12)


static const struct device *ipm_dev;
static char received_string[TRANS_SIZE];
static struct k_sem sync;

static void ipm_receive_callback(const struct device *ipmdev, void *user_data, uint32_t id,
				 volatile void *data)
{
	ARG_UNUSED(ipmdev);
	ARG_UNUSED(user_data);

	strncpy(received_string, (const char *)data, sizeof(received_string));
	k_sem_give(&sync);
}

int main(void)
{
	k_sem_init(&sync, 0, 1);

	ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm0));
	if (!ipm_dev) {
		printk("Failed to get IPM device.\n\r");
		return 0;
	}

	ipm_register_callback(ipm_dev, ipm_receive_callback, NULL);

	/* Workaround to catch up with APPCPU */
	k_sleep(K_MSEC(50));

	while (1) {
		k_sem_take(&sync, K_FOREVER);
		printk("PRO_CPU received a message from APP_CPU :\n");
		for (int i = 0; i < TRANS_SIZE; i++) printk("received word %d: %d\n", i, received_string[i]);
	}
	return 0;
}
