#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define SLEEP_TIME  1
// #define LED0_Node DT_ALIAS(led0)
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_Node, gpios);

#define BUTTON_NODE DT_ALIAS(button0)
#if !DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
#error "Unsupported board: button0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BUTTON_NODE, gpios, {0});
static struct gpio_callback button_cb_data; 
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
bool led_state = false;
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
    if(led.port) { 
        led_state = !led_state; 
        gpio_pin_set_dt(&led, led_state); 
        printk("Button toggled\n");
    }
}

int main(void)
{
    int ret; 
  

    if(!gpio_is_ready_dt(&button)) { 
        printk("Error: Button device %s not ready\n", button.port->name);
        return 0; 
    }
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT); 
    if(ret != 0) { 
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
    }
    ret = gpio_pin_interrupt_configure_dt(&button, 
        GPIO_INT_EDGE_TO_ACTIVE); 
    if(ret != 0) { 
        printk("Error %d: failed to configure interrupt on %s pin %d\n", 
            ret, button.port->name, button.pin); 
        return 0; 
    }
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin)); 
    gpio_add_callback(button.port, &button_cb_data); 
    printk("Set up button at %s pin %d\n", button.port->name, button.pin); 

    if(led.port && !gpio_is_ready_dt(&led)) { 
        printk("Error %d: LED device %s is not ready; ignoring it\n", 
            ret, led.port->name); 
            led.port = NULL; 
    }
    if (led.port) { 
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT); 
        if(ret != 0) {
            printk("Error %d: failed to configure LED device %s pin %d\n", 
                ret, led.port->name, led.pin); 
            led.port = NULL; 
        } else { 
            printk("Set up LED at %s pin %d\n", led.port->name, led.pin); 
        }
    }
    printk("Press the button\n"); 
    if(led.port) { 
        gpio_pin_set_dt(&led, 0);
    }
    return 0; 
        
}

