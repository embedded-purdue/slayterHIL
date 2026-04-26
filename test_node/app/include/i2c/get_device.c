#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>


const struct device* getDevice(char *nodeName) {
    return DEVICE_DT_GET(DT_NODE_LABEL(nodeName));
}

