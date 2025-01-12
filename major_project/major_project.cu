#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <iostream>
#include <time.h>
#include <omp.h>

using namespace std;

// CUDA kernel for matrix addition
__global__ void matrixAddKernel(int n, int* A, int* B, int* C) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < n && col < n) {
        C[row * n + col] = A[row * n + col] + B[row * n + col];
    }
}

// CUDA kernel for matrix subtraction
__global__ void matrixSubKernel(int n, int* A, int* B, int* C) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < n && col < n) {
        C[row * n + col] = A[row * n + col] - B[row * n + col];
    }
}

// CUDA kernel for standard matrix multiplication
__global__ void matrixMulKernel(int n, int* A, int* B, int* C) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < n && col < n) {
        int sum = 0;
        for (int k = 0; k < n; k++) {
            sum += A[row * n + k] * B[k * n + col];
        }
        C[row * n + col] = sum;
    }
}

// Allocate and initialize matrix on host
void matrixRand(int n, int*& matrix) {
    matrix = (int*)malloc(n * n * sizeof(int));
    for (int i = 0; i < n * n; i++) {
        matrix[i] = rand() % 1000;
    }
}

// Print matrix
void matrixPrint(int n, int* matrix) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cout << matrix[i * n + j] << " ";
        }
        cout << endl;
    }
}

int** matrixAllocate(int n) {
    int **matrix = new int*[n];
    for (int i = 0; i < n; i++) {
        matrix[i] = new int[n];
    }
    return matrix;
}

void matrixFree(int **matrix, int n) {
    for (int i = 0; i < n; i++) {
        delete [] matrix[i];
    }
    delete [] matrix;
}

bool matrixCompare(int n, int** A, int** B) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (A[i][j] != B[i][j]) return false;
        }
    }
    return true;
}

void matrixStandardMul(int n, int** A, int** B, int** C) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Main function
int main(int argc, char* argv[]) {
    struct timespec start, stop, start_standard, stop_standard;
    double total_time, total_time_standard;

    if (argc != 3) {
        printf("Need two integers as input \n");
        printf("Use: <executable_name> <log_2(list_size)> <log_2(terminal_size)>\n");
        exit(0);
    }

    // k and k'
    int n = (1 << atoi(argv[1]));
    int threshold = (1 << atoi(argv[2]));

    // Allocate new matrices
    int** A = matrixAllocate(n);
    int** B = matrixAllocate(n);
    int** C = matrixAllocate(n);
    int** Cseq = matrixAllocate(n);

    // Randomly generate matrix values
    matrixRand(n, *A);
    matrixRand(n, *B);

    // Strassen Multiplication using OpenMP
    clock_gettime(CLOCK_REALTIME, &start);
    omp_set_num_threads(8);
    #pragma omp parallel
    {
        #pragma omp single
        {
            // Placeholder function for Strassen's Algorithm
            if (n <= threshold) {
                matrixStandardMul(n, A, B, C); // Use standard multiplication for small matrices
            } else {
                // Implement Strassen's Algorithm here if available
                matrixStandardMul(n, A, B, C); // Currently using standard multiplication as a placeholder
            }
        }
    }
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec - start.tv_sec) + 0.000000001 * (stop.tv_nsec - start.tv_nsec);

    // Standard Multiplication
    clock_gettime(CLOCK_REALTIME, &start_standard);
    matrixStandardMul(n, A, B, Cseq);
    clock_gettime(CLOCK_REALTIME, &stop_standard);
    total_time_standard = (stop_standard.tv_sec - start_standard.tv_sec) + 0.000000001 * (stop_standard.tv_nsec - start_standard.tv_nsec);

    // Check answer
    if (matrixCompare(n, C, Cseq)) {
        cout << "Correct!!!" << endl;
        printf("Matrix Size = %d * %d, Threshold = %d, time (sec) = %8.4f, standard_time = %8.4f\n", 
               n, n, threshold, total_time, total_time_standard);
    } else {
        cout << "We have a problem!" << endl;
    }

    // Free allocated memory
    matrixFree(A, n);
    matrixFree(B, n);
    matrixFree(C, n);
    matrixFree(Cseq, n);

    return 0;
}
