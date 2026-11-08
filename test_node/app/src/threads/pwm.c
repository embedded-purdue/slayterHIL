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

const struct device *const device_list[NUM_PWM] = {
    DEVICE_DT_GET(DT_ALIAS(pwm_input1)),
    DEVICE_DT_GET(DT_ALIAS(pwm_input2)),
    DEVICE_DT_GET(DT_ALIAS(pwm_input3)),
    DEVICE_DT_GET(DT_ALIAS(pwm_input4))};

K_THREAD_STACK_DEFINE(pwm_stack, PWM_STACK_SIZE);
struct k_thread pwm_thread_data;

// queue for captured pwm
K_MSGQ_DEFINE(avg_v_q,
              sizeof(single_avg_v_packet_t),
              AVG_V_QUEUE_LEN,
              4);

void pwm_thread(void *, void *, void *)
{
    avg_v_packet_t avg_v_packet = {0};
    single_avg_v_packet_t single_packet = {0};

    uint8_t update_mask = 0;
    uint8_t correct_mask = 0;
    for (int i = 0; i < NUM_PWM; i++)
    {
        correct_mask |= 1 << i;
    }

    while (1)
    {
        if (k_msgq_get(&avg_v_q, &single_packet, K_FOREVER) == 0)
        {
            LOG_INF("PWM %d: %.3f V", single_packet.id, single_packet.avg_v);

            if (single_packet.id < 0 || single_packet.id >= NUM_PWM)
            {
                LOG_ERR("Invalid PWM ID: %d", single_packet.id);
                continue;
            }

            avg_v_packet.v[single_packet.id] = single_packet.avg_v;
            update_mask |= 0b1 << single_packet.id;

            if ((update_mask & correct_mask) == correct_mask)
            {
                k_msgq_put(&spi_to_fs_q, &avg_v_packet, K_NO_WAIT);
                update_mask = 0;
            }
        }
    }
}

void pwm_capture_cb(const struct device *dev,
                    uint32_t channel,
                    uint32_t period_cycles,
                    uint32_t pulse_cycles,
                    int status,
                    void *user_data)
{
    if (status != 0)
    {
        LOG_ERR("PWM capture failed: %d", status);
        return;
    }

    if (period_cycles == 0)
    {
        return;
    }

    double duty_cycle = (double)pulse_cycles / period_cycles;
    double avg_voltage = duty_cycle * VOLTAGE_HIGH;

    single_avg_v_packet_t packet = {
        .id = (int)(uintptr_t)user_data,
        .avg_v = avg_voltage};

    if (k_msgq_put(&avg_v_q, &packet, K_NO_WAIT) != 0)
    {
        LOG_WRN("Average voltage queue full");
    }
}

void pwm_init()
{
    for (int i = 0; i < 4; i++)
    {
        if (!device_is_ready(device_list[i]))
        {
            LOG_ERR("PWM input device %d is not ready", i + 1);
            return;
        }
    }

    // if (!device_is_ready(pwm_output_dev))
    // {
    //     LOG_ERR("PWM output device is not ready");
    //     return;
    // }

    for (int i = 0; i < 4; i++)
    {
        int ret = pwm_configure_capture(device_list[i], 0,
                                        PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_CONTINUOUS,
                                        pwm_capture_cb, (void *)(uintptr_t)i);
        if (ret != 0)
        {
            LOG_ERR("Failed to configure PWM %d: %d", i + 1, ret);
            return;
        }
        ret = pwm_enable_capture(device_list[i], 0);
        if (ret != 0)
        {
            LOG_ERR("Failed to enable PWM %d: %d", i + 1, ret);
            return;
        }
    }

    k_thread_create(&pwm_thread_data, pwm_stack,
                    K_THREAD_STACK_SIZEOF(pwm_stack),
                    pwm_thread,
                    NULL, NULL, NULL,
                    PWM_PRIORITY, 0, K_NO_WAIT);
}