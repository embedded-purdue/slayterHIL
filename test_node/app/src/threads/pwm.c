#include "pwm.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_thread, LOG_LEVEL_INF);

const struct device *pwm_input_dev = DEVICE_DT_GET(DT_ALIAS(pwm_input));
const struct device *pwm_output_dev = NULL; // TODO: assign correct device

//shared voltage updated by pwm_thread, read by spi_fs thread
static float latest_voltage;
static struct k_mutex voltage_mutex;

float pwm_get_latest_voltage(void) {
    float v;
    k_mutex_lock(&voltage_mutex, K_FOREVER);
    v = latest_voltage;
    k_mutex_unlock(&voltage_mutex);
    return v;
}

K_THREAD_STACK_DEFINE(pwm_stack, PWM_STACK_SIZE);
struct k_thread pwm_thread_data;

void pwm_thread(void*, void*, void*) {
    uint64_t pwm_period;
    uint64_t pwm_pulse;
    while(1) {
        if(pwm_capture_usec(pwm_input_dev, PWM_CAPTURE_CHANNEL, PWM_CAPTURE_FLAGS, &pwm_period, &pwm_pulse, PWM_CAPTURE_TIMEOUT) == 0) {
            // dummy printing for sanity
            LOG_INF("PWM captured: period = %llu us, pulse = %llu us", pwm_period, pwm_pulse);

            double duty_cycle = (double)pwm_pulse / (double)pwm_period;
            double avg_voltage = duty_cycle * VOLTAGE_HIGH;

            //update shared voltage for SPI thread to send to FS
            k_mutex_lock(&voltage_mutex, K_FOREVER);
            latest_voltage = (float)avg_voltage;
            k_mutex_unlock(&voltage_mutex);
        } else {
            LOG_ERR("Failed to capture PWM signal");
        }
    }
}

void pwm_init() {
    k_mutex_init(&voltage_mutex);

    if (!device_is_ready(pwm_input_dev)) {
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