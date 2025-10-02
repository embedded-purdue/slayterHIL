#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>  
#include <inttypes.h>

#include <string.h>
#include <ctype.h>

#define I2C_NODE   DT_NODELABEL(i2c0)
#define SLAVE_ADDR 0x55
#define RX_LEN     16
#define BUTTON_NODE DT_ALIAS(button0)
#if !DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
#error "Unsupported board: button0 devicetree alias is not defined"
#endif
const struct device *i2c = DEVICE_DT_GET(I2C_NODE);
static struct gpio_callback button_cb_data; 
static struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BUTTON_NODE, gpios, {0});
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
bool led_state = false; 

// Add work queue for I2C operations
static struct k_work i2c_work;

static void hexdump(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        printk("%02X ", buf[i]);
    }
    printk("\n");
}

// I2C work handler - runs in thread context, not interrupt context
static void i2c_work_handler(struct k_work *work)
{
    if(device_is_ready(i2c)) { 
        uint8_t cmd = 0x01; 
        uint8_t rx[RX_LEN] = {0}; 
        int ret = i2c_write_read(i2c, SLAVE_ADDR, &cmd, 1, rx, RX_LEN); 
        printk("write_read ret=%d  \n", ret);
        if (ret == 0) {
            printk("HEX: ");
            hexdump(rx, RX_LEN);

            // Print ASCII safely
            char ascii[RX_LEN + 1];
            for (int i = 0; i < RX_LEN; ++i) {
                ascii[i] = isprint((int)rx[i]) ? (char)rx[i] : '.';
            }
            ascii[RX_LEN] = '\0';
            printk("ASC: %s\n", ascii);
        }
    } else { 
        printk("I2C device not ready\n");
    }
}

// Interrupt handler - now just schedules work and toggles LED
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) { 
    printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
    
    // Schedule I2C work to run in thread context
    k_work_submit(&i2c_work);

    // LED toggle can stay here (fast operation)
    if(led.port) { 
        led_state = !led_state; 
        gpio_pin_set_dt(&led, led_state); 
        printk("Button toggled\n");
    }
}

void main(void) {
    int ret; 
    
    // Initialize work queue
    k_work_init(&i2c_work, i2c_work_handler);
    
    if (!device_is_ready(i2c)) {
        printk("I2C not ready\n");
        return;
    }

    if(!gpio_is_ready_dt(&button)) { 
        printk("Error: Button device %s not ready\n", button.port->name);
        return; 
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if(ret != 0) { 
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
    }
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if(ret != 0) { 
        printk("Error %d: failed to configure interrupt on %s pin %d\n", 
            ret, button.port->name, button.pin); 
        return; 
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    printk("Set up button at %s pin %d\n", button.port->name, button.pin);
    
    if(led.port && !gpio_is_ready_dt(&led)) { 
        printk("Error: LED device %s is not ready; ignoring it\n", led.port->name); 
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

    printk("System ready. Press the button to trigger I2C communication.\n");
    while (1) {
        k_sleep(K_MSEC(1000));
    }
}