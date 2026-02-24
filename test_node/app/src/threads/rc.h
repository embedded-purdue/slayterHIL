#ifndef RC_H
#define RC_H
#include <zephyr/kernel.h>

void rc_command_received(uint8_t command);
int rc_init(const struct device* dev);

#endif