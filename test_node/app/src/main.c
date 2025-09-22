#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "threads/example_consumer.h"
#include "threads/example_producer.h"

int main(void)
{
    printk("Hello World! %s\n", CONFIG_BOARD);

    example_consumer_init();
    example_producer_init();

	return 0;
}
