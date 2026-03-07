#ifndef BNO_H
#define BNO_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

typedef struct { 
    float roll; 
    float pitch; 
    float yaw;  
} euler_angles;


typedef struct { 
    uint32_t time;  
    euler_angles eul;
} bno_data;

const char *now_str(void);
int bno_init(const struct device *dev);

int bno_get(struct bno_data *out, k_timeout_t timeout);
void bno_read_thread(void *arg1, void *arg2, void *arg3);

extern struct k_msgq bno_msgq;
extern struct k_sem bno_sem; 