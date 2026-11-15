#define PWM_SEND 0

#if PWM_SEND == 0

#include "threads/pwm.h"
#include "threads/spi_to_fs.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stdint.h>

int main(void)
{
    printk("Starting PWM capture test\n");

    pwm_init();

    printk("PWM capture initialization requested\n");

    avg_v_packet_t packet = {0};
    uint32_t packet_count = 0;

    while (1)
    {
        int ret = k_msgq_get(&spi_to_fs_q, &packet, K_FOREVER);

        if (ret != 0)
        {
            printk("Failed to receive PWM packet: %d\n", ret);
            continue;
        }

        packet_count++;

        printk("\n========== PWM Packet %u ==========\n", packet_count);

        for (int i = 0; i < NUM_PWM; i++)
        {
            int32_t millivolts = (int32_t)(packet.v[i] * 1000.0 + 0.5);

            printk("PWM %d: %d.%03d V\n", i + 1, (int)(millivolts / 1000),(int)(millivolts % 1000));
        }

        printk("===================================\n");
    }

    return 0;
}
#else

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stdint.h>

#define NUM_PWM 4
#define PWM_PERIOD PWM_MSEC(1)
#define PWM_UPDATE_MS 100

static const struct device *const pwm_outputs[NUM_PWM] = {
    DEVICE_DT_GET(DT_ALIAS(pwm_input1)),
    DEVICE_DT_GET(DT_ALIAS(pwm_input2)),
    DEVICE_DT_GET(DT_ALIAS(pwm_input3)),
    DEVICE_DT_GET(DT_ALIAS(pwm_input4))
};

static uint32_t duty_cycles[NUM_PWM] = {
    25, 50, 75, 90
};

int main(void)
{
    printk("Starting PWM output test\n");

    for (int i = 0; i < NUM_PWM; i++) {
        if (!device_is_ready(pwm_outputs[i])) {
            printk("PWM %d device not ready\n", i + 1);
            return -ENODEV;
        }
    }

    while (1) {
        for (int i = 0; i < NUM_PWM; i++) {
            uint32_t pulse = (PWM_PERIOD / 100) * duty_cycles[i];

            int ret = pwm_set(pwm_outputs[i], 1, PWM_PERIOD, pulse, PWM_POLARITY_NORMAL);

            if (ret != 0) {
                printk("Failed to configure PWM %d: %d\n", i + 1, ret);
                return ret;
            }

            printk("PWM %d: channel %u, duty %u%%, period 1000 us\n", i + 1, 1, duty_cycles[i]);

            duty_cycles[i] += 1;

            if (duty_cycles[i] >= 100) {
                duty_cycles[i] = 1;
            }
        }

        printk("----------------------------\n");

        k_msleep(PWM_UPDATE_MS);
    }

    return 0;
}

#endif