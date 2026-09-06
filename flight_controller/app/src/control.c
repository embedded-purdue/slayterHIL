#include "control.h"
#include <zephyr/kernel.h>

static K_MUTEX_DEFINE(control_mutex);

static float    s_pitch_sp;
static float    s_roll_sp;
static float    s_altitude_sp;
static uint32_t s_altitude_mm;
static bool     s_altitude_valid;

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void control_init(void)
{
    k_mutex_lock(&control_mutex, K_FOREVER);
    s_pitch_sp       = 0.0f;
    s_roll_sp        = 0.0f;
    s_altitude_sp    = 0.0f;
    s_altitude_mm    = 0;
    s_altitude_valid = false;
    k_mutex_unlock(&control_mutex);
}

void control_adjust_pitch(float delta_deg)
{
    k_mutex_lock(&control_mutex, K_FOREVER);
    s_pitch_sp = clampf(s_pitch_sp + delta_deg,
                        -CONTROL_ATTITUDE_LIMIT_DEG, CONTROL_ATTITUDE_LIMIT_DEG);
    k_mutex_unlock(&control_mutex);
}

void control_adjust_roll(float delta_deg)
{
    k_mutex_lock(&control_mutex, K_FOREVER);
    s_roll_sp = clampf(s_roll_sp + delta_deg,
                       -CONTROL_ATTITUDE_LIMIT_DEG, CONTROL_ATTITUDE_LIMIT_DEG);
    k_mutex_unlock(&control_mutex);
}

void control_adjust_altitude(float delta_mm)
{
    k_mutex_lock(&control_mutex, K_FOREVER);
    s_altitude_sp = clampf(s_altitude_sp + delta_mm,
                           CONTROL_ALTITUDE_MIN_MM, CONTROL_ALTITUDE_MAX_MM);
    k_mutex_unlock(&control_mutex);
}

void control_set_altitude(uint32_t distance_mm)
{
    k_mutex_lock(&control_mutex, K_FOREVER);
    s_altitude_mm = distance_mm;
    if (!s_altitude_valid) {
        /* Engage altitude hold at the current height on the first reading. */
        s_altitude_sp = clampf((float)distance_mm,
                               CONTROL_ALTITUDE_MIN_MM, CONTROL_ALTITUDE_MAX_MM);
        s_altitude_valid = true;
    }
    k_mutex_unlock(&control_mutex);
}

void control_get(float *pitch_sp, float *roll_sp, float *altitude_sp,
                 uint32_t *altitude_mm, bool *altitude_valid)
{
    k_mutex_lock(&control_mutex, K_FOREVER);
    if (pitch_sp)       *pitch_sp       = s_pitch_sp;
    if (roll_sp)        *roll_sp        = s_roll_sp;
    if (altitude_sp)    *altitude_sp    = s_altitude_sp;
    if (altitude_mm)    *altitude_mm    = s_altitude_mm;
    if (altitude_valid) *altitude_valid = s_altitude_valid;
    k_mutex_unlock(&control_mutex);
}
