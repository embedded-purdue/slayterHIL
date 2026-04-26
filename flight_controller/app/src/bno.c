#include "bno.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <../../drivers/sensor/bno055/bno055.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const struct device *bno_dev = NULL;
static struct sensor_trigger trig_acc_drdy; 
static struct sensor_trigger trig_acc_motion;
static struct sensor_trigger trig_acc_high_g;
static struct sensor_trigger trig_gyr_any_motion;
static struct sensor_trigger trig_gyr_high_rate;

static void acc_drdy(const struct device *dev, const struct sensor_trigger *trigger) { ARG_UNUSED(dev); ARG_UNUSED(trigger); }
static void acc_motion(const struct device *dev, const struct sensor_trigger *trigger) { ARG_UNUSED(dev); ARG_UNUSED(trigger); }
static void acc_high_g(const struct device *dev, const struct sensor_trigger *trigger) { ARG_UNUSED(dev); ARG_UNUSED(trigger); }
static void gyr_any_motion(const struct device *dev, const struct sensor_trigger *trigger) { ARG_UNUSED(dev); ARG_UNUSED(trigger); }
static void gyr_high_rate(const struct device *dev, const struct sensor_trigger *trigger) { ARG_UNUSED(dev); ARG_UNUSED(trigger); }

const char *now_str(void) { 
    static char buf[16];
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

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
    return buf;
}

static int process_bno055(const struct device *dev, struct bno_data *out) { 
    struct sensor_value eul[3]; 
    int rc; 
    
    rc = sensor_sample_fetch(dev); 
    if (rc) { 
        return rc; 
    }

    rc = sensor_channel_get(dev, BNO055_SENSOR_CHAN_EULER_YRP, eul);
    if (rc) { 
        return rc; 
    }

    out->time = k_uptime_get_32();
    // DO thsi when need precision just working on simple values rn 
    out->eul.yaw = (float)eul[0].val1 + (float)eul[0].val2 / 1000000.0f;
    out->eul.roll  = (float)eul[1].val1 + (float)eul[1].val2 / 1000000.0f;
    out->eul.pitch = (float)eul[2].val1 + (float)eul[2].val2 / 1000000.0f;
    // out->eul.yaw = (float)eul[1].val1;
    // out->eul.roll  = (float)eul[0].val1;
    // out->eul.pitch = (float)eul[2].val1;
    

    
    return 0; 
}

void bno_read_thread(void *arg1, void *arg2, void *arg3) { 
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    while (1) { 
        const struct device *dev = bno_dev ? bno_dev : (const struct device *)arg1;
        
        if (dev == NULL) {
            k_msleep(50);   // Wait until bno_init() sets bno_dev
            continue;
        }

        struct bno_data data; 
        int rc = process_bno055(dev, &data); 
        
        if (rc == 0) { 
            int put_rc = k_msgq_put(&bno_msgq, &data, K_NO_WAIT); 
            if (put_rc != 0) { 
                printk("BNO msgq full, dropping data\n"); 
            }
        } else { 
            printk("Failed to read BNO055 data: %d\n", rc); 
        }
        
        k_msleep(40); // Poll at 50 Hz; adjust as needed
    }
}

int bno_init(const struct device *dev)
{
    struct sensor_value config;
    int rc;

    if (dev == NULL || !device_is_ready(dev)) {
        printk("BNO055 device not ready\n");
        return -ENODEV;
    }

    bno_dev = dev;

    /* Mode + power */
    config.val1 = BNO055_MODE_NDOF;
    config.val2 = 0;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CONFIGURATION, &config);
    if (rc < 0) return rc;

    config.val1 = BNO055_POWER_NORMAL;
    config.val2 = 0;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_ALL, BNO055_SENSOR_ATTR_POWER_MODE, &config);
    if (rc < 0) return rc;

    /* DRDY */
    trig_acc_drdy.type = SENSOR_TRIG_DATA_READY;
    trig_acc_drdy.chan = SENSOR_CHAN_ACCEL_XYZ;
    rc = sensor_trigger_set(bno_dev, &trig_acc_drdy, acc_drdy);
    if (rc < 0) return rc;

    /* ACC any-motion */
    config.val1 = BNO055_ACC_DURATION_MOTION_ANY;
    config.val2 = 0x02;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SLOPE_DUR, &config);
    if (rc < 0) return rc;

    config.val1 = BNO055_ACC_THRESHOLD_MOTION_ANY;
    config.val2 = 0x04;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SLOPE_TH, &config);
    if (rc < 0) return rc;

    trig_acc_motion.type = SENSOR_TRIG_DELTA;
    trig_acc_motion.chan = SENSOR_CHAN_ACCEL_XYZ;
    rc = sensor_trigger_set(bno_dev, &trig_acc_motion, acc_motion);
    if (rc < 0) return rc;

    /* ACC high-G */
    config.val1 = BNO055_ACC_DURATION_HIGH_G;
    config.val2 = 0x7F;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SLOPE_DUR, &config);
    if (rc < 0) return rc;

    config.val1 = BNO055_ACC_THRESHOLD_HIGH_G;
    config.val2 = 0x48;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SLOPE_TH, &config);
    if (rc < 0) return rc;

    trig_acc_high_g.type = BNO055_SENSOR_TRIG_HIGH_G;
    trig_acc_high_g.chan = SENSOR_CHAN_ACCEL_XYZ;
    rc = sensor_trigger_set(bno_dev, &trig_acc_high_g, acc_high_g);
    if (rc < 0) return rc;

    /* GYR any-motion */
    config.val1 = 0x02;
    config.val2 = BNO055_GYR_AWAKE_DURATION_MOTION_ANY_8_SAMPLES;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SLOPE_DUR, &config);
    if (rc < 0) return rc;

    config.val1 = 0x08;
    config.val2 = 0;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SLOPE_TH, &config);
    if (rc < 0) return rc;

    trig_gyr_any_motion.type = SENSOR_TRIG_DELTA;
    trig_gyr_any_motion.chan = SENSOR_CHAN_GYRO_XYZ;
    rc = sensor_trigger_set(bno_dev, &trig_gyr_any_motion, gyr_any_motion);
    if (rc < 0) return rc;

    /* GYR high-rate + filter */
    trig_gyr_high_rate.type = BNO055_SENSOR_TRIG_HIGH_RATE;
    trig_gyr_high_rate.chan = SENSOR_CHAN_GYRO_XYZ;
    rc = sensor_trigger_set(bno_dev, &trig_gyr_high_rate, gyr_high_rate);
    if (rc < 0) return rc;

    config.val1 = BNO055_GYR_FILTER_OFF;
    config.val2 = BNO055_GYR_FILTER_OFF;
    rc = sensor_attr_set(bno_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FEATURE_MASK, &config);
    if (rc < 0) return rc;

    printk("BNO055 initialized\n");
    return 0;
}

int bno_get(struct bno_data *out, k_timeout_t timeout) { 
    if (out == NULL) { 
        printk("bno_get: output pointer is NULL\n"); 
        return -1; 
    }
    return k_msgq_get(&bno_msgq, out, timeout);
}