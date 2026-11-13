#ifndef SDCARD_H
#define SDCARD_H

#include <stdbool.h>

int sdcard_init(void);
bool sdcard_ready(void);

#endif /* SDCARD_H */
