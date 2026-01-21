#include <string.h>
#include <math.h>
// #include "matrix.h"
void mat_mult(float *A, int m, int n, float *B, int p, float *C) { 
    memset(C, 0, m * p * sizeof(float)); 
    for(int i = 0; i < m; i++) { 
        for(int j = 0; j < p; j++) { 
            for(int k = 0; k < n; k++) { 
                C[i*p + j] += A[i*n + k] * B[k*p + j]; 
            }
        }
    }
}

void mat_add(float *A, float *B, int m, int n, float *C) { 
    for(int i = 0; i < m * n; i++) { 
        C[i] = A[i] + B[i]; 
    }
}

void mat_sub(float *A, float *B, int m, int n, float *C) { 
    for(int i = 0; i < m * n; i++) { 
        C[i] = A[i] - B[i]; 
    }
}

void mat_transpose(float *A, int m, int n, float *AT) { 
    for(int i = 0; i < m; i++) { 
        for(int j = 0; j < n; j++) { 
            AT[j*m + i] = A[i*n + j]; 
        }
    }
}

void mat_inv_2x2(float A[2][2], float A_inv[2][2]) {
  float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
  if (fabs(det) < 1e-6f) {
    memset(A_inv, 0, sizeof(float) * 4);
    return;
  }
  A_inv[0][0] = A[1][1] / det;
  A_inv[0][1] = -A[0][1] / det;
  A_inv[1][0] = -A[1][0] / det;
  A_inv[1][1] = A[0][0] / det;
}