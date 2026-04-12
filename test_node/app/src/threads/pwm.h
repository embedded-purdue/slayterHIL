#ifndef PWM_H
#define PWM_H
#include <zephyr/drivers/pwm.h>

#define PWM_STACK_SIZE (10 * 1024)
#define PWM_PRIORITY 5
#define VOLTAGE_HIGH 3.3
#define PWM_CAPTURE_CHANNEL 1
#define PWM_CAPTURE_FLAGS PWM_CAPTURE_TYPE_BOTH
#define PWM_CAPTURE_TIMEOUT K_USEC(1500) // 1.5 ms timeout for capture (longer than expected period to avoid spurious timeouts)

/*
* NOTE: the currently defined PWM period on DUT's end is 1ms (1 kHz frequency)
*  in the future, this means that FS can read at most 1kHz, but probably slightly slower because the period isn't perfect
*  and other calculations + setup require cycles.
*
*/

#define NUM_MOTORS 4

void pwm_init();
void pwm_get_latest_voltages(float *out, int count);

#endif
