#ifndef PWM_H
#define PWM_H
#include <zephyr/drivers/pwm.h>

#include "spi_to_fs.h"

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

#define NUM_PWM 4

#define AVG_V_QUEUE_LEN (4U * NUM_PWM)
extern struct k_msgq avg_v_q;

typedef struct
{
    int id;
    double avg_v;
} single_avg_v_packet_t;

void pwm_init();
void pwm_capture_cb(const struct device *dev,
                    uint32_t channel,
                    uint32_t period_cycles,
                    uint32_t pulse_cycles,
                    int status,
                    void *user_data);

#endif