/*
 * PWM Master (ESP32-S3) - Sends a constant PWM voltage
 *
 * Outputs a fixed-duty-cycle PWM signal on GPIO6 via the LEDC peripheral.
 * Wire GPIO6 to the Nucleo's PB4 (TIM3_CH1) to test capture on the target.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/devicetree.h>

/* PWM output from device tree alias */
static const struct pwm_dt_spec pwm_out = PWM_DT_SPEC_GET(DT_ALIAS(pwm_output));

/* Desired constant output voltage (0.0 – 3.3 V) */
#define VOLTAGE_HIGH   3.3
#define TARGET_VOLTAGE 2.3   /* not 50 % duty cycle */

int main(void)
{
	if (!pwm_is_ready_dt(&pwm_out)) {
		printk("PWM device not ready\n");
		return -1;
	}

	/* duty_cycle = target_voltage / rail_voltage  (clamped 0-100 %) */
	int duty_pct = (int)((TARGET_VOLTAGE / VOLTAGE_HIGH) * 100.0);
	if (duty_pct < 0) {
		duty_pct = 0;
	} else if (duty_pct > 100) {
		duty_pct = 100;
	}

	uint32_t period_ns = pwm_out.period;          /* from device tree (20 ms) */
	uint32_t pulse_ns  = period_ns * duty_pct / 100;

	int rc = pwm_set_dt(&pwm_out, period_ns, pulse_ns);
	if (rc) {
		printk("pwm_set_dt failed: %d\n", rc);
		return rc;
	}

	printk("PWM master running: period=%u ns, pulse=%u ns (%d %% duty -> ~%.2f V)\n",
	       period_ns, pulse_ns, duty_pct, TARGET_VOLTAGE);

	/* Signal is now free-running; nothing else to do. */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
