#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

/* ======= Synchronization Objects ======= */
K_SEM_DEFINE(lidar_sem, 0, 1);

struct lidar_msg {
    uint16_t distance_mm;
};
K_MSGQ_DEFINE(lidar_msgq, sizeof(struct lidar_msg), 8, 4);

/* ======= Timer ISR ======= */
void lidar_timer_handler(struct k_timer *timer_id)
{
    k_sem_give(&lidar_sem);
}
K_TIMER_DEFINE(lidar_timer, lidar_timer_handler, NULL);

/* ======= LiDAR Read Thread ======= */
void lidar_thread(void)
{
    const struct device *dev = DEVICE_DT_GET_ONE(grmn_lidar_lite_v4);
    struct sensor_value val;
    struct lidar_msg msg;

    if (!device_is_ready(dev)) {
        printk("LIDAR device not ready!\n");
        return;
    }

    while (1) {
        /* Wait until ISR releases semaphore */
        k_sem_take(&lidar_sem, K_FOREVER);

        if (sensor_sample_fetch(dev) == 0 &&
            sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &val) == 0) {
            msg.distance_mm = val.val1 * 1000 + val.val2 / 1000;
            k_msgq_put(&lidar_msgq, &msg, K_NO_WAIT);
        }
    }
}
K_THREAD_DEFINE(lidar_tid, 2048, lidar_thread, NULL, NULL, NULL,
                7, 0, 0);

/* ======= Main Thread ======= */
void main(void)
{
    struct lidar_msg msg;

    printk("Starting LiDAR-Lite V4 test app...\n");

    /* Timer period = 50 ms ⇒ 20 Hz polling rate */
    k_timer_start(&lidar_timer, K_MSEC(30), K_MSEC(30));

    while (1) {
        //printk("test to see if we get msg\n");
        if (k_msgq_get(&lidar_msgq, &msg, K_FOREVER) == 0) {
            printk("Distance: %u mm\n", msg.distance_mm);

        }
    }
#include <zephyr/sys/printk.h> 
#include "state_machine.h"
#include "imu.h"
#include <stdalign.h>

// define thread
#define STACK_SIZE      2048
#define COMMS_PRIORITY  1
#define IMU_STACK_SIZE 1024
#define IMU_PRIORITY   3
#define MAX_IMU_MSGQ_LEN 16
#define IMU_STACK_SIZE 1024
#define IMU_SEM_INIT_COUNT 0 
#define IMU_SEM_MAX_COUNT 1
#define IMU_THREAD_PRIORITY 2


K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;

K_THREAD_STACK_DEFINE(imu_consumer_stack, IMU_STACK_SIZE);
static struct k_thread imu_consumer_thread_data;

K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT)
K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, 0);

// Demo app for the IMU Consumer 
static void imu_consumer(void *arg1, void *arg2, void *arg3) {
    struct imu_data out; 
    while (1) {
        // struct imu_data out;
        int rc = imu_get(&out, K_FOREVER);
        if (rc == 0) {
            printk("[%s] Temp: %.2f C, Accel: [%.2f, %.2f, %.2f] m/s², Gyro: [%.2f, %.2f, %.2f] dps\n",
                   now_str(), out.temp,
                   out.accel[0], out.accel[1], out.accel[2],
                   out.gyro[0], out.gyro[1], out.gyro[2]);
        } else {
            printk("Failed to get IMU data: %d\n", rc);
        }
    }
}
int main(void)
{
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);
	const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);
    int rc = imu_init(mpu6050);
    if (rc != 0) {
        printk("imu_init failed: %d\n", rc);
    }
    k_thread_create(&imu_consumer_thread_data, imu_consumer_stack, IMU_STACK_SIZE, imu_consumer, NULL, NULL, NULL, IMU_PRIORITY, 0, K_NO_WAIT);
	return 0;
}