#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec my_button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

int main(void)
{
    gpio_pin_configure_dt(&my_button, GPIO_INPUT);

    while (1)
    {
        printk("%d\n", gpio_pin_get_dt(&my_button));
    }

    return 0;
}
