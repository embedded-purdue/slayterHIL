#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("flash bruh %s\n", CONFIG_BOARD);

	return 0;
}
