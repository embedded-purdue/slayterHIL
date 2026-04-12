/*
 * PWM Target (Nucleo H563ZI) - Reads incoming PWM and computes average voltage
 *
 * Captures the PWM signal on PB4 (TIM3_CH1) using Zephyr's PWM capture API,
 * then calculates duty cycle and the corresponding average voltage.
 *
 * Wire the ESP32-S3 GPIO6 (master PWM output) to Nucleo PB4.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/devicetree.h>

#define VOLTAGE_HIGH 3.3
#define CAPTURE_INTERVAL_MS 500

/* PWM capture device from device tree alias (timers3_pwm) */
static const struct device *pwm_input_dev = DEVICE_DT_GET(DT_ALIAS(pwm_input));

int main(void)
{
	if (!device_is_ready(pwm_input_dev)) {
		printk("PWM input device not ready\n");
		return -1;
	}

	printk("PWM target: listening for PWM on TIM3_CH1 (PB4)\n");

	uint64_t period_usec;
	uint64_t pulse_usec;

	while (1) {
		int rc = pwm_capture_usec(pwm_input_dev, 1, PWM_CAPTURE_TYPE_BOTH, &period_usec, &pulse_usec, K_FOREVER);
		if (rc == 0 && period_usec > 0) {
			double duty_cycle = (double)pulse_usec / (double)period_usec;
			double avg_voltage = duty_cycle * VOLTAGE_HIGH;

			printk("Captured: period=%llu us, pulse=%llu us, "
			       "duty=%.1f %%, voltage=%.2f V\n",
			       period_usec, pulse_usec,
			       duty_cycle * 100.0, avg_voltage);
		} else {
			printk("PWM capture failed (rc=%d)\n", rc);
		}

		k_msleep(CAPTURE_INTERVAL_MS);
	}

	return 0;
}
