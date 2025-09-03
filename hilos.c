#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>

// ========================= ESTADÍSTICAS =========================
typedef struct {
    double execution_time;
    double real_time;       // tiempo real en segundos (wall clock time)
    double user_time;       // tiempo de CPU en modo usuario
    double system_time;     // tiempo de CPU en modo kernel
    double total_cpu_time;  // suma de user_time y system_time
    long long total_operations;  // número total de operaciones aritméticas
    double gops; // operaciones por segundo (en miles de millones)
    double elements_per_second; // elementos procesados por segundo (millones)
    size_t memory_used; // memoria aproximada en MB
} PerformanceStats;

// Convierte timeval a segundos
static inline double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// ========================= MULTIPLICACIÓN CON HILOS =========================
typedef struct {
    int32_t **A;
    int32_t **B;
    int32_t **C;
    int n;
    int fila_inicio;
    int fila_fin;
} thread_data;

void llenarMatrix(int32_t **matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (rand() % 101) - 50;  // [-50, 50]
        }
    }
}

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

void multiplicarMatrixHilos(int32_t **A, int32_t **B, int32_t **C, int n, int num_hilos) {
    pthread_t hilos[num_hilos];
    thread_data datos[num_hilos];

    int filas_por_hilo = n / num_hilos;
    int resto = n % num_hilos;

    int fila_actual = 0;
    for (int i = 0; i < num_hilos; i++) {
        int filas_extra = (i < resto) ? 1 : 0;
        datos[i].A = A;
        datos[i].B = B;
        datos[i].C = C;
        datos[i].n = n;
        datos[i].fila_inicio = fila_actual;
        datos[i].fila_fin = fila_actual + filas_por_hilo + filas_extra;
        fila_actual = datos[i].fila_fin;

        pthread_create(&hilos[i], NULL, multiplicarParte, &datos[i]);
    }

    for (int i = 0; i < num_hilos; i++) {
        pthread_join(hilos[i], NULL);
    }
}

// ========================= MAIN =========================
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

    int num_hilos = (argc >= 3) ? atoi(argv[2]) : 4; // default 4 hilos
    srand((unsigned int)time(NULL));

    // Reservar memoria
    int32_t **A = malloc(n * sizeof(int32_t *));
    int32_t **B = malloc(n * sizeof(int32_t *));
    int32_t **C = malloc(n * sizeof(int32_t *));
    for (int i = 0; i < n; i++) {
        A[i] = malloc(n * sizeof(int32_t));
        B[i] = malloc(n * sizeof(int32_t));
        C[i] = malloc(n * sizeof(int32_t));
    }

    llenarMatrix(A, n);
    llenarMatrix(B, n);

    PerformanceStats stats = {0};

    // Medición
    struct timespec start_time, end_time;
    struct rusage start_usage, end_usage;

    getrusage(RUSAGE_SELF, &start_usage);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    multiplicarMatrixHilos(A, B, C, n, num_hilos);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    getrusage(RUSAGE_SELF, &end_usage);

    // Cálculos de tiempo
    stats.real_time = (end_time.tv_sec - start_time.tv_sec) +
                      (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    stats.execution_time = stats.real_time;
    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);
    stats.system_time = timeval_to_seconds(end_usage.ru_stime) - timeval_to_seconds(start_usage.ru_stime);
    stats.total_cpu_time = stats.user_time + stats.system_time;

    // Operaciones: aproximación clásica O(n^3)
    stats.total_operations = (long long)n * n * (2 * n - 1);
    stats.gops = (stats.total_operations / stats.execution_time) / 1e9;
    stats.elements_per_second = ((double)(n * n) / stats.execution_time) / 1e6;

    stats.memory_used = (3 * n * n * sizeof(int32_t)) / (1024 * 1024);

    // Mostrar resultados
    printf("\n===== ESTADÍSTICAS DE RENDIMIENTO =====\n");
    printf("Tamaño de la matriz: %d x %d\n", n, n);
    printf("Número de hilos: %d\n", num_hilos);
    printf("Tiempo real (wall clock): %.9f segundos\n", stats.real_time);
    printf("Tiempo de CPU total: %.9f segundos\n", stats.total_cpu_time);
    printf("  └─ Usuario: %.9f s\n", stats.user_time);
    printf("  └─ Sistema: %.9f s\n", stats.system_time);
    printf("Total de operaciones: %lld\n", stats.total_operations);
    printf("Rendimiento: %.6f GOPS\n", stats.gops);
    printf("Elementos procesados por segundo: %.6f millones\n", stats.elements_per_second);
    printf("Memoria utilizada: ~%lu MB\n", stats.memory_used);

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