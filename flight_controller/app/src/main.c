#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/drivers/sensor.h>
#include "state_machine.h"
#include "imu.h"
#include "lidar.h"
#include "uart.h"
#include <stdio.h> 
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
#define UART_CONSUMER_STACK 1024
#define UART_CONSUMER_PRIORITY 5


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

//uart section
K_THREAD_STACK_DEFINE(uart_consumer_stack, UART_CONSUMER_STACK);
static struct k_thread uart_consumer_thread_data;
K_MSGQ_DEFINE(uart_rx_msgq, sizeof(struct uart_msg), 16, alignof(struct uart_msg));


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

static void uart_consumer(void *arg1,  void *arg2, void *arg3) {
    struct uart_msg out;
    printk("User Control thread started\n");
    while(1)
    {
        if(k_msgq_get(&uart_rx_msgq, &out, K_FOREVER) == 0)
        {
            char c = out.data[0];
                switch (c) {
                case 'L':
                    printk("UART: LEFT\n");
                    break;
                case 'R':
                    printk("UART: RIGHT\n");
                    break;
                case 'U':
                    printk("UART: UP\n");
                    break;
                case 'D':
                    printk("UART: DOWN\n");
                    break;
                case 'A':
                    printk("UART: ASCEND\n");
                    break;
                case 'X':
                    printk("UART: DESCEND\n");
                    break;
                default:
                    printk("UART: UNKNOWN '%c'\n", c);
            }
        }
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

    //Initialize IMU 
    const struct device *const mpu6050 = DEVICE_DT_GET(DT_NODELABEL(mpu6050));
    int rc = imu_init(mpu6050);
    if (rc != 0) {
        printk("imu_init failed: %d\n", rc);
        return rc;
    }

    // // Initialize LiDAR
    const struct device *const lidar_device = DEVICE_DT_GET(DT_NODELABEL(lidar));
    rc = lidar_init(lidar_device); 
    if(rc != 0) { 
        printk("lidar_init failed: %d\n", rc);
        return rc;
    }

    //Initialize UART
    const struct device *const uart_control = DEVICE_DT_GET(DT_NODELABEL(uart1));
    int uc = uart_init(uart_control);
    if(uc != 0){
        printk("uart_init failed: %d\n",uc);
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

    // Start UART consumer thread (the one that prints data)
    k_thread_create(&uart_consumer_thread_data, uart_consumer_stack, UART_CONSUMER_STACK, uart_consumer, NULL, NULL, NULL,UART_CONSUMER_PRIORITY, 0, K_NO_WAIT);

    printk("All threads started\n");

    // Start LiDAR timer after everything is initialized
    k_timer_start(&lidar_timer, K_MSEC(30), K_MSEC(30));
    
    printk("LiDAR timer started, monitoring data...\n");
    return 0;
}