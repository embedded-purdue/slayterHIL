#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/drivers/sensor.h>
#include "state_machine.h"
#include "imu.h"
#include "lidar.h"
#include "ledCtrl.h"
#include <stdio.h> 
#include <stdbool.h>
#include <stdalign.h>

#define LED_COUNT 4
#define LIDAR_BRIGHT_MM 10U  // min distance
#define LIDAR_DIM_MM 3000U  // max distance
#define LIDAR_DIM_INTENSITY 10
#define LIDAR_SAMPLE_HZ 33U
#define LIDAR_SAMPLE_PERIOD_US (1000000U / LIDAR_SAMPLE_HZ)

static int intensity_from_distance_mm(uint16_t distance_mm)
{
    if (distance_mm <= LIDAR_BRIGHT_MM) {
        return 100;
    }

    if (distance_mm >= LIDAR_DIM_MM) {
        return LIDAR_DIM_INTENSITY;
    }

    uint32_t span = LIDAR_DIM_MM - LIDAR_BRIGHT_MM;
    uint32_t remaining = LIDAR_DIM_MM - distance_mm;

    return LIDAR_DIM_INTENSITY +
           (int)((remaining * (100U - (uint32_t)LIDAR_DIM_INTENSITY)) / span);
}

static void led_startup_test(void)
{
    printk("Running LED startup test...\n");

    for (int i = 0; i < LED_COUNT; ++i) {
        set_led_intensity(i, 100);
    }
    k_msleep(600);

    for (int i = 0; i < LED_COUNT; ++i) {
        set_led_intensity(i, 0);
    }

    printk("LED startup test complete\n");
}

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
            int intensity = intensity_from_distance_mm(out.distance_mm);

            for (int i = 0; i < LED_COUNT; ++i) {
                set_led_intensity(i, intensity);
            }

            printk("[%s] LiDAR Distance: %u mm (LED intensity: %d%%)\n",
                   now_str(), out.distance_mm, intensity);
        } else {
            printk("Failed to get LiDAR data: %d\n", rc);
        }
        k_msleep(100);
    }
}

int main(void)
{
    bool imu_ready = false;
#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_FAMILY_ESPRESSIF_ESP32)
    const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

    printk("I2C0 ready: %s\n", device_is_ready(i2c0_dev) ? "YES" : "NO");
    printk("I2C1 ready: %s\n", device_is_ready(i2c1_dev) ? "YES" : "NO");

    printk("Starting LiDAR-Lite V4 test app...\n");

    const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);
    int rc = imu_init(mpu6050);
    if (rc != 0) {
        printk("IMU not available, continuing without IMU (rc=%d)\n", rc);
    } else {
        imu_ready = true;
    }

    const struct device *const lidar_device = DEVICE_DT_GET(DT_NODELABEL(lidar));
    rc = lidar_init(lidar_device);
    if (rc != 0) {
        printk("lidar_init failed: %d\n", rc);
        return rc;
    }

    initialize_leds();
    led_startup_test();
#else
    printk("Non-ESP32 build: LiDAR/PWM hardware init disabled\n");
#endif

    // Start state machine thread
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);

    // Start IMU consumer thread (the one that prints data)
    if (imu_ready) {
        k_thread_create(&imu_consumer_thread_data,
                        imu_consumer_stack,
                        IMU_STACK_SIZE,
                        imu_consumer,
                        NULL, NULL, NULL,
                        IMU_PRIORITY, 0, K_NO_WAIT);
    } else {
        printk("IMU consumer not started\n");
    }
     // Start LiDAR consumer thread (the one that prints data)
    k_thread_create(&lidar_consumer_thread_data, 
                    lidar_consumer_stack, 
                    LIDAR_STACK_SIZE, 
                    lidar_consumer, 
                    NULL, NULL, NULL, 
                    LIDAR_PRIORITY, 0, K_NO_WAIT);

    printk("All threads started\n");

#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_FAMILY_ESPRESSIF_ESP32)
    k_timer_start(&lidar_timer, K_USEC(LIDAR_SAMPLE_PERIOD_US), K_USEC(LIDAR_SAMPLE_PERIOD_US));
    printk("LiDAR timer started at %u Hz, monitoring data...\n", LIDAR_SAMPLE_HZ);
#endif

    return 0;
}