#include "pwm.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_thread, LOG_LEVEL_INF);

const struct device *pwm_input_dev = DEVICE_DT_GET(DT_ALIAS(pwm_input));
const struct device *pwm_output_dev = NULL; // TODO: assign correct device

K_THREAD_STACK_DEFINE(pwm_stack, PWM_STACK_SIZE);
struct k_thread pwm_thread_data;

void pwm_thread(void*, void*, void*) {
    uint64_t pwm_period;
    uint64_t pwm_pulse;
    while(1) {
        if(pwm_capture_usec(pwm_input_dev, 1, PWM_CAPTURE_TYPE_BOTH, &pwm_period, &pwm_pulse, K_FOREVER) == 0) {
            // dummy printing for sanity
            LOG_INF("PWM captured: period = %llu us, pulse = %llu us", pwm_period, pwm_pulse);

            double duty_cycle = (double)pwm_pulse / (double)pwm_period;
            double avg_voltage = duty_cycle * VOLTAGE_HIGH;
            
            // eventually, we will send the output on the output device, but we don't
            // have the output device set up yet
        } else {
            LOG_ERR("Failed to capture PWM signal");
        }
    }
}

void pwm_init() {
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