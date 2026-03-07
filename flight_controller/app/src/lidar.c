#include "lidar.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

static const struct dev *lidar_dev = NULL; 

void lidar_timer_handler(struct k_timer *timer_id) { 
    k_sem_give(&lidar_sem);
}


static int process_lidar_lite_v4(const struct device *dev, struct lidar_data *out) { 
    struct sensor_value distance; 
    int rc; 

    rc = sensor_sample_fetch(dev); 
    if(rc < 0) { 
        return rc; 
    }
    rc = sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &distance);
    if(rc < 0) { 
        return rc; 
    }

    out->time = k_uptime_get_32();
    out->distance_mm = distance.val1 * 1000 + distance.val2 / 1000;
    return 0; 

}

void lidar_read_thread(void *arg1, void *arg2, void *arg3) { 
    while(1)  { 
        k_sem_take(&lidar_sem, K_FOREVER); 

        struct lidar_data data; 
        const struct device *dev = lidar_dev ? lidar_dev : DEVICE_DT_GET(DT_NODELABEL(lidar)); 
        int rc = process_lidar_lite_v4(dev, &data);
        if(rc == 0) { 
            int put_rc = k_msgq_put(&lidar_msgq, &data, K_NO_WAIT);
            if (put_rc != 0) { 
                printk("LiDAR msgq full, dropping data\n");
            } else {
                printk("Failed to read data\n");
            }
            
        }
    }
}

int lidar_init(const struct device *dev) { 
    if(dev == NULL) { 
        printk("LiDAR device not found\n"); 
        return -1; 
    }
    if(!device_is_ready(dev)) { 
        printk("LiDAR device not ready\n"); 
        return -1;
    }
    lidar_dev = dev;
    printk("LiDAR initialized successfully\n");
    return 0; 
}

int lidar_get(struct lidar_data *out, k_timeout_t timeout) { 
    if(out == NULL) { 
        printk("lidar_get: output pointer is NULL\n");
        return -1;
    }
    return k_msgq_get(&lidar_msgq, out, timeout);
}