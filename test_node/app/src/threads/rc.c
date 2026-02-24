#include "threads/rc.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

// register logging module
LOG_MODULE_REGISTER(rc_thread, LOG_LEVEL_INF);

static atomic_t latest_rc_command = ATOMIC_INIT(0);

void rc_command_received(uint8_t command) {
    // store latest command in atomic variable
    atomic_set(&latest_rc_command, command);
}

int rc_init(const struct device* dev) {
    // initialize UART for RC
    if (!device_is_ready(dev)) {
        LOG_ERR("UART device for RC is not ready");
        return -1;
    }
    return 0;
}
