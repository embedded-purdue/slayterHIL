#include "bno.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <../../drivers/sensor/bno055/bno055.h>



static const struct device *bno_dev = NULL;
const char *now_str(void) { 
    static char buf[16]; /* ...HH:MM:SS.MMM */
    uint32_t now = k_uptime_get_32();
    unsigned int ms = now % MSEC_PER_SEC;
    unsigned int s;
    unsigned int min;
    unsigned int h;

    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
         h, min, s, ms);
    return buf;
}

static int process_bno055(const struct device *dev, struct bno_data *out) { 
    struct sensor_value eul[3]; 
    int rc; 
    rc = sensor_sample_fetch(dev); 
    if(rc) { 
        return rc; 
    }


    rc = sensor_channel_get(dev, BNO055_SENSOR_CHAN_EULER_YRP, eul);
    if(rc) { 
        return rc; 
    }

    out->time = k_uptime_get_32();

    out->eul.yaw = (float)eul[0].val1 + (float)eul[0].val2 / 1000000.0f;
    out->eul.roll  = (float)eul[1].val1 + (float)eul[1].val2 / 1000000.0f;  
    out ->eul.pitch = (float)eul[2].val1 + (float)eul[2].val2 / 1000000.0f;
    return 0; 
}

static struct sensor_trigger trigger; 

static void handle_bno055_drdy(const struct device *dev, const struct sensor_trigger *trig) { 
    k_sem_give(&bno_sem);
}

void bno_read_thread(void *arg1, void *arg2, void *arg3) { 
    while(1) { 
        k_sem_take(&bno_sem, K_FOREVER); 
        struct bno_data data; 
        const struct device *dev = bno_dev ? bno_dev : (const struct device *)arg1;
        int rc = process_bno055(dev, &data); 
        if(rc == 0) { 
            int put_rc = k_msgq_put(&bno_msgq, &data, K_NO_WAIT); 
            if(put_rc != 0) { 
                printk("BNO msgq full, dropping data\n"); 
            }
        } else { 
            printk("Failed to read BNO055 data: %d\n", rc); 
        }
    }
}

int bno_init(const struct device *dev) { 
    int rc; 
    if(dev == NULL) { 
        printk("BNO055 device not found\n"); 
        return -1; 
    }
    if(!device_is_ready(dev)) { 
        printk("BNO055 device not ready\n"); 
        return -1; 
    }

    bno_dev = dev; 

    struct sensor_value config = { 
        .val1 = BNO055_MODE_NDOF, 
        .val2 = 0,
    }; 

    sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CONFIGURATION, &config);
    config.val1 = BNO055_POWER_NORMAL;
    config.val2 = 0;
    sensor_attr_set(dev, SENSOR_CHAN_ALL, BNO055_SENSOR_ATTR_POWER_MODE, &config);
    trigger = (struct sensor_trigger) { 
        .type = SENSOR_TRIG_DATA_READY, 
        .chan = SENSOR_CHAN_ACCEL_XYZ,
    }; 

    rc = sensor_trigger_set(dev, &trigger, handle_bno055_drdy);
    if(rc) { 
        printk("Failed to set BNO055 trigger: %d\n", rc); 
        return rc; 
    }
    printk("BNO055 initialized successfully\n");
    return 0;
}

int bno_get(struct bno_data *out, k_timeout_t timeout) { 
    if(out == NULL) { 
        printk("bno_get: output pointer is NULL\n"); 
        return -1; 
    }
    return k_msgq_get(&bno_msgq, out, timeout);
}