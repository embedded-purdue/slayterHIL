#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define sleep_time  1000
#define LED0_Node DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_Node, gpios);

int main(void)
{
    int ret; 
    bool led_state = true; 

    if(!gpio_is_ready_dt(&led)) { 
        printk("Error: LED device not ready\n");
        return 0; 
    }
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if(ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return 0; 
    }
    while(1) {
         ret = gpio_pin_set_dt(&led, (int)led_state);
         if(ret < 0) { 
            printk("Error %d: failed to set LED pin\n", ret);
            return 0; 
         }
         led_state = !led_state; 
         printk("LED is %s\n", led_state ? "ON" : "OFF");
         k_msleep(sleep_time);
    }
    return 0; 
}
