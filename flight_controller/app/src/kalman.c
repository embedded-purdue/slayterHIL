#define G 9.81f
#define DT 0.01f
#include <string.h>
#include <math.h>
#include "matrix.h"

typedef struct {
  float x[4];
  
  float P[4][4];
  
  float Q[4][4];
  
  float R[2][2];
} EKF_State;

void ekf_init(EKF_State *ekf) {
    ekf->x[0] = 0.0f; 
    ekf->x[1] = 0.0f; 
    ekf->x[2] = 0.0f; 
    ekf->x[3] = 0.0f; 

    float P_init[4][4] = { 
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.1f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.1f},
    };
    memcpy(ekf->P, P_init, sizeof(P_init)); 
    
    float Q_init[4][4] = { 
        {0.001f, 0.0f, 0.0f, 0.0f},
        {0.000f, 0.001f, 0.0f, 0.0f},
        {0.00f, 0.0f, 0.0001f, 0.0f},
        {0.00f, 0.0f, 0.0f, 0.0001f},
    }; 
    memcpy(ekf->Q, Q_init, sizeof(Q_init));

    float R_init[2][2] = { 
        {0.5f, 0.0f}, 
        {0.0f, 0.5f}
    };
    memcpy(ekf->R, R_init, sizeof(R_init)); 
}

void ekf_predict(EKF_State *ekf, float gx, float gy){ 
    float gx_corrected = gx-ekf->x[2];
    float gy_corrected = gy-ekf->x[3]; 

    ekf->x[0] += gx_corrected * DT; 
    ekf->x[1] += gy_corrected * DT; 

    float F[4][4] = { 
        {1.0f, 0.0f, -DT, 0.0f}, 
        {0.0f, 1.0f, 0.0f, -DT}, 
        {0.0f, 0.0f, 1.0f, 0.0f}, 
        {0.0f, 0.0f, 0.0f, 1.0f}

    }; 

    float P_temp[4][4];
    mat_mult((float*)F, 4, 4, (float*)ekf->P, 4, (float*)P_temp); 

    float FT[4][4] = { 
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {-DT, 0.0f, 1.0f, 0.0f},
        {0.0f, -DT, 0.0f, 1.0f}
    }; 
    mat_mult((float*)P_temp, 4, 4, (float*)FT, 4, (float*)ekf->P);
    float P_with_Q[4][4]; 
    mat_add((float*)ekf->P, (float*)ekf->Q, 4, 4, (float*)P_with_Q); 
    memcpy(ekf->P, P_with_Q, sizeof(P_with_Q)); 
}

void ekf_update(EKF_State *ekf, float ax, float ay, float az) {
  // Calculate measured angles from accelerometer
  float roll_measured = atan2f(-ay, az);
  float pitch_measured = atan2f(ax, sqrtf(ay * ay + az * az));

  // Measurement vector z
  float z[2] = {roll_measured, pitch_measured};

  // Measurement function h(x): we only measure angles
  float h[2] = {ekf->x[0], ekf->x[1]};

  // Innovation y = z - h(x)
  float y[2];
  y[0] = z[0] - h[0];
  y[1] = z[1] - h[1];

  // Measurement Jacobian H (2x4 matrix)
  float H[2][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f}
  };

  // P_HT = P @ H^T (4x4 @ 4x2 = 4x2)
  float HT[4][2];
  mat_transpose((float*)H, 2, 4, (float*)HT);

  float P_HT[4][2];
  mat_mult((float*)ekf->P, 4, 4, (float*)HT, 2, (float*)P_HT);

  // S = H @ P @ H^T + R (2x2)
  float H_P_HT[2][2];
  mat_mult((float*)H, 2, 4, (float*)P_HT, 2, (float*)H_P_HT);

  float S[2][2];
  mat_add((float*)H_P_HT, (float*)ekf->R, 2, 2, (float*)S);

  // S_inv = S^-1
  float S_inv[2][2];
  mat_inv_2x2(S, S_inv);

  // K = P_HT @ S_inv (4x2 @ 2x2 = 4x2)
  float K[4][2];
  mat_mult((float*)P_HT, 4, 2, (float*)S_inv, 2, (float*)K);

  // Update state: x = x + K @ y (4x2 @ 2x1 = 4x1)
  float K_y[4];
  memset(K_y, 0, sizeof(K_y));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      K_y[i] += K[i][j] * y[j];
    }
  }

  for (int i = 0; i < 4; i++) {
    ekf->x[i] += K_y[i];
  }

  // Update covariance: P = (I - K @ H) @ P
  // KH = K @ H (4x2 @ 2x4 = 4x4)
  float KH[4][4];
  mat_mult((float*)K, 4, 2, (float*)H, 4, (float*)KH);

  // I - KH
  float I_KH[4][4];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      I_KH[i][j] = (i == j ? 1.0f : 0.0f) - KH[i][j];
    }
  }

  // P_new = (I - KH) @ P
  float P_new[4][4];
  mat_mult((float*)I_KH, 4, 4, (float*)ekf->P, 4, (float*)P_new);
  memcpy(ekf->P, P_new, sizeof(ekf->P));
}