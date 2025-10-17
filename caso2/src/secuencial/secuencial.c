#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/Secuencial_Data"

/* ======================================================
 * ESTRUCTURA DE MÉTRICAS DE RENDIMIENTO
 * ====================================================== */
typedef struct {
    double real_time;             // Tiempo real transcurrido (segundos)
    double user_time;             // Tiempo de CPU en modo usuario (segundos)
    double system_time;           // Tiempo de CPU en modo sistema (segundos)
    double total_cpu_time;        // user_time + system_time
    long long total_operations;   // Total de operaciones aritméticas estimadas
    double gops;                  // Rendimiento en miles de millones de operaciones por segundo
    double elements_per_second;   // Cantidad de elementos procesados por segundo (en millones)
    size_t memory_used;           // Memoria utilizada (MB)
} PerformanceStats;

/* ======================================================
 * FUNCIONES AUXILIARES
 * ====================================================== */

void createDirectoryIfNotExists(const char* dirPath) {
    if (!dirPath || *dirPath == '\0') return;

    char tmp[1024];
    size_t len = strlen(dirPath);
    if (len >= sizeof(tmp)) {
        fprintf(stderr, "Ruta demasiado larga.\n");
        exit(EXIT_FAILURE);
    }

    strcpy(tmp, dirPath);
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0700);
            *p = '/';
        }
    }
    mkdir(tmp, 0700);
}

char* generateFilename(const char* dirPath) {
    char* filename = malloc(256);
    sprintf(filename, "%s/Secuencial_Results.csv", dirPath);
    return filename;
}

void writeCSVHeaderIfNotExists(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        FILE* file = fopen(filename, "w");
        if (!file) {
            perror("Error creando CSV");
            exit(EXIT_FAILURE);
        }

        // Encabezados explicativos
        fprintf(file,
            "matrix_size,"
            "real_time_sec,"
            "user_time_sec,"
            "system_time_sec,"
            "total_cpu_time_sec,"
            "total_operations,"
            "performance_gops,"
            "elements_per_second_million,"
            "memory_used_mb,"
            "algorithm\n");

        fclose(file);
    }
}

void writeResultsToCSV(const char* filename, int size, PerformanceStats stats, const char* algorithm) {
    FILE* file = fopen(filename, "a");
    if (!file) {
        perror("Error escribiendo CSV");
        return;
    }

    fprintf(file,
        "%d,"           // matrix_size
        "%.9f,"         // real_time_sec
        "%.9f,"         // user_time_sec
        "%.9f,"         // system_time_sec
        "%.9f,"         // total_cpu_time_sec
        "%lld,"         // total_operations
        "%.6f,"         // performance_gops
        "%.6f,"         // elements_per_second_million
        "%lu,"          // memory_used_mb
        "%s\n",         // algorithm
        size,
        stats.real_time,
        stats.user_time,
        stats.system_time,
        stats.total_cpu_time,
        stats.total_operations,
        stats.gops,
        stats.elements_per_second,
        stats.memory_used,
        algorithm);

    fclose(file);
}

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

/* ======================================================
 * FUNCIONES DE MATRICES
 * ====================================================== */

int** createMatrix(int size) {
    int** matrix = malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++) {
        matrix[i] = malloc(size * sizeof(int));
        for (int j = 0; j < size; j++)
            matrix[i][j] = rand() % 100 + 1;
    }
    return matrix;
}

int** createResultMatrix(int size) {
    int** C = malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++)
        C[i] = calloc(size, sizeof(int));
    return C;
}

void freeMatrix(int** M, int size) {
    for (int i = 0; i < size; i++) free(M[i]);
    free(M);
}

/* ======================================================
 * MULTIPLICACIÓN SECUENCIAL
 * ====================================================== */

void multiplyMatrices(int** A, int** B, int** C, int size) {
    for (int i = 0; i < size; i++)
        for (int k = 0; k < size; k++) {
            int temp = A[i][k];
            for (int j = 0; j < size; j++)
                C[i][j] += temp * B[k][j];
        }
}

/* ======================================================
 * PROGRAMA PRINCIPAL
 * ====================================================== */

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <tamaño_matriz>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    srand(time(NULL));

    createDirectoryIfNotExists(DATA_DIR);
    char* csvFilename = generateFilename(DATA_DIR);
    writeCSVHeaderIfNotExists(csvFilename);

    printf("Creando matrices de %dx%d...\n", size, size);
    int** A = createMatrix(size);
    int** B = createMatrix(size);
    int** C = createResultMatrix(size);
    printf("Matrices creadas. Iniciando multiplicación secuencial...\n");

    PerformanceStats stats = {0};
    struct rusage start_usage, end_usage;
    struct timespec start_time, end_time;

    getrusage(RUSAGE_SELF, &start_usage);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    multiplyMatrices(A, B, C, size);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    getrusage(RUSAGE_SELF, &end_usage);

    stats.real_time = (end_time.tv_sec - start_time.tv_sec) +
                      (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);
    stats.system_time = timeval_to_seconds(end_usage.ru_stime) - timeval_to_seconds(start_usage.ru_stime);
    stats.total_cpu_time = stats.user_time + stats.system_time;

    stats.total_operations = (long long)size * size * (2 * size - 1);

    if (stats.real_time > 1e-9) {
        stats.gops = (stats.total_operations / stats.real_time) / 1e9;
        stats.elements_per_second = (double)(size * size) / stats.real_time / 1e6;
    }

    stats.memory_used = end_usage.ru_maxrss / 1024; // Memoria real en MB

    writeResultsToCSV(csvFilename, size, stats, "Secuencial");

    printf("\n===== RESULTADOS =====\n");
    printf("Tamaño de la matriz: %d x %d\n", size, size);
    printf("Tiempo real: %.9f s\n", stats.real_time);
    printf("Tiempo usuario: %.9f s\n", stats.user_time);
    printf("Tiempo sistema: %.9f s\n", stats.system_time);
    printf("Tiempo total CPU: %.9f s\n", stats.total_cpu_time);
    printf("Total operaciones: %lld\n", stats.total_operations);
    printf("Rendimiento: %.6f GOPS\n", stats.gops);
    printf("Elementos/s: %.6f millones\n", stats.elements_per_second);
    printf("Memoria usada: %lu MB\n", stats.memory_used);
    printf("Resultados guardados en: %s\n", csvFilename);

    freeMatrix(A, size);
    freeMatrix(B, size);
    freeMatrix(C, size);
    free(csvFilename);

    return EXIT_SUCCESS;
}
