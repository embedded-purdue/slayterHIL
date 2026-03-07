#ifndef PWM_H
#define PWM_H

#define PWM_STACK_SIZE (10 * 1024)
#define PWM_PRIORITY 5
#define VOLTAGE_HIGH 3.3

void pwm_init();

#endif