#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

// Estructura para pasar datos a los hilos
typedef struct {
    int32_t **A;
    int32_t **B;
    int32_t **C;
    int n;
    int fila_inicio;
    int fila_fin;
} thread_data;

// Función para llenar matriz con enteros aleatorios
void llenarMatrix(int32_t **matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (rand() % 101) - 50;  // Rango [-50, 50]
        }
    }
}

// Función que ejecutan los hilos
void *multiplicarParte(void *arg) {
    thread_data *data = (thread_data *)arg;
    int n = data->n;

    for (int i = data->fila_inicio; i < data->fila_fin; i++) {
        for (int j = 0; j < n; j++) {
            data->C[i][j] = 0;
            for (int k = 0; k < n; k++) {
                data->C[i][j] += data->A[i][k] * data->B[k][j];
            }
        }
    }

    return NULL;
}

// Multiplicación con hilos
void multiplicarMatrixHilos(int32_t **A, int32_t **B, int32_t **C, int n, int num_hilos) {
    pthread_t hilos[num_hilos];
    thread_data datos[num_hilos];

    int filas_por_hilo = n / num_hilos;
    int resto = n % num_hilos; // por si no divide exacto

    int fila_actual = 0;
    for (int i = 0; i < num_hilos; i++) {
        int filas_extra = (i < resto) ? 1 : 0; // repartir el resto
        datos[i].A = A;
        datos[i].B = B;
        datos[i].C = C;
        datos[i].n = n;
        datos[i].fila_inicio = fila_actual;
        datos[i].fila_fin = fila_actual + filas_por_hilo + filas_extra;
        fila_actual = datos[i].fila_fin;

        pthread_create(&hilos[i], NULL, multiplicarParte, &datos[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_hilos; i++) {
        pthread_join(hilos[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <tamano_matriz> [num_hilos]\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        printf("Tamaño inválido\n");
        return 1;
    }

    int num_hilos = (argc >= 3) ? atoi(argv[2]) : 4; // por defecto 4 hilos

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

    // Llenar matrices
    llenarMatrix(A, n);
    llenarMatrix(B, n);

    // Medir el tiempo de multiplicación de matrices
    clock_t start = clock();

    // Multiplicación con hilos
    multiplicarMatrixHilos(A, B, C, n, num_hilos);

    clock_t end = clock();

    // Calcular el tiempo de CPU utilizado
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo de CPU usado para la multiplicación: %f segundos con %d hilos\n", cpu_time_used, num_hilos);

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