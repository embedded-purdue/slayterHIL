#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/drivers/sensor.h>
#include "state_machine.h"
// #include "imu.h"
#include "lidar.h"
#include "bno.h"
#include "ledCtrl.h"
#include "uart.h"
#include <stdio.h> 
#include <stdalign.h>


// static const struct device *bno055_dev = DEVICE_DT_GET(DT_NODELABEL(bno)); 

// define thread
#define STACK_SIZE      2048
#define COMMS_PRIORITY  1
#define BNO_STACK_SIZE 1024
#define BNO_PRIORITY   3
#define LIDAR_STACK_SIZE 1024
#define LIDAR_PRIORITY 4
#define MAX_BNO_MSGQ_LEN 16
#define MAX_LIDAR_MSGQ_LEN 8
#define BNO_SEM_INIT_COUNT 0 
#define BNO_SEM_MAX_COUNT 1
#define BNO_THREAD_PRIORITY 2
#define UART_CONSUMER_STACK_SIZE 1024
#define UART_CONSUMER_PRIORITY 5
#define TOP_LEFT  1
#define TOP_RIGHT  2
#define BOTTOM_LEFT  0
#define BOTTOM_RIGHT  3

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

K_THREAD_STACK_DEFINE(bno_consumer_stack, BNO_STACK_SIZE);
static struct k_thread bno_consumer_thread_data;


K_MSGQ_DEFINE(bno_msgq, sizeof(struct bno_data), MAX_BNO_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(bno_sem, BNO_SEM_INIT_COUNT, BNO_SEM_MAX_COUNT);


K_THREAD_DEFINE(bno_thread, BNO_STACK_SIZE, bno_read_thread, NULL, NULL, NULL, BNO_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(lidar_thread, LIDAR_STACK_SIZE, lidar_read_thread, NULL, NULL, NULL, LIDAR_PRIORITY, 0, 0);

//uart section
K_THREAD_STACK_DEFINE(uart_consumer_stack, UART_CONSUMER_STACK_SIZE);
static struct k_thread uart_consumer_thread_data;
K_MSGQ_DEFINE(uart_rx_msgq, sizeof(struct uart_msg), 16, alignof(struct uart_msg));


// Demo app for the BNO Consumer 
static void bno_consumer(void *arg1, void *arg2, void *arg3) {
    struct bno_data out; 
    printk("BNO consumer thread started - Motor Emulation Mode\n");
    
    while (1) {
        int rc = bno_get(&out, K_MSEC(100));
        if (rc != 0) {
            // No BNO data available, set LEDs to idle
            for (int i = 0; i < 4; i++) {
                set_led_intensity(i, 10);  // Dim idle state
            }
            k_msleep(50);
            continue;
        }

        // Convert radians to degrees for easier tuning
        float pitch_deg = out.eul.pitch * 180.0f / M_PI;
        float roll_deg = out.eul.roll * 180.0f / M_PI;

        // Base throttle (0-100)
        int base = 50;
        
        // Gains control how much tilt affects motor differential
        float pitch_gain = 0.8f;
        float roll_gain = 0.8f;

        // Quadcopter motor mixing:
        // Pitch+ → nose down → front motors up, rear down
        // Roll+ → right tilt → right motors up, left down
        int front_left  = base + (int)(pitch_gain * pitch_deg) + (int)(roll_gain * roll_deg);
        int front_right = base + (int)(pitch_gain * pitch_deg) - (int)(roll_gain * roll_deg);
        int back_left   = base - (int)(pitch_gain * pitch_deg) + (int)(roll_gain * roll_deg);
        int back_right  = base - (int)(pitch_gain * pitch_deg) - (int)(roll_gain * roll_deg);

        // Clamp to 0-100 range
        if (front_left < 0) front_left = 0; else if (front_left > 100) front_left = 100;
        if (front_right < 0) front_right = 0; else if (front_right > 100) front_right = 100;
        if (back_left < 0) back_left = 0; else if (back_left > 100) back_left = 100;
        if (back_right < 0) back_right = 0; else if (back_right > 100) back_right = 100;

        // Map to your LED position defines
        set_led_intensity(TOP_LEFT, front_left);      // 1
        set_led_intensity(TOP_RIGHT, front_right);    // 2
        set_led_intensity(BOTTOM_LEFT, back_left);    // 0
        set_led_intensity(BOTTOM_RIGHT, back_right);  // 3

        // Debug print every 10th iteration (~500ms)
        static int print_counter = 0;
        if (++print_counter >= 10) {
            printk("[%s] P:%.1f R:%.1f Y:%.1f | Motors FL:%d FR:%d BL:%d BR:%d\n",
                   now_str(),
                   (double)pitch_deg, (double)roll_deg, (double)(out.eul.yaw * 180.0f / M_PI),
                   front_left, front_right, back_left, back_right);
            print_counter = 0;
        }

        k_msleep(50);  // 20 Hz control loop
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
    
    printk("Starting LiDAR-Lite V4 and BNO test app...\n");

    //Initialize BNO
    const struct device *const bno = DEVICE_DT_GET(DT_NODELABEL(bno));
    int rc = bno_init(bno);
    if (rc != 0) {
        printk("bno_init failed: %d\n", rc);
        return rc;
    }

    // // Initialize LiDAR
    const struct device *const lidar_device = DEVICE_DT_GET(DT_NODELABEL(lidar));
    rc = lidar_init(lidar_device); 
    if(rc != 0) { 
        printk("lidar_init failed: %d\n", rc);
        // return rc;
    }

    //Initialize UART
    const struct device *const uart_control = DEVICE_DT_GET(DT_NODELABEL(uart1));
    int uc = uart_init(uart_control);
    if(uc != 0){
        printk("uart_init failed: %d\n",uc);
    }
    initialize_leds(); 
    printk("Leds initialized\n");



    // Start state machine thread
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);

    // Start BNO consumer thread (the one that prints data)
    k_thread_create(&bno_consumer_thread_data, 
                    bno_consumer_stack, 
                    BNO_STACK_SIZE, 
                    bno_consumer, 
                    NULL, NULL, NULL, 
                    BNO_PRIORITY, 0, K_NO_WAIT);
     // Start LiDAR consumer thread (the one that prints data)
    k_thread_create(&lidar_consumer_thread_data, 
                    lidar_consumer_stack, 
                    LIDAR_STACK_SIZE, 
                    lidar_consumer, 
                    NULL, NULL, NULL, 
                    LIDAR_PRIORITY, 0, K_NO_WAIT);

    // Start UART consumer thread (the one that prints data)
    k_thread_create(&uart_consumer_thread_data, 
                    uart_consumer_stack, 
                    UART_CONSUMER_STACK_SIZE, 
                    uart_consumer, 
                    NULL, NULL, NULL,
                    UART_CONSUMER_PRIORITY, 0, K_NO_WAIT);

    printk("All threads started\n");

    // Start LiDAR timer after everything is initialized
    k_timer_start(&lidar_timer, K_MSEC(30), K_MSEC(30));
    
    printk("LiDAR timer started, monitoring data...\n");
    while(1) { 
        for(int i = 0; i < 100; i++) { 
            // set_led_intensity(TOP_LEFT, i); 
            // set_led_intensity(TOP_RIGHT, i); 
            // set_led_intensity(BOTTOM_LEFT, i); 
            // set_led_intensity(BOTTOM_RIGHT, i); 
            k_msleep(10);
        }
        k_msleep(1000); 
        for(int i = 100; i>=0; i--) { 
            // set_led_intensity(TOP_LEFT, i); 
            // set_led_intensity(TOP_RIGHT, i); 
            // set_led_intensity(BOTTOM_LEFT, i); 
            // set_led_intensity(BOTTOM_RIGHT, i); 
            k_msleep(10);
        }
    }
    return 0;
}
