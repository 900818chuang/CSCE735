#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

void matrixPrint(int n, int** matrix) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void matrixAdd(int n, int** A, int** B, int** C) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = A[i][j] + B[i][j];
        }
    }
}

void matrixSub(int n, int** A, int** B, int** C) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = A[i][j] - B[i][j];
        }
    }
}

void matrixRand(int n, int** matrix) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = rand() % 1000;
        }
    }
}

int** matrixAllocate(int n) {
    int** matrix = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        matrix[i] = (int*)malloc(n * sizeof(int));
    }
    return matrix;
}

void matrixFree(int** matrix, int n) {
    for (int i = 0; i < n; ++i) {
        free(matrix[i]);
    }
    free(matrix);
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

void matrixCopy(int h, int n, int iOffset, int jOffset, int** A, int** B) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < h; j++) {
            if (i + iOffset < n && j + jOffset < n) {
                B[i][j] = A[i + iOffset][j + jOffset];
            } else {
                B[i][j] = 0; // Fill with 0 if out of bounds
            }
        }
    }
}

void matrixStrassen(int threshold, int n, int** A, int** B, int** C) {
    if (n <= threshold) {
        matrixStandardMul(n, A, B, C);
        return;
    }

    int h = (n + 1) / 2;

    int** addTemp_1 = matrixAllocate(h);
    int** addTemp_2 = matrixAllocate(h);
    int** addTemp_3 = matrixAllocate(h);
    int** addTemp_4 = matrixAllocate(h);
    int** addTemp_5 = matrixAllocate(h);
    int** addTemp_6 = matrixAllocate(h);
    int** addTemp_7 = matrixAllocate(h);
    int** addTemp_8 = matrixAllocate(h);
    int** addTemp_9 = matrixAllocate(h);
    int** addTemp_10 = matrixAllocate(h);

    int** M1 = matrixAllocate(h);
    int** M2 = matrixAllocate(h);
    int** M3 = matrixAllocate(h);
    int** M4 = matrixAllocate(h);
    int** M5 = matrixAllocate(h);
    int** M6 = matrixAllocate(h);
    int** M7 = matrixAllocate(h);

    int** A11 = matrixAllocate(h);
    int** A12 = matrixAllocate(h);
    int** A21 = matrixAllocate(h);
    int** A22 = matrixAllocate(h);

    int** B11 = matrixAllocate(h);
    int** B12 = matrixAllocate(h);
    int** B21 = matrixAllocate(h);
    int** B22 = matrixAllocate(h);

    matrixCopy(h, n, 0, 0, A, A11);
    matrixCopy(h, n, 0, h, A, A12);
    matrixCopy(h, n, h, 0, A, A21);
    matrixCopy(h, n, h, h, A, A22);

    matrixCopy(h, n, 0, 0, B, B11);
    matrixCopy(h, n, 0, h, B, B12);
    matrixCopy(h, n, h, 0, B, B21);
    matrixCopy(h, n, h, h, B, B22);

    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            {
                matrixAdd(h, A11, A22, addTemp_1);
                matrixAdd(h, B11, B22, addTemp_2);
                matrixStrassen(threshold, h, addTemp_1, addTemp_2, M1);
            }

            #pragma omp task
            {
                matrixAdd(h, A21, A22, addTemp_3);
                matrixStrassen(threshold, h, addTemp_3, B11, M2);
            }

            #pragma omp task
            {
                matrixSub(h, B12, B22, addTemp_4);
                matrixStrassen(threshold, h, A11, addTemp_4, M3);
            }

            #pragma omp task
            {
                matrixSub(h, B21, B11, addTemp_5);
                matrixStrassen(threshold, h, A22, addTemp_5, M4);
            }

            #pragma omp task
            {
                matrixAdd(h, A11, A12, addTemp_6);
                matrixStrassen(threshold, h, addTemp_6, B22, M5);
            }

            #pragma omp task
            {
                matrixSub(h, A21, A11, addTemp_7);
                matrixAdd(h, B11, B12, addTemp_8);
                matrixStrassen(threshold, h, addTemp_7, addTemp_8, M6);
            }

            #pragma omp task
            {
                matrixSub(h, A12, A22, addTemp_9);
                matrixAdd(h, B21, B22, addTemp_10);
                matrixStrassen(threshold, h, addTemp_9, addTemp_10, M7);
            }

            #pragma omp taskwait
        }
    }

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < h; j++) {
            C[i][j] = M1[i][j] + M4[i][j] - M5[i][j] + M7[i][j];
            if (j + h < n) {
                C[i][j + h] = M3[i][j] + M5[i][j];
            }
            if (i + h < n) {
                C[i + h][j] = M2[i][j] + M4[i][j];
            }
            if (i + h < n && j + h < n) {
                C[i + h][j + h] = M1[i][j] - M2[i][j] + M3[i][j] + M6[i][j];
            }
        }
    }

    matrixFree(addTemp_1, h);
    matrixFree(addTemp_2, h);
    matrixFree(addTemp_3, h);
    matrixFree(addTemp_4, h);
    matrixFree(addTemp_5, h);
    matrixFree(addTemp_6, h);
    matrixFree(addTemp_7, h);
    matrixFree(addTemp_8, h);
    matrixFree(addTemp_9, h);
    matrixFree(addTemp_10, h);

    matrixFree(M1, h);
    matrixFree(M2, h);
    matrixFree(M3, h);
    matrixFree(M4, h);
    matrixFree(M5, h);
    matrixFree(M6, h);
    matrixFree(M7, h);

    matrixFree(A11, h);
    matrixFree(A12, h);
    matrixFree(A21, h);
    matrixFree(A22, h);

    matrixFree(B11, h);
    matrixFree(B12, h);
    matrixFree(B21, h);
    matrixFree(B22, h);
}

int matrixCompare(int n, int** A, int** B) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (A[i][j] != B[i][j]) return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    struct timespec start, stop, start_standard, stop_standard;
    double total_time, total_time_standard;

    if (argc != 3) {
        printf("Need two integers as input \n");
        printf("Use: <executable_name> <log_2(list_size)> <log_2(num_threads)>\n");
        exit(0);
    }

    int n = (1 << atoi(argv[1]));
    int threshold = (1 << atoi(argv[2]));

    int** A = matrixAllocate(n);
    int** B = matrixAllocate(n);
    int** C = matrixAllocate(n);
    int** Cseq = matrixAllocate(n);

    matrixRand(n, A);
    matrixRand(n, B);

    clock_gettime(CLOCK_REALTIME, &start);
    omp_set_num_threads(8);
    #pragma omp parallel
    {
        #pragma omp single
        {
            matrixStrassen(threshold, n, A, B, C);
        }
    }
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec - start.tv_sec) + 0.000000001 * (stop.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_REALTIME, &start_standard);
    matrixStandardMul(n, A, B, Cseq);
    clock_gettime(CLOCK_REALTIME, &stop_standard);
    total_time_standard = (stop_standard.tv_sec - start_standard.tv_sec) + 0.000000001 * (stop_standard.tv_nsec - start_standard.tv_nsec);

    if (matrixCompare(n, C, Cseq)) {
        printf("Correct!!!\n");
        printf("Matrix Size = %d * %d, Threshold = %d, time (sec) = %8.4f, standard_time = %8.4f\n",
            n, n, threshold, total_time, total_time_standard);
    } else {
        printf("We have a problem!\n");
    }

    matrixFree(A, n);
    matrixFree(B, n);
    matrixFree(C, n);
    matrixFree(Cseq, n);

    return 0;
}
