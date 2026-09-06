#ifndef PID_H
#define PID_H

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float integral_limit;   /* anti-windup clamp on the integral accumulator */
} pid_ctrl_t;

/**
 * @brief Initialize a PID controller.
 *
 * @param pid             PID instance to initialize.
 * @param kp              Proportional gain.
 * @param ki              Integral gain.
 * @param kd              Derivative gain (applied to error, not measurement).
 * @param integral_limit  Symmetric clamp applied to the integral accumulator.
 */
void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float integral_limit);

/**
 * @brief Compute one PID step.
 *
 * @param pid       PID instance.
 * @param setpoint  Desired value.
 * @param measured  Current measured value.
 * @param dt        Time since last call, in seconds. Must be > 0.
 * @return          Control output: Kp*e + Ki*∫e dt + Kd*(de/dt).
 */
float pid_update(pid_ctrl_t *pid, float setpoint, float measured, float dt);

/**
 * @brief Reset integral accumulator and previous error.
 *        Call on sensor loss or mode transitions to prevent wind-up carry-over.
 */
void pid_reset(pid_ctrl_t *pid);

#endif /* PID_H */
