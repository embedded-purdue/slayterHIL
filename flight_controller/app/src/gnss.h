#ifndef GNSS_H
#define GNSS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdint.h>

#define GNSS_NMEA_MAX_LEN 96
struct gnss_sentence {
    uint32_t time;
    char text[GNSS_NMEA_MAX_LEN];
};

extern struct k_sem gnss_sem;
extern struct k_msgq gnss_msgq;

int gnss_init(const struct device *dev);
int gnss_get(struct gnss_sentence *out, k_timeout_t timeout);
void gnss_read_thread(void *arg1, void *arg2, void *arg3);
void gnss_timer_handler(struct k_timer *timer_id);

#endif /* GNSS_H */
