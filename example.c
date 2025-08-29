#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Función para llenar matriz con enteros aleatorios
void llenarMatrix(int32_t **matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (rand() % 101) - 50;  // Rango [-50, 50]
        }
    }
}

// Multiplicación de matrices
void multiplicarMatrix(int32_t **A, int32_t **B, int32_t **C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        // No se pasa el parámetro -> salir
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        // Tamaño inválido
        return 1;
    }

    srand((unsigned int)time(NULL));

    // Reservar memoria dinámica para las matrices
    int32_t **A = malloc(n * sizeof(int32_t *));
    int32_t **B = malloc(n * sizeof(int32_t *));
    int32_t **C = malloc(n * sizeof(int32_t *));
    for (int i = 0; i < n; i++) {
        A[i] = malloc(n * sizeof(int32_t));
        B[i] = malloc(n * sizeof(int32_t));
        C[i] = malloc(n * sizeof(int32_t));
    }

    // Llenar y multiplicar
    llenarMatrix(A, n);
    llenarMatrix(B, n);
  // Medir el tiempo de multiplicación de matrices
    clock_t start = clock();

    // Realizar la multiplicación
    multiplicarMatrix(A, B, C, n);

    clock_t end = clock();

    // Calcular el tiempo de CPU utilizado
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo de CPU usado para la multiplicación: %f segundos\n", cpu_time_used);

    // Mostrar resultados
    // print_matrix("Matriz A", A, n);
    // print_matrix("Matriz B", B, n);
    // print_matrix("Matriz C = A * B", C, n);

    // Liberar memoria
    for (int i = 0; i < n; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }
    free(A);
    free(B);
    free(C);

    return 0;
}