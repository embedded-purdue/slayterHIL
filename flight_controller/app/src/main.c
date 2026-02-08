#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/drivers/sensor.h>
#include "state_machine.h"
#include "imu.h"
#include <stdalign.h>
#include <bno055.h>
// // define thread
// #define STACK_SIZE      2048
// #define COMMS_PRIORITY  1
// #define IMU_STACK_SIZE 1024
// #define IMU_PRIORITY   3
// #define MAX_IMU_MSGQ_LEN 16
// #define IMU_STACK_SIZE 1024
// #define IMU_SEM_INIT_COUNT 0 
// #define IMU_SEM_MAX_COUNT 1
// #define IMU_THREAD_PRIORITY 2

// //lidar section
// K_SEM_DEFINE(lidar_sem, 0, 1);

// struct lidar_msg {
//     uint16_t distance_mm;
// };
// K_MSGQ_DEFINE(lidar_msgq, sizeof(struct lidar_msg), 8, 4);

// /* ======= Timer ISR ======= */
// void lidar_timer_handler(struct k_timer *timer_id)
// {
//     k_sem_give(&lidar_sem);
// }
// K_TIMER_DEFINE(lidar_timer, lidar_timer_handler, NULL);

// /* ======= LiDAR Read Thread ======= */
// void lidar_thread(void)
// {
//     const struct device *dev = DEVICE_DT_GET_ONE(grmn_lidar_lite_v4);
//     struct sensor_value val;
//     struct lidar_msg msg;

//     if (!device_is_ready(dev)) {
//         printk("LIDAR device not ready!\n");
//         return;
//     }

//     while (1) {
//         /* Wait until ISR releases semaphore */
//         k_sem_take(&lidar_sem, K_FOREVER);

//         if (sensor_sample_fetch(dev) == 0 &&
//             sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &val) == 0) {
//             msg.distance_mm = val.val1 * 1000 + val.val2 / 1000;
//             k_msgq_put(&lidar_msgq, &msg, K_NO_WAIT);
//         }
//     }
// }
// K_THREAD_DEFINE(lidar_tid, 2048, lidar_thread, NULL, NULL, NULL,
//                 7, 0, 0);

// //imu section
// K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
// static struct k_thread state_machine_thread_data;

// K_THREAD_STACK_DEFINE(imu_consumer_stack, IMU_STACK_SIZE);
// static struct k_thread imu_consumer_thread_data;

// K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
// K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT)
// K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, 0);

// // Demo app for the IMU Consumer 
// static void imu_consumer(void *arg1, void *arg2, void *arg3) {
//     struct imu_data out; 
//     while (1) {
//         // struct imu_data out;
//         int rc = imu_get(&out, K_FOREVER);
//         if (rc == 0) {
//             printk("[%s] Temp: %.2f C, Accel: [%.2f, %.2f, %.2f] m/s², Gyro: [%.2f, %.2f, %.2f] dps\n",
//                    now_str(), out.temp,
//                    out.accel[0], out.accel[1], out.accel[2],
//                    out.gyro[0], out.gyro[1], out.gyro[2]);
//         } else {
//             printk("Failed to get IMU data: %d\n", rc);
//         }
//     }
// }






static const struct device *bno055_dev = DEVICE_DT_GET(DT_NODELABEL(bno)); 


int main(void)
{   
    #if Z_DEVICE_DT_FLAGS(DT_NODELABEL(bno)) & DEVICE_FLAG_INIT_DEFERRED
        k_sleep(K_MSEC(BNO055_TIMING_STARTUP)); 
        device_init(bno055_dev); 
    #endif

    if (!device_is_ready(bno055_dev)) {
        printk("BNO055 device not ready!\n");
        return -1;
    }

    printk("BNO055 initialized"); 
    // Set operation mode to NDOF (fusion mode) gives Euler, Quarternion, Linear Accel, Gravity. 
    struct sensor_value mode = {.val1 = BNO055_MODE_NDOF, .val2 = 0}; 
    sensor_attr_set(bno055_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CONFIGURATION, &mode); 

    //Set power mode 
    struct sensor_value pwr = {.val1 = BNO055_PWR_MODE_NORMAL, .val2 = 0};
    sensor_attr_set(bno055_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CONFIGURATION, &pwr);


    while(1) { 
        if(sensor_sample_fetch(bno055_dev) < 0) { 
            printk("Fetch failed\n"); 
            continue; 
        }

        struct sensor_value euler[3], lia[3], grav[3]; 
        sensor_channel_get(bno055_dev, SENSOR_CHAN_EULER, euler);
        sensor_channel_get(bno055_dev, SENSOR_CHAN_LINEAR_ACCEL, lia);
        sensor_channel_get(bno055_dev, SENSOR_CHAN_GRAVITY, grav);

        printk("Euler Y:%d.%06d R:%d.%06d P:%d.%06d rad\n",
               euler[0].val1, euler[0].val2,
               euler[1].val1, euler[1].val2,
               euler[2].val1, euler[2].val2);
               
        printk("Linear Accel X:%d.%06d Y:%d.%06d Z:%d.%06d m/s²\n",
               lia[0].val1, lia[0].val2,
               lia[1].val1, lia[1].val2,
               lia[2].val1, lia[2].val2);
        k_sleep(K_MSEC(1000));
    }

	return 0;
    

}