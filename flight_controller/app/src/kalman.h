#ifndef KALMAN_H
#define KALMAN_H

typedef struct { 
    float x; 
    float P; 
    float Q; 
    float R; 
} EKF_State; 

void ekf_init(EKF_State *ekf); 
void ekf_predict(EKF_State *ekf, float gx, float gy); 
void ekf_update(EKF_State *ekf, float ax, float ay, float az); 

#endif