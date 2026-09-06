#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Shared control setpoints. RC/UART commands adjust the setpoints; the LiDAR
 * thread publishes the latest altitude measurement; the attitude control loop
 * snapshots all of it each tick. All access is mutex-protected because these
 * fields are written and read from three different threads.
 */

/* RC command step sizes and limits */
#define CONTROL_ATTITUDE_STEP_DEG   2.0f    /* per L/R/U/D command   */
#define CONTROL_ATTITUDE_LIMIT_DEG  15.0f   /* max commanded tilt    */
#define CONTROL_ALTITUDE_STEP_MM    100.0f  /* per A/X command       */
#define CONTROL_ALTITUDE_MIN_MM     200.0f  /* floor of altitude hold */
#define CONTROL_ALTITUDE_MAX_MM     3000.0f /* ceiling of altitude hold */

void control_init(void);

/* RC setpoint adjustments (thread-safe, clamped) */
void control_adjust_pitch(float delta_deg);
void control_adjust_roll(float delta_deg);
void control_adjust_altitude(float delta_mm);

/* Publish latest measured altitude. On the first sample, engages altitude
 * hold by seeding the setpoint to the current height. */
void control_set_altitude(uint32_t distance_mm);

/* Snapshot the current setpoints + latest altitude for one control tick.
 * Any pointer may be NULL. */
void control_get(float *pitch_sp, float *roll_sp, float *altitude_sp,
                 uint32_t *altitude_mm, bool *altitude_valid);

#endif /* CONTROL_H */
