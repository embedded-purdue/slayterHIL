#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define I2C_ADDR 0x7F

int main(void)
{
	const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	if(!device_is_ready(i2c)) {
		printk("i2c device not ready\n");
		return -1;
	}

	uint8_t data[] = { 0x10, 0x5, 0x8, 0x17 }; // this is just for example purposes

	int ret = i2c_write(i2c, data, sizeof(data), I2C_ADDR);

	if (ret) {
		printk("I2C write failed: %d\n", ret);
	} else {
		printk("I2C write successful\n");
	}

    printk("Complete!");

	return 0;
}
