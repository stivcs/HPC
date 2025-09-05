#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>

// Número de hilos global
#define NUM_THREADS 2
pthread_t threads[NUM_THREADS];

// Estructura para pasar datos a cada hilo
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

// Función que ejecutará cada hilo
void *multiplicar_parcial(void *arg) {
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

// Verificar multiplicación en posiciones aleatorias
void verificarMultiplicacion(int32_t **A, int32_t **B, int32_t **C, int n, int pruebas) {
    for (int p = 0; p < pruebas; p++) {
        int i = rand() % n;
        int j = rand() % n;

        int32_t esperado = 0;
        for (int k = 0; k < n; k++) {
            esperado += A[i][k] * B[k][j];
        }

        if (C[i][j] =! esperado) {
            printf("✘ ERROR en C[%d][%d]: obtenido=%d, esperado=%d\n", i, j, C[i][j], esperado);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <tamaño_matriz>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        printf("Tamaño inválido\n");
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

    // Llenar matrices
    llenarMatrix(A, n);
    llenarMatrix(B, n);

    // ===== SOLO TIEMPO DE MULTIPLICACIÓN =====
    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);
    clock_t start = clock();

    // Crear hilos para la multiplicación
    thread_data data[NUM_THREADS];
    int filas_por_hilo = n / NUM_THREADS;

    for (int t = 0; t < NUM_THREADS; t++) {
        data[t].A = A;
        data[t].B = B;
        data[t].C = C;
        data[t].n = n;
        data[t].fila_inicio = t * filas_por_hilo;
        // Último hilo toma las filas sobrantes
        data[t].fila_fin = (t == NUM_THREADS - 1) ? n : (t + 1) * filas_por_hilo;

        pthread_create(&threads[t], NULL, multiplicar_parcial, &data[t]);
    }

    // Esperar a los hilos
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    clock_t end = clock();
    getrusage(RUSAGE_SELF, &usage_end);
    // ===== FIN DE TIEMPO DE MULTIPLICACIÓN =====

    // Calcular tiempo de CPU
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Calcular tiempo de usuario y sistema
    double user_time = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
                       (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;
    double sys_time = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
                      (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;

    printf("Tiempo de CPU usado (multiplicación): %f segundos\n", cpu_time_used);
    printf("Tiempo de usuario (multiplicación): %f segundos\n", user_time);
    printf("Tiempo de sistema (multiplicación): %f segundos\n", sys_time);

    // Verificar multiplicación en 5 posiciones aleatorias
    verificarMultiplicacion(A, B, C, n, 5);

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