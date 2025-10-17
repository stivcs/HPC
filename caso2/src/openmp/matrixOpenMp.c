#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <omp.h>
#include <errno.h>
#include <string.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/OpenMp_Data"

// ======================================================
// Estructura para registrar métricas de rendimiento
// ======================================================
typedef struct {
    double user_time;
} PerformanceStats;

// ======================================================
// Función auxiliar: convertir timeval a segundos
// ======================================================
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// ======================================================
// Crear directorios recursivamente (similar a mkdir -p)
// ======================================================
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

    for (char* p = tmp + 1; *p; p++) {
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

// ======================================================
// Creación y liberación de matrices
// ======================================================
int** createMatrix(int size) {
    int** matrix = (int**)malloc(size * sizeof(int*));
    if (!matrix) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la matriz\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++) {
        matrix[i] = (int*)malloc(size * sizeof(int));
        if (!matrix[i]) {
            fprintf(stderr, "Error: no se pudo asignar memoria para la fila %d\n", i);
            for (int j = 0; j < i; j++) free(matrix[j]);
            free(matrix);
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < size; j++) {
            matrix[i][j] = rand() % 100 + 1;
        }
    }
    return matrix;
}

void freeMatrix(int** matrix, int size) {
    for (int i = 0; i < size; i++) free(matrix[i]);
    free(matrix);
}

int** createResultMatrix(int size) {
    int** C = (int**)malloc(size * sizeof(int*));
    if (!C) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la matriz resultado\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++) {
        C[i] = (int*)calloc(size, sizeof(int));
        if (!C[i]) {
            fprintf(stderr, "Error: no se pudo asignar memoria para la fila %d\n", i);
            for (int j = 0; j < i; j++) free(C[j]);
            free(C);
            exit(EXIT_FAILURE);
        }
    }
    return C;
}

// ======================================================
// Multiplicación paralela con OpenMP
// ======================================================
void multiplyMatrices(int** A, int** B, int** C, int size) {
    #pragma omp parallel for collapse(2) shared(A, B, C)
    for (int i = 0; i < size; i++) {
        for (int k = 0; k < size; k++) {
            int tempA = A[i][k];
            for (int j = 0; j < size; j++) {
                C[i][j] += tempA * B[k][j];
            }
        }
    }
}

// ======================================================
// Manejo de CSV
// ======================================================
char* generateFilename(const char* dirPath) {
    char* filename = (char*)malloc(256);
    if (!filename) {
        fprintf(stderr, "Error: no se pudo asignar memoria para el nombre de archivo\n");
        exit(EXIT_FAILURE);
    }
    sprintf(filename, "%s/OpenMP_Results.csv", dirPath);
    return filename;
}

void writeCSVHeaderIfNotExists(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        FILE* file = fopen(filename, "w");
        if (!file) {
            fprintf(stderr, "Error: no se pudo crear el archivo CSV %s\n", filename);
            exit(EXIT_FAILURE);
        }
        fprintf(file, "size,user_time,threads\n");
        fclose(file);
    }
}

void writeResultsToCSV(const char* filename, int size, PerformanceStats stats, int threads) {
    FILE* file = fopen(filename, "a");
    if (!file) {
        fprintf(stderr, "Error: no se pudo abrir el archivo CSV para escritura\n");
        return;
    }
    fprintf(file, "%d,%.9f,%d\n", size, stats.user_time, threads);
    fclose(file);
}

// ======================================================
// Programa principal
// ======================================================
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> <num_hilos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    int threads = atoi(argv[2]);
    if (size <= 0 || threads <= 0) {
        fprintf(stderr, "Error: los parámetros deben ser números positivos\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    omp_set_num_threads(threads);

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

    if (getrusage(RUSAGE_SELF, &start_usage) < 0)
        perror("Error en getrusage inicial");

    multiplyMatrices(A, B, C, size);

    if (getrusage(RUSAGE_SELF, &end_usage) < 0)
        perror("Error en getrusage final");

    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);

    writeResultsToCSV(csvFilename, size, stats, threads);

    printf("\n===== Resultados =====\n");
    printf("Tamaño de la matriz: %d x %d\n", size, size);
    printf("Hilos utilizados: %d\n", threads);
    printf("Tiempo de usuario: %.9f segundos\n", stats.user_time);

    freeMatrix(A, size);
    freeMatrix(B, size);
    freeMatrix(C, size);
    free(csvFilename);

    return EXIT_SUCCESS;
}
