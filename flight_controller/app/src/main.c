// ...existing code...
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/pwm.h>
#include <stdio.h>
#include <zephyr/drivers/adc.h>
#include "state_machine.h"
#include "imu.h"
#include <stdalign.h>

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
#define SLEEP_DELTA_MSEC 20U

static const struct device *pwm_dev;


K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;

K_THREAD_STACK_DEFINE(imu_consumer_stack, IMU_STACK_SIZE);
static struct k_thread imu_consumer_thread_data;

K_MSGQ_DEFINE(imu_msgq, sizeof(struct imu_data), MAX_IMU_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(imu_sem, IMU_SEM_INIT_COUNT, IMU_SEM_MAX_COUNT)
K_THREAD_DEFINE(imu_thread, IMU_STACK_SIZE, imu_read_thread, NULL, NULL, NULL, IMU_THREAD_PRIORITY, 0, 0);

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(led0));
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(led1));
static const struct pwm_dt_spec pwm_led2 = PWM_DT_SPEC_GET(DT_ALIAS(led2));
static const struct pwm_dt_spec pwm_led3 = PWM_DT_SPEC_GET(DT_ALIAS(led3));
static uint32_t pulse_width_nsec = 0U; 
static uint32_t pulse_width_delta_nsec = 0U;
static uint32_t period_ns = 0U;
static uint32_t steps_taken = 0U; 
static bool increasing_intensity = true; 
int ret;
void test_led1_once(void)
{
    uint32_t period = pwm_led1.period ? pwm_led1.period : (20U * 1000U * 1000U);
    printk("LED1 dev=%s period=%u\n", pwm_led1.dev->name, (unsigned)period);

    int r_on  = pwm_set_dt(&pwm_led1, period, period);      /* full on */
    printk("LED1 full-on ret=%d\n", r_on);
    k_msleep(800);

    // int r_off = pwm_set_dt(&pwm_led1, period, 0);           /* full off */
    // printk("LED1 full-off ret=%d\n", r_off);
    // k_msleep(800);

    // int r_mid = pwm_set_dt(&pwm_led1, period, period/2);    /* half duty */
    // printk("LED1 half ret=%d\n", r_mid);
    // k_msleep(800);
}


void led_delta_timer_handler(struct k_timer *timer_info)
{
    /* set with explicit period to avoid reliance on internal state */
    int r0 = pwm_set_dt(&pwm_led0, period_ns, pulse_width_nsec);
    int r1 = pwm_set_dt(&pwm_led1, period_ns, pulse_width_nsec);
    int r2 = pwm_set_dt(&pwm_led2, period_ns, pulse_width_nsec);
    int r3 = pwm_set_dt(&pwm_led3, period_ns, pulse_width_nsec);
    if (r0 || r1 || r2 || r3) {
        printk("pwm err: led0=%d led1=%d led2=%d led3=%d pulse=%u\n", r0, r1, r2, r3, pulse_width_nsec);
    }

    if (increasing_intensity) {
        if (steps_taken + 1 >= NUM_STEPS) {
            /* reach top next tick -> start decreasing */
            increasing_intensity = false;
        } else {
            steps_taken++;
            pulse_width_nsec += pulse_width_delta_nsec;
            if (pulse_width_nsec > period_ns) {
                pulse_width_nsec = period_ns;
            }
        }
    } else {
        if (steps_taken == 0) {
            increasing_intensity = true;
        } else {
            steps_taken--;
            /* prevent underflow */
            if (pulse_width_nsec > pulse_width_delta_nsec) {
                pulse_width_nsec -= pulse_width_delta_nsec;
            } else {
                pulse_width_nsec = 0U;
            }
        }
    }
}
K_TIMER_DEFINE(led_delta_timer, led_delta_timer_handler, NULL);
>>>>>>> 3b007e3 (PWM LED devicetree set up. Led fade in and out app)

// Demo app for the IMU Consumer 
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
        } else {
            printk("Failed to get IMU data: %d\n", rc);
        }
    }
}
int main(void) {   
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);
    const struct device *const mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);
    // int rc = imu_init(mpu6050);
    if (!pwm_is_ready_dt(&pwm_led0)) {
        printk("pwm_led0 not ready\n");
    } else {
        printk("pwm_led0 dev: %d\n");
    }
    if (!pwm_is_ready_dt(&pwm_led1)) {
        printk("pwm_led1 not ready\n");
    } else {
        printk("pwm_led1 dev: %d\n");
    }
    
    // if (rc != 0) {
    //     printk("imu_init failed: %d\n", rc);
    // }
    k_thread_create(&imu_consumer_thread_data, imu_consumer_stack, IMU_STACK_SIZE, imu_consumer, NULL, NULL, NULL, IMU_PRIORITY, 0, K_NO_WAIT);
   if (pwm_led0.period) {
        period_ns = pwm_led0.period;
    } else if (pwm_led1.period) {
        period_ns = pwm_led1.period;
    } else {
        period_ns = 20U * 1000U * 1000U; /* fallback 20 ms */
    }

    pulse_width_nsec = 0U;
    pulse_width_delta_nsec = period_ns / NUM_STEPS;

    k_timer_start(&led_delta_timer, K_MSEC(SLEEP_DELTA_MSEC), K_MSEC(SLEEP_DELTA_MSEC));
    return 0;
}
