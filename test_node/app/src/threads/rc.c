#include "threads/rc.h"
#include "threads/sensor_emulation.h" 
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

// register logging module
LOG_MODULE_REGISTER(rc_thread, LOG_LEVEL_INF);
static const struct device* uart_dev;

bool rc_command_received(rc_data_t command) {
    // check to ensure all of the characters in the command are valid (F/B/L/R for horiz, U/D for vert)
    if((command.rc_horiz != 'F' && command.rc_horiz != 'B' && command.rc_horiz != 'L' && command.rc_horiz != 'R' && command.rc_horiz != 'I') || 
       (command.rc_vert != 'U' && command.rc_vert != 'D' && command.rc_vert != 'I')) {
        LOG_ERR("Invalid RC command received: horiz = %c, vert = %c", command.rc_horiz, command.rc_vert);
        return false;
    }
    // store latest command in atomic variable
    // atomic_set(&latest_rc_command, (atomic_t));
    // send the command over the UART interface
    // 'I' means idle
    // order should (i think) be 1. [L/R] 2. [F/B] 3. [U/D]
    if(command.rc_horiz == 'F' || command.rc_horiz == 'B') {
        uart_poll_out(uart_dev, 'I');
        uart_poll_out(uart_dev, command.rc_horiz);
    } else {
        uart_poll_out(uart_dev, command.rc_horiz);
        uart_poll_out(uart_dev, 'I');
    }

    uart_poll_out(uart_dev, command.rc_vert);

    return true;
}

int rc_init(const struct device* dev) {
    // initialize UART for RC
    if (!device_is_ready(dev)) {
        LOG_ERR("UART device for RC is not ready");
        return -1;
    }

    uart_dev = dev;
    return 0;
}
