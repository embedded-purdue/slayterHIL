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
            
            // eventually, we will send the output on the output device, but we don't
            // have the output device set up yet
        } else {
            LOG_ERR("Failed to capture PWM signal");
        }
    }
}

void pwm_init() {
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