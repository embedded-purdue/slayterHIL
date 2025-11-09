/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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


K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), 16, 4);
static struct k_sem imu_sem; 

K_THREAD_STACK_DEFINE(imu_stack, 1024); 
static struct k_thread imu_thread_data; 

static const char *now_str(void)
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

	int rc = sensor_sample_fetch(dev); 
	if(rc == 0) { 
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel); 
	}
	if(rc == 0) { 
		rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro); 
	}
	if(rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temp); 
	}
	if(rc == 0) { 
		out->time = k_uptime_get_32(); 
		out->temp = sensor_value_to_double(&temp); 
		out->accel[0] = sensor_value_to_double(&accel[0]); 
		out->accel[1] = sensor_value_to_double(&accel[1]); 
		out->accel[2] = sensor_value_to_double(&accel[2]); 
		out->gyro[0] = sensor_value_to_double(&gyro[0]); 
		out->gyro[1] = sensor_value_to_double(&gyro[1]); 
		out->gyro[2] = sensor_value_to_double(&gyro[2]); 
	} else { 
		printf("sample fetch/get failed: %d\n", rc); 
	}
	return rc;
}

static struct sensor_trigger trigger;

static void handle_mpu6050_drdy(const struct device *dev,
				const struct sensor_trigger *trig)
{
   
	// int rc = process_mpu6050(. /dev);

	// if (rc != 0) {
	// 	printf("cancelling trigger due to failure: %d\n", rc);
	// 	(void)sensor_trigger_set(dev, trig, NULL);
	// 	return;
	// }
	// return rc; 
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
				printf("IMU msgq full, dropping data\n"); 
			}
		} else { 
			printf("Failed to read IMU data: %d\n", rc); 
		}
	}
	
}


int main(void)
{

	const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);

	if (!device_is_ready(mpu6050)) {
		printf("Device %s is not ready\n", mpu6050->name);
		return 0;
	}

	printf("Device %s is ready\n", mpu6050->name); 
	k_sem_init(&imu_sem, 0, 1); 
	k_thread_create(&imu_thread_data, imu_stack, K_THREAD_STACK_SIZEOF(imu_stack), imu_read_thread, (void *)mpu6050, NULL, NULL, 1, 0, K_NO_WAIT);
	
	
	trigger = (struct sensor_trigger) {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	 
	// while(1) { 
	// 	int rc = process_mpu6050(mpu6050);
	// 	if (rc != 0) {
	// 		return 0;
	// 	}
	// 	k_sleep(K_MSEC(1000));
	// }
	
	if (sensor_trigger_set(mpu6050, &trigger,
			       handle_mpu6050_drdy) < 0) {
		printf("Cannot configure trigger\n");
		return 0;
	}	
	printk("Configured for triggered sampling.\n");

	// while (!IS_ENABLED(CONFIG_MPU6050_TRIGGER)) {
	// 	int rc = process_mpu6050(mpu6050);

	// 	if (rc != 0) {
	// 		break;
	// 	}
	// 	k_sleep(K_MSEC(1000));
	// 
	while(1)
 	{ 
		struct imu_data data;
		int get_rc = k_msgq_get(&imu_msgq, &data, K_FOREVER);
		if(get_rc == 0) {
			printf("[%s] Temp: %.2f C, Accel: [%.2f, %.2f, %.2f] m/s², Gyro: [%.2f, %.2f, %.2f] dps\n",
				now_str(),
				data.temp,
				data.accel[0], data.accel[1], data.accel[2],
				data.gyro[0], data.gyro[1], data.gyro[2]);
		} else { 
			printf("Failed to get IMU data from msgq: %d\n", get_rc); 
		}
		k_sleep(K_MSEC(200)); 
 	}
    


	/* triggered runs with its own thread after exit */
	return 0;
}
