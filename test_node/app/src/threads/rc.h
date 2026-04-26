#ifndef RC_H
#define RC_H
#include <zephyr/kernel.h>
#include "threads/sensor_emulation.h"

bool rc_command_received(rc_data_t command);
int rc_init(const struct device* dev);

#endif
