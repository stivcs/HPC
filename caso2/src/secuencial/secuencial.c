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

typedef struct {
    double user_time;
} PerformanceStats;

/* ======================================================
 * FUNCIONES AUXILIARES
 * ====================================================== */

void createDirectoryIfNotExists(const char* dirPath) {
    if (dirPath == NULL || *dirPath == '\0') return;

    char tmp[1024];
    strcpy(tmp, dirPath);
    size_t len = strlen(tmp);
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
 * MULTIPLICACIÓN
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
 * GUARDADO DE MATRICES EN CSV
 * ====================================================== */

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

/* ======================================================
 * PROGRAMA PRINCIPAL
 * ====================================================== */

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> [save]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    int saveMatrices = (argc >= 3 && strcmp(argv[2], "save") == 0);

    srand(time(NULL));

    createDirectoryIfNotExists(DATA_DIR);
    if (saveMatrices)
        createDirectoryIfNotExists(DATA_DIR "/matrices");

    printf("Creando matrices de %dx%d...\n", size, size);
    int** A = createMatrix(size);
    int** B = createMatrix(size);
    int** C = createResultMatrix(size);
    printf("Matrices creadas. Iniciando multiplicación...\n");

    PerformanceStats stats = {0};
    struct rusage start_usage, end_usage;

    getrusage(RUSAGE_SELF, &start_usage);
    multiplyMatrices(A, B, C, size);
    getrusage(RUSAGE_SELF, &end_usage);

    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);

    printf("\n===== RESULTADOS =====\n");
    printf("Tamaño de la matriz: %d x %d\n", size, size);
    printf("Tiempo de usuario: %.9f segundos\n", stats.user_time);

    if (saveMatrices) {
        printf("Guardando matrices en CSV...\n");
        saveMatrixCSV(DATA_DIR "/matrices", "A", A, size);
        saveMatrixCSV(DATA_DIR "/matrices", "B", B, size);
        saveMatrixCSV(DATA_DIR "/matrices", "C_resultado", C, size);
    }

    freeMatrix(A, size);
    freeMatrix(B, size);
    freeMatrix(C, size);

    return EXIT_SUCCESS;
}
