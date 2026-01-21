#ifndef MATRIX_H
#define MATRIX_H

void mat_mult(float *A, int m, int n, float *B, int p, float *C); 
void mat_add(float *A, float *B, int m, int n, float *C); 
void mat_sub(float *A, float *B, int m, int n, float *C); 
void mat_transpose(float *A, int m, int n, float *AT); 
void mat_inv_2x2(float A[2][2], float A_inv[2][2]);


#endif 