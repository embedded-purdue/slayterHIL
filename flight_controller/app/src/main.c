#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include "state_machine.h"
#include "imu.h"
#include <stdalign.h>
#include <zephyr/drivers/uart.h>
#include "uart.h"

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
#define UART_CONSUMER_STACK 1024
#define UART_CONSUMER_PRIORITY 2 


K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;

K_THREAD_STACK_DEFINE(imu_consumer_stack, IMU_STACK_SIZE);
static struct k_thread imu_consumer_thread_data;

K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT);
K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, 0);

K_THREAD_STACK_DEFINE(uart_consumer_stack, UART_CONSUMER_STACK);
static struct k_thread uart_consumer_thread_data;
K_MSGQ_DEFINE(uart_rx_msgq, sizeof(struct uart_msg), 16, alignof(struct uart_msg));

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
                default:
                    printk("UART: UNKNOWN '%c'\n", c);
            }
        }
    }
}

int main(void)
{

    const struct device *const uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

    printk("User Command Module ready: %s\n", device_is_ready(uart) ? "YES" : "NO");


    uart_irq_callback_user_data_set(uart, uart_callback,&uart_rx_msgq); //idk what to put for user_data | check if null is okay
    uart_irq_rx_enable(uart);
    
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&uart_consumer_thread_data,
                uart_consumer_stack,
                UART_CONSUMER_STACK,
                uart_consumer,
                NULL, NULL, NULL,
                UART_CONSUMER_PRIORITY,
                0,
                K_NO_WAIT);
	const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);
    int rc = imu_init(mpu6050);
    if (rc != 0) {
        printk("imu_init failed: %d\n", rc);
    }
    k_thread_create(&imu_consumer_thread_data, imu_consumer_stack, IMU_STACK_SIZE, imu_consumer, NULL, NULL, NULL, IMU_PRIORITY, 0, K_NO_WAIT);
	return 0;
}