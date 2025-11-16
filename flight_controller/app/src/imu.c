
#include "imu.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>


#define MAX_IMU_MSGQ_LEN 16
#define IMU_STACK_SIZE 1024
#define IMU_SEM_INIT_COUNT 0 
#define IMU_SEM_MAX_COUNT 1
#define IMU_THREAD_PRIORITY 2

K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT)
K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, K_NO_WAIT); 

const char *now_str(void)
{
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



static int process_mpu6050(const struct device *dev, struct imu_data *out) { 
	struct sensor_value temp;
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    int rc;

    rc = sensor_sample_fetch(dev);
    if (rc) {
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (rc) {
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (rc) {
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temp);
    if (rc) {
        return rc;
    }

    out->time = k_uptime_get_32();
    out->temp = sensor_value_to_double(&temp);
    out->accel[0] = sensor_value_to_double(&accel[0]);
    out->accel[1] = sensor_value_to_double(&accel[1]);
    out->accel[2] = sensor_value_to_double(&accel[2]);
    out->gyro[0] = sensor_value_to_double(&gyro[0]);
    out->gyro[1] = sensor_value_to_double(&gyro[1]);
    out->gyro[2] = sensor_value_to_double(&gyro[2]);

    return 0;
}

static struct sensor_trigger trigger;

static void handle_mpu6050_drdy(const struct device *dev,
				const struct sensor_trigger *trig, const struct imu_sem *imu_sem)
{
	k_sem_give(&imu_sem);
}

static void imu_read_thread(void *arg1, void *arg2, void *args3) { 
	const struct device *mpu6050 = (const struct device *)arg1; 
	while(1) { 
		k_sem_take(&imu_sem, K_FOREVER); 
		struct imu_data data; 
		int rc = process_mpu6050(mpu6050, &data); 
		if(rc == 0) { 
			int put_rc = k_msgq_put(&imu_msgq, &data, K_NO_WAIT); 
			if(put_rc != 0) { 
				printk("IMU msgq full, dropping data\n"); 
			}
		} else { 
			printk("Failed to read IMU data: %d\n", rc); 
		}
	}
	
}


int imu_init(const struct device *dev) { 
	int rc;
	if(dev == NULL) {
		printk("IMU device not found\n"); 
		return -1; 
	}
	if(!device_is_ready(dev)) { 
		printk("IMU device not ready\n"); 
		return -1; 
	}
	// k_sem_init(&imu_sem, 0, 1);
	// k_thread_create(&imu_thread_data, imu_stack, K_THREAD_STACK_SIZEOF(imu_stack), imu_read_thread, (void *)dev, NULL, NULL, 1, 0, K_NO_WAIT);
	trigger = (struct sensor_trigger) {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	rc = sensor_trigger_set(dev, &trigger, handle_mpu6050_drdy);
	if(rc < 0) { 
		printk("Failed to set IMU trigger: %d\n", rc); 
		return rc; 
	}
	printk("IMU initialized successfully\n");
	return 0; 
}

int imu_get(struct imu_data *out, k_timeout_t timeout) { 
	if (out == NULL) { 
		printk("imu_get: output pointer is NULL\n");
		return -1; 
	}
	return k_msgq_get(&imu_msgq, out, timeout); 
}
