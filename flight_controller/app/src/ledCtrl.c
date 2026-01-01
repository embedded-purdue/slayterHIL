#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/devicetree.h>
#include "ledCtrl.h"

#define LED_COUNT 4



static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(led0));
static const struct pwm_dt_spec leds[LED_COUNT] = {
    PWM_DT_SPEC_GET(DT_ALIAS(led0)),
    PWM_DT_SPEC_GET(DT_ALIAS(led1)),
    PWM_DT_SPEC_GET(DT_ALIAS(led2)),
    PWM_DT_SPEC_GET(DT_ALIAS(led3)),
};


static const uint32_t fallback_period_ns = 20U * 1000U * 1000U;

void initialize_leds(void)
{
    for (int i = 0; i < LED_COUNT; ++i) {
        const struct pwm_dt_spec *spec = &leds[i];

        if (!pwm_is_ready_dt(spec)) {
            printk("ledCtrl: PWM %d not ready (dev/ph=%p, channel=%u)\n",
                   i, spec->dev, spec->channel);
            continue;
        }

        uint32_t period = spec->period ? spec->period : fallback_period_ns;

        int rc = pwm_set_dt(spec, period, 0);
        if (rc) {
            printk("ledCtrl: pwm_set_dt init fail led=%d rc=%d\n", i, rc);
        } else {
            printk("ledCtrl: led %d initialized (period=%u)\n", i, (unsigned)period);
        }
    }
}


void set_led_intensity(int led_id, int intensity)
{
    if (led_id < 0 || led_id >= LED_COUNT) {
        printk("ledCtrl: invalid led id %d\n", led_id);
        return;
    }

    if (intensity < 0) {
        intensity = 0;
    } else if (intensity > 100) {
        intensity = 100;
    }

    const struct pwm_dt_spec *spec = &leds[led_id];
    if (!pwm_is_ready_dt(spec)) {
        printk("ledCtrl: pwm not ready for led %d\n", led_id);
        return;
    }

    uint32_t period = spec->period ? spec->period : fallback_period_ns;
    uint64_t pulse = (uint64_t)period * (uint64_t)intensity / 100ULL;

    int rc = pwm_set_dt(spec, period, (uint32_t)pulse);
    if (rc) {
        printk("ledCtrl: pwm_set_dt failed led=%d intensity=%d rc=%d\n",
               led_id, intensity, rc);
    }
}
