#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <omp.h>

typedef int** Matrix;

void printMatrix(int size, Matrix matrix){
    for(int row = 0; row < size; row++){
        for(int col = 0; col < size; col++){
            printf("%d ", matrix[row][col]);
        }
        printf("\n");
    }
    printf("\n");
}

void addMatrices(int size, Matrix matrixA, Matrix matrixB, Matrix matrixC){
    for (int row = 0; row < size; row++){
        for (int col = 0; col < size; col++){
            matrixC[row][col] = matrixA[row][col] + matrixB[row][col];
        }
    }
}

void subtractMatrices(int size, Matrix matrixA, Matrix matrixB, Matrix matrixC){
    for (int row = 0; row < size; row++){
        for (int col = 0; col < size; col++){
            matrixC[row][col] = matrixA[row][col] - matrixB[row][col];
        }
    }
}

void randomizeMatrix(int size, Matrix matrix){
    for (int row = 0; row < size; row++){
        for(int col = 0; col < size; col++){
            matrix[row][col] = rand() % 1000;
        }
    }
}

Matrix allocateMatrix(int size){
    Matrix matrix = (Matrix)malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++){
        matrix[i] = (int*)malloc(size * sizeof(int));
    }
    return matrix;
}

void freeMatrix(Matrix matrix, int size){
    for (int i = 0; i < size; i++){
        free(matrix[i]);
    }
    free(matrix);
}

void standardMultiply(int size, Matrix matrixA, Matrix matrixB, Matrix matrixC){
    for (int row = 0; row < size; row++){
        for (int col = 0; col < size; col++){
            matrixC[row][col] = 0;
            for (int k = 0; k < size; k++){
                matrixC[row][col] += matrixA[row][k] * matrixB[k][col];
            }
        }
    }
}

void copySubmatrix(int halfSize, int fullSize, int rowOffset, int colOffset, Matrix source, Matrix destination){
    for (int row = 0; row < halfSize; row++){
        for (int col = 0; col < halfSize; col++){
            if (row + rowOffset < fullSize && col + colOffset < fullSize){
                destination[row][col] = source[row + rowOffset][col + colOffset];
            }
        }
    }
}

void strassenMultiply(int threshold, int size, Matrix matrixA, Matrix matrixB, Matrix matrixC){
    if (size <= threshold){
        standardMultiply(size, matrixA, matrixB, matrixC);
        return;
    }
    int halfSize = (size + 1) / 2;

    Matrix temp1 = allocateMatrix(halfSize);
    Matrix temp2 = allocateMatrix(halfSize);
    Matrix temp3 = allocateMatrix(halfSize);
    Matrix temp4 = allocateMatrix(halfSize);
    Matrix temp5 = allocateMatrix(halfSize);
    Matrix temp6 = allocateMatrix(halfSize);
    Matrix temp7 = allocateMatrix(halfSize);
    Matrix temp8 = allocateMatrix(halfSize);
    Matrix temp9 = allocateMatrix(halfSize);
    Matrix temp10 = allocateMatrix(halfSize);

    Matrix m1 = allocateMatrix(halfSize);
    Matrix m2 = allocateMatrix(halfSize);
    Matrix m3 = allocateMatrix(halfSize);
    Matrix m4 = allocateMatrix(halfSize);
    Matrix m5 = allocateMatrix(halfSize);
    Matrix m6 = allocateMatrix(halfSize);
    Matrix m7 = allocateMatrix(halfSize);

    Matrix a11 = allocateMatrix(halfSize);
    Matrix a12 = allocateMatrix(halfSize);
    Matrix a21 = allocateMatrix(halfSize);
    Matrix a22 = allocateMatrix(halfSize);

    Matrix b11 = allocateMatrix(halfSize);
    Matrix b12 = allocateMatrix(halfSize);
    Matrix b21 = allocateMatrix(halfSize);
    Matrix b22 = allocateMatrix(halfSize);

    copySubmatrix(halfSize, size, 0, 0, matrixA, a11);
    copySubmatrix(halfSize, size, 0, halfSize, matrixA, a12);
    copySubmatrix(halfSize, size, halfSize, 0, matrixA, a21);
    copySubmatrix(halfSize, size, halfSize, halfSize, matrixA, a22);

    copySubmatrix(halfSize, size, 0, 0, matrixB, b11);
    copySubmatrix(halfSize, size, 0, halfSize, matrixB, b12);
    copySubmatrix(halfSize, size, halfSize, 0, matrixB, b21);
    copySubmatrix(halfSize, size, halfSize, halfSize, matrixB, b22);

    #pragma omp task
    {
        addMatrices(halfSize, a11, a22, temp1);
        addMatrices(halfSize, b11, b22, temp2);
        strassenMultiply(threshold, halfSize, temp1, temp2, m1);
    }

    #pragma omp task
    {
        addMatrices(halfSize, a21, a22, temp3);
        strassenMultiply(threshold, halfSize, temp3, b11, m2);
    }

    #pragma omp task
    {
        subtractMatrices(halfSize, b12, b22, temp4);
        strassenMultiply(threshold, halfSize, a11, temp4, m3);
    }

    #pragma omp task
    {
        subtractMatrices(halfSize, b21, b11, temp5);
        strassenMultiply(threshold, halfSize, a22, temp5, m4);
    }

    #pragma omp task
    {
        addMatrices(halfSize, a11, a12, temp6);
        strassenMultiply(threshold, halfSize, temp6, b22, m5);
    }

    #pragma omp task
    {
        subtractMatrices(halfSize, a21, a11, temp7);
        addMatrices(halfSize, b11, b12, temp8);
        strassenMultiply(threshold, halfSize, temp7, temp8, m6);
    }

    #pragma omp task
    {
        subtractMatrices(halfSize, a12, a22, temp9);
        addMatrices(halfSize, b21, b22, temp10);
        strassenMultiply(threshold, halfSize, temp9, temp10, m7);
    }

    #pragma omp taskwait

    for (int row = 0; row < halfSize; row++){
        for(int col = 0; col < halfSize; col++){
            matrixC[row][col] = m1[row][col] + m4[row][col] - m5[row][col] + m7[row][col];
            if (col + halfSize < size){
                matrixC[row][col + halfSize] = m3[row][col] + m5[row][col];
            }
            if (row + halfSize < size){
                matrixC[row + halfSize][col] = m2[row][col] + m4[row][col];
            }
            if (row + halfSize < size && col + halfSize < size){
                matrixC[row + halfSize][col + halfSize] = m1[row][col] - m2[row][col] + m3[row][col] + m6[row][col];
            }
        }
    }

    freeMatrix(temp1, halfSize);
    freeMatrix(temp2, halfSize);
    freeMatrix(temp3, halfSize);
    freeMatrix(temp4, halfSize);
    freeMatrix(temp5, halfSize);
    freeMatrix(temp6, halfSize);
    freeMatrix(temp7, halfSize);
    freeMatrix(temp8, halfSize);
    freeMatrix(temp9, halfSize);
    freeMatrix(temp10, halfSize);

    freeMatrix(m1, halfSize);
    freeMatrix(m2, halfSize);
    freeMatrix(m3, halfSize);
    freeMatrix(m4, halfSize);
    freeMatrix(m5, halfSize);
    freeMatrix(m6, halfSize);
    freeMatrix(m7, halfSize);

    freeMatrix(a11, halfSize);
    freeMatrix(a12, halfSize);
    freeMatrix(a21, halfSize);
    freeMatrix(a22, halfSize);

    freeMatrix(b11, halfSize);
    freeMatrix(b12, halfSize);
    freeMatrix(b21, halfSize);
    freeMatrix(b22, halfSize);
}

int compareMatrices(int size, Matrix matrixA, Matrix matrixB){
    for (int row = 0; row < size; row++){
        for (int col = 0; col < size; col++){
            if(matrixA[row][col] != matrixB[row][col]) return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]){
    struct timespec start, stop, startStandard, stopStandard;
    double strassenTime, standardTime;

    if (argc != 3) {
        printf("Need two integers as input \n");
        exit(0);
    }

    int size = (1 << atoi(argv[1]));
    int threshold = (1 << atoi(argv[2]));

    Matrix matrixA = allocateMatrix(size);
    Matrix matrixB = allocateMatrix(size);
    Matrix matrixC = allocateMatrix(size);
    Matrix matrixCseq = allocateMatrix(size);

    randomizeMatrix(size, matrixA);
    randomizeMatrix(size, matrixB);

    clock_gettime(CLOCK_REALTIME, &start);
    omp_set_num_threads(8);
    #pragma omp parallel
    {
        #pragma omp single
        {
            strassenMultiply(threshold, size, matrixA, matrixB, matrixC);
        }
    }
    clock_gettime(CLOCK_REALTIME, &stop);
    strassenTime = (stop.tv_sec - start.tv_sec) + 0.000000001 * (stop.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_REALTIME, &startStandard);
    standardMultiply(size, matrixA, matrixB, matrixCseq);
    clock_gettime(CLOCK_REALTIME, &stopStandard);
    standardTime = (stopStandard.tv_sec - startStandard.tv_sec) + 0.000000001 * (stopStandard.tv_nsec - startStandard.tv_nsec);

    if (compareMatrices(size, matrixC, matrixCseq)){
        printf("Matrix Size = %d * %d, Threshold = %d, error = %d, time (Strassen) = %8.4f, standard_time = %8.4f\n", 
            size, size, threshold, 0, strassenTime, standardTime);
    }
    else{
        printf("Houston, We have a problem!\n");
    }

    freeMatrix(matrixA, size);
    freeMatrix(matrixB, size);
    freeMatrix(matrixC, size);
    freeMatrix(matrixCseq, size);

    return 0;
}