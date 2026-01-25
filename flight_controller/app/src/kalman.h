#ifndef KALMAN_H
#define KALMAN_H

typedef struct { 
    float x[4];  
    float P[4][4]; 
    float Q[4][4]; 
    float R[2][2]; 
} EKF_State; 

void ekf_init(EKF_State *ekf); 
void ekf_predict(EKF_State *ekf, float gx, float gy); 
void ekf_update(EKF_State *ekf, float ax, float ay, float az); 

#endif