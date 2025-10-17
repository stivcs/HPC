#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <omp.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/OpenMp_Data"

/* ==========================================
 * Estructura de métricas de rendimiento
 * ========================================== */
typedef struct {
    double real_time;
    double user_time;
    double system_time;
    double total_cpu_time;
    long long total_operations;
    double gops;
    double elements_per_second;
    size_t memory_used;
} PerformanceStats;

/* ==========================================
 * Funciones auxiliares de directorios y CSV
 * ========================================== */
void createDirectoryIfNotExists(const char* dirPath) {
    if (dirPath == NULL || *dirPath == '\0') return;

    char tmp[1024];
    size_t len = strlen(dirPath);
    if (len >= sizeof(tmp)) {
        fprintf(stderr, "Error: ruta demasiado larga: %s\n", dirPath);
        exit(EXIT_FAILURE);
    }

    strcpy(tmp, dirPath);
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
                fprintf(stderr, "Error creando directorio '%s': %s\n", tmp, strerror(errno));
                exit(EXIT_FAILURE);
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error creando directorio '%s': %s\n", tmp, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

char* generateFilename(const char* dirPath) {
    char* filename = (char*)malloc(256);
    if (filename == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nombre de archivo\n");
        exit(EXIT_FAILURE);
    }
    sprintf(filename, "%s/OpenMP_Results.csv", dirPath);
    return filename;
}

void writeCSVHeaderIfNotExists(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        FILE* file = fopen(filename, "w");
        if (file == NULL) {
            fprintf(stderr, "Error: No se pudo crear el archivo CSV %s\n", filename);
            exit(EXIT_FAILURE);
        }
        fprintf(file,
            "size,threads,real_time,user_time,system_time,total_cpu_time,"
            "total_operations,gops,elements_per_second_millions,memory_used_mb,algorithm\n");
        fclose(file);
    }
}

void writeResultsToCSV(const char* filename, int size, int threads, PerformanceStats stats, const char* algorithm) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo CSV para escritura\n");
        return;
    }

    fprintf(file, "%d,%d,%.9f,%.9f,%.9f,%.9f,%lld,%.6f,%.6f,%lu,%s\n",
        size,
        threads,
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

/* ==========================================
 * Funciones de manejo de matrices
 * ========================================== */
int** createMatrix(int size) {
    int** matrix = (int**)malloc(size * sizeof(int*));
    if (matrix == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la matriz\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++) {
        matrix[i] = (int*)malloc(size * sizeof(int));
        if (matrix[i] == NULL) {
            fprintf(stderr, "Error: No se pudo asignar memoria para la fila %d\n", i);
            for (int j = 0; j < i; j++) free(matrix[j]);
            free(matrix);
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < size; j++)
            matrix[i][j] = rand() % 100 + 1;
    }
    return matrix;
}

int** createResultMatrix(int size) {
    int** C = (int**)malloc(size * sizeof(int*));
    if (C == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la matriz resultado\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++) {
        C[i] = (int*)calloc(size, sizeof(int));
        if (C[i] == NULL) {
            fprintf(stderr, "Error: No se pudo asignar memoria para la fila %d\n", i);
            for (int j = 0; j < i; j++) free(C[j]);
            free(C);
            exit(EXIT_FAILURE);
        }
    }
    return C;
}

void freeMatrix(int** M, int size) {
    for (int i = 0; i < size; i++) free(M[i]);
    free(M);
}

/* ==========================================
 * Multiplicación con OpenMP
 * ========================================== */
void multiplyMatricesOMP(int** A, int** B, int** C, int size, int threads) {
    #pragma omp parallel for collapse(2) num_threads(threads) shared(A, B, C)
    for (int i = 0; i < size; i++) {
        for (int k = 0; k < size; k++) {
            int temp = A[i][k];
            for (int j = 0; j < size; j++) {
                C[i][j] += temp * B[k][j];
            }
        }
    }
}

/* ==========================================
 * Guardado de matrices en CSV
 * ========================================== */
void saveMatrixCSV(const char* dir, const char* name, int** M, int size) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.csv", dir, name);

    FILE* file = fopen(path, "w");
    if (!file) {
        perror("Error al crear archivo de matriz");
        return;
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            fprintf(file, "%d", M[i][j]);
            if (j < size - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }

    fclose(file);
    printf("Guardada matriz: %s\n", path);
}

/* ==========================================
 * Programa principal
 * ========================================== */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> <num_hilos> [save]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    int threads = atoi(argv[2]);
    int saveMatrices = (argc >= 4 && strcmp(argv[3], "save") == 0);

    if (size <= 0 || threads <= 0) {
        fprintf(stderr, "Error: tamaño y número de hilos deben ser positivos.\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    createDirectoryIfNotExists(DATA_DIR);
    char* csvFilename = generateFilename(DATA_DIR);
    writeCSVHeaderIfNotExists(csvFilename);

    printf("Creando matrices de %dx%d...\n", size, size);
    int** A = createMatrix(size);
    int** B = createMatrix(size);
    int** C = createResultMatrix(size);
    printf("Matrices creadas. Iniciando multiplicación con %d hilos...\n", threads);

    PerformanceStats stats = {0};
    struct rusage start_usage, end_usage;
    struct timespec start_time, end_time;

    getrusage(RUSAGE_SELF, &start_usage);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    multiplyMatricesOMP(A, B, C, size, threads);

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
    } else {
        stats.gops = 0.0;
        stats.elements_per_second = 0.0;
    }

    stats.memory_used = end_usage.ru_maxrss / 1024;

    writeResultsToCSV(csvFilename, size, threads, stats, "openmp");

    printf("\n===== RESULTADOS OPENMP =====\n");
    printf("Tamaño de la matriz: %d x %d\n", size, size);
    printf("Hilos utilizados: %d\n", threads);
    printf("Tiempo real: %.9f s\n", stats.real_time);
    printf("Tiempo usuario: %.9f s\n", stats.user_time);
    printf("Tiempo sistema: %.9f s\n", stats.system_time);
    printf("Tiempo total CPU: %.9f s\n", stats.total_cpu_time);
    printf("Total operaciones: %lld\n", stats.total_operations);
    printf("Rendimiento: %.6f GOPS\n", stats.gops);
    printf("Elementos/s: %.6f millones\n", stats.elements_per_second);
    printf("Memoria usada: %lu MB\n", stats.memory_used);
    printf("Datos guardados en: %s\n", csvFilename);

    if (saveMatrices) {
        printf("Guardando matrices en CSV...\n");
        createDirectoryIfNotExists(DATA_DIR "/matrices");
        saveMatrixCSV(DATA_DIR "/matrices", "A", A, size);
        saveMatrixCSV(DATA_DIR "/matrices", "B", B, size);
        saveMatrixCSV(DATA_DIR "/matrices", "C_resultado", C, size);
    }

    freeMatrix(A, size);
    freeMatrix(B, size);
    freeMatrix(C, size);
    free(csvFilename);

    return EXIT_SUCCESS;
}
