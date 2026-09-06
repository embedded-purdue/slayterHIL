#include "pid.h"

void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float integral_limit)
{
    pid->kp             = kp;
    pid->ki             = ki;
    pid->kd             = kd;
    pid->integral       = 0.0f;
    pid->prev_error     = 0.0f;
    pid->integral_limit = integral_limit;
}

float pid_update(pid_ctrl_t *pid, float setpoint, float measured, float dt)
{
    float error = setpoint - measured;

    /* Integral with symmetric anti-windup clamp */
    pid->integral += error * dt;
    if (pid->integral >  pid->integral_limit) {
        pid->integral =  pid->integral_limit;
    } else if (pid->integral < -pid->integral_limit) {
        pid->integral = -pid->integral_limit;
    }

    /* Derivative on error — setpoint is constant (0° level), so no kick */
    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error = error;

    return (pid->kp * error)
         + (pid->ki * pid->integral)
         + (pid->kd * derivative);
}

void pid_reset(pid_ctrl_t *pid)
{
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
}
