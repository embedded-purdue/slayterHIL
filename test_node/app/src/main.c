#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void) {
    printk("Hello World! %s\n", CONFIG_BOARD);

    return 0;
}
