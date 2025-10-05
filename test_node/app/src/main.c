#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "threads/example_consumer.h"
#include "threads/example_producer.h"
#include "threads/orchestrator_comms.h"
#include "threads/scheduler.h"
#include "threads/sensor_emulation.h"
#include "threads/dut_interface.h"

int main(void)
{
    printk("Hello World! %s\n", CONFIG_BOARD);

    example_consumer_init();
    example_producer_init();

    orchestrator_comms_init();
    scheduler_init();
    sensor_emulation_init();
    dut_interface_init();

	return 0;
}
