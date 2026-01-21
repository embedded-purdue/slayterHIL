// ...existing code...
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/pwm.h>
#include <stdio.h>
#include <zephyr/drivers/adc.h>
#include "state_machine.h"
#include "imu.h"
#include <stdalign.h>
#include "ledCtrl.h"
#include <math.h>
#include "kalman.h"
// define thread
#define STACK_SIZE      2048
#define COMMS_PRIORITY  1
#define IMU_STACK_SIZE 1024
#define IMU_PRIORITY   3
#define MAX_IMU_MSGQ_LEN 16
#define IMU_SEM_INIT_COUNT 0 
#define IMU_SEM_MAX_COUNT 1
#define IMU_THREAD_PRIORITY 2
#define NUM_STEPS 100U
#define SLEEP_DELTA_MSEC 200
#define TOP_LEFT  2
#define TOP_RIGHT  3
#define BOTTOM_LEFT  1
#define BOTTOM_RIGHT  0
#define X_BIAS 1.3
static const struct device *pwm_dev;


K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;

K_THREAD_STACK_DEFINE(imu_consumer_stack, IMU_STACK_SIZE);
static struct k_thread imu_consumer_thread_data;

K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT)
K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, 0);

static void imu_consumer(void *arg1, void *arg2, void *arg3) {
    struct imu_data out;
    while (1) {
        int rc = imu_get(&out, K_FOREVER);
        if (rc == 0) {
            // printk("[%s] Temp: %.2f C, Accel: [%.2f, %.2f, %.2f] m/s², Gyro: [%.2f, %.2f, %.2f] dps\n",
            //        now_str(), out.temp,
            //        out.accel[0], out.accel[1], out.accel[2],
            //        out.gyro[0], out.gyro[1], out.gyro[2]);
            printk(">Accel x: %.2f\n", out.accel[0]);
            printk(">Accel y: %.2f\n", out.accel[1]);
            printk(">Accel z: %.2f\n", out.accel[2]);
            printk(">Gyro x: %.2f\n", out.gyro[0]);
            printk(">Gyro y: %.2f\n", out.gyro[1]);
            printk(">Gyro z: %.2f\n", out.gyro[2]);
            driftx += out.gyro[0];
            drifty += out.gyro[1];
            printk(">Drift x: %d\n", driftx);
            printk(">Drift y: %d\n", drifty);
        } else {
            printk("Failed to get IMU data: %d\n", rc);
            continue;
        }

        k_msleep(100); /* update rate; tune as needed */
    }
}

int main(void) {   
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);
    const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);
    int rc = imu_init(mpu6050);
    if (rc != 0) {
        printk("imu_init failed: %d\n", rc);
    }
    k_thread_create(&imu_consumer_thread_data, imu_consumer_stack, IMU_STACK_SIZE, imu_consumer, NULL, NULL, NULL, IMU_PRIORITY, 0, K_NO_WAIT);
    initialize_leds();
    while(1) { 
        for(int i = 0; i < 100; i++) {
            set_led_intensity(TOP_LEFT,i);
            set_led_intensity(TOP_RIGHT,i);
            set_led_intensity(BOTTOM_LEFT,i);
            set_led_intensity(BOTTOM_RIGHT,i);
            k_msleep(10);
        }
        k_msleep(1000);
        for(int i = 100; i >= 0; i--) {
            set_led_intensity(TOP_LEFT,i);
            set_led_intensity(TOP_RIGHT,i);
            set_led_intensity(BOTTOM_LEFT,i);
            set_led_intensity(BOTTOM_RIGHT,i);
            k_msleep(10);
        }
        k_msleep(1000);
    }
    return 0;
}
