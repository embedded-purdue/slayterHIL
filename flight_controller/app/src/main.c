#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/drivers/sensor.h>
#include "state_machine.h"
#include "imu.h"
#include "lidar.h"
#include <stdio.h> 
#include <stdalign.h>
#include <stdalign.h>

// define thread
#define STACK_SIZE      2048
#define COMMS_PRIORITY  1
#define IMU_STACK_SIZE 1024
#define IMU_PRIORITY   3
#define LIDAR_STACK_SIZE 1024
#define LIDAR_PRIORITY 4
#define MAX_IMU_MSGQ_LEN 16
#define MAX_LIDAR_MSGQ_LEN 8
#define IMU_SEM_INIT_COUNT 0 
#define IMU_SEM_MAX_COUNT 1
#define IMU_THREAD_PRIORITY 2

//lidar section
K_SEM_DEFINE(lidar_sem, 0, 1);
K_MSGQ_DEFINE(lidar_msgq, sizeof(struct lidar_data), MAX_LIDAR_MSGQ_LEN, alignof(struct lidar_data));
K_TIMER_DEFINE(lidar_timer, lidar_timer_handler, NULL);
//LiDAR stack definiton 
K_THREAD_STACK_DEFINE(lidar_consumer_stack, LIDAR_STACK_SIZE); 
static struct k_thread lidar_consumer_thread_data;

//imu section
K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;

K_THREAD_STACK_DEFINE(imu_consumer_stack, IMU_STACK_SIZE);
static struct k_thread imu_consumer_thread_data;


K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT);


K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(lidar_thread, LIDAR_STACK_SIZE, lidar_read_thread, NULL, NULL, NULL, LIDAR_PRIORITY, 0, 0);



// Demo app for the IMU Consumer 
static void imu_consumer(void *arg1, void *arg2, void *arg3) {
    struct imu_data out; 
    printk("IMU consumer thread started\n");
    
    while (1) {
        int rc = imu_get(&out, K_FOREVER);
        if (rc == 0) {
            printk("[%s] Temp: %.2f C, Accel: [%.2f, %.2f, %.2f] m/s², Gyro: [%.2f, %.2f, %.2f] dps\n",
                   now_str(), out.temp,
                   out.accel[0], out.accel[1], out.accel[2],
                   out.gyro[0], out.gyro[1], out.gyro[2]);
        } else {
            printk("Failed to get IMU data: %d\n", rc);
        }
        k_msleep(100); // Add small delay to prevent spam
    }
}

static void lidar_consumer(void *arg1, void *arg2, void *arg3) {
    struct lidar_data out; 
    printk("LiDAR consumer thread started\n");
    
    while (1) {
        int rc = k_msgq_get(&lidar_msgq, &out, K_FOREVER);
        if (rc == 0) {
            printk("[%s] LiDAR Distance: %u mm\n", now_str(), out.distance_mm);
        } else {
            printk("Failed to get LiDAR data: %d\n", rc);
        }
        k_msleep(100);
    }
}

int main(void)
{
    // printk("Checking I2C devices...\n");

    const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

    printk("I2C0 ready: %s\n", device_is_ready(i2c0_dev) ? "YES" : "NO");
    printk("I2C1 ready: %s\n", device_is_ready(i2c1_dev) ? "YES" : "NO");
    
    printk("Starting LiDAR-Lite V4 and IMU test app...\n");

    // Initialize IMU 
    const struct device *const mpu6050 = DEVICE_DT_GET_ONE(DT_NODELABEL(mpu6050));
    int rc = imu_init(mpu6050);
    if (rc != 0) {
        printk("imu_init failed: %d\n", rc);
        return rc;
    }

    // Initialize LiDAR
    const struct device *const lidar_device = DEVICE_DT_GET(DT_NODELABEL(lidar));
    rc = lidar_init(lidar_device); 
    if(rc != 0) { 
        printk("lidar_init failed: %d\n", rc);
        return rc;
    }

    // Start state machine thread
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);

    // Start IMU consumer thread (the one that prints data)
    k_thread_create(&imu_consumer_thread_data, 
                    imu_consumer_stack, 
                    IMU_STACK_SIZE, 
                    imu_consumer, 
                    NULL, NULL, NULL, 
                    IMU_PRIORITY, 0, K_NO_WAIT);
     // Start LiDAR consumer thread (the one that prints data)
    k_thread_create(&lidar_consumer_thread_data, 
                    lidar_consumer_stack, 
                    LIDAR_STACK_SIZE, 
                    lidar_consumer, 
                    NULL, NULL, NULL, 
                    LIDAR_PRIORITY, 0, K_NO_WAIT);

    printk("All threads started\n");

    // Start LiDAR timer after everything is initialized
    k_timer_start(&lidar_timer, K_MSEC(30), K_MSEC(30));
    
    printk("LiDAR timer started, monitoring data...\n");
    return 0;
}