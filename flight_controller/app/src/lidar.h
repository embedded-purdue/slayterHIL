#ifndef LIDAR_H
#define LIDAR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdint.h>

struct lidar_data { 
    uint32_t time; 
    uint16_t distance_mm;
};

extern struct k_sem lidar_sem; 
extern struct k_msgq lidar_msgq;

int lidar_init(const struct device *dev); 
int lidar_get(struct lidar_data *out, k_timeout_t timeout); 
void lidar_read_thread(void *arg1, void *arg2, void *arg3);
void lidar_timer_handler(struct k_timer *timer_id);


#endif 