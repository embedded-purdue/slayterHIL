#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "threads/orchestrator_comms.h"
#include "threads/scheduler.h"
#include "threads/sensor_emulation.h"
#include "threads/dut_interface.h"

int main(void)
{
    printk("Hello World! %s\n", CONFIG_BOARD);

    orchestrator_comms_init();
    scheduler_init();
    sensor_emulation_init();
    dut_interface_init();

	return 0;
}
