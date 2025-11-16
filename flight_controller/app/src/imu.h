#ifndef IMU_H
#define IMU_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
struct imu_data { 
    uint32_t time; 
    double temp; 
    double accel[3]; 
    double gyro[3];
};

const char *now_str(void);
int imu_init(const struct device *dev); 
int imu_get(struct imu_data *out, k_timeout_t timeout);
void imu_read_thread(void *arg1, void *arg2, void *arg3);

extern struct k_msgq imu_msgq; 
extern struct k_sem imu_sem; 

#endif