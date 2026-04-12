#include "pwm.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_thread, LOG_LEVEL_INF);

const struct device *pwm_input_dev1 = DEVICE_DT_GET(DT_ALIAS(pwm_input1));
const struct device *pwm_input_dev2 = DEVICE_DT_GET(DT_ALIAS(pwm_input2));
const struct device *pwm_input_dev3 = DEVICE_DT_GET(DT_ALIAS(pwm_input3));
const struct device *pwm_input_dev4 = DEVICE_DT_GET(DT_ALIAS(pwm_input4));
const struct device *pwm_output_dev = NULL; // TODO: assign correct device

// shared voltages updated by pwm_thread, read by spi_fs thread
#define NUM_MOTORS 4
static float latest_voltages[NUM_MOTORS];
static struct k_mutex voltage_mutex;

void pwm_get_latest_voltages(float *out, int count) {
    k_mutex_lock(&voltage_mutex, K_FOREVER);
    for (int i = 0; i < count && i < NUM_MOTORS; i++) {
        out[i] = latest_voltages[i];
    }
    k_mutex_unlock(&voltage_mutex);
}

K_THREAD_STACK_DEFINE(pwm_stack, PWM_STACK_SIZE);
struct k_thread pwm_thread_data;

void pwm_thread(void*, void*, void*) {
    uint64_t pwm_period1;
    uint64_t pwm_pulse1;
    uint64_t pwm_period2;
    uint64_t pwm_pulse2;
    uint64_t pwm_period3;
    uint64_t pwm_pulse3;
    uint64_t pwm_period4;
    uint64_t pwm_pulse4;

    while(1) {
        if(pwm_capture_usec(pwm_input_dev1, 0, &pwm_period1, &pwm_pulse1) == 0 &&
           pwm_capture_usec(pwm_input_dev2, 0, &pwm_period2, &pwm_pulse2) == 0 &&
           pwm_capture_usec(pwm_input_dev3, 0, &pwm_period3, &pwm_pulse3) == 0 &&
           pwm_capture_usec(pwm_input_dev4, 0, &pwm_period4, &pwm_pulse4) == 0) {
            // dummy printing for sanity
            LOG_INF("PWM captured: period = %llu us, pulse = %llu us", pwm_period, pwm_pulse);

            double duty_cycle1 = (double)pwm_pulse1 / (double)pwm_period1;
            double avg_voltage1 = duty_cycle1 * VOLTAGE_HIGH;
            double duty_cycle2 = (double)pwm_pulse2 / (double)pwm_period2;
            double avg_voltage2 = duty_cycle2 * VOLTAGE_HIGH;
            double duty_cycle3 = (double)pwm_pulse3 / (double)pwm_period3;
            double avg_voltage3 = duty_cycle3 * VOLTAGE_HIGH;
            double duty_cycle4 = (double)pwm_pulse4 / (double)pwm_period4;
            double avg_voltage4 = duty_cycle4 * VOLTAGE_HIGH;

            // update shared voltages for SPI thread to send to FS
            k_mutex_lock(&voltage_mutex, K_FOREVER);
            latest_voltages[0] = (float)avg_voltage1;
            latest_voltages[1] = (float)avg_voltage2;
            latest_voltages[2] = (float)avg_voltage3;
            latest_voltages[3] = (float)avg_voltage4;
            k_mutex_unlock(&voltage_mutex);
        } else {
            LOG_ERR("Failed to capture PWM signal");
        }
    }
}

void pwm_init() {
    k_mutex_init(&voltage_mutex);

    if (!device_is_ready(pwm_input_dev1) ||
        !device_is_ready(pwm_input_dev2) ||
        !device_is_ready(pwm_input_dev3) ||
        !device_is_ready(pwm_input_dev4)) {
        LOG_ERR("PWM input device is not ready");
        return;
    }

    if(!device_is_ready(pwm_output_dev)) {
        LOG_ERR("PWM output device is not ready");
        return;
    }

    k_thread_create(&pwm_thread_data, pwm_stack, 
                    K_THREAD_STACK_SIZEOF(pwm_stack),
                    pwm_thread, 
                    NULL, NULL, NULL,
                    PWM_PRIORITY, 0, K_NO_WAIT);
}