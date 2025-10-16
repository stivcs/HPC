#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <omp.h> // 🔹 Importante para OpenMP

#define DATA_DIR "Secuencial_Data"

typedef struct {
    double user_time;   // Solo nos interesa el tiempo de usuario
} PerformanceStats;

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

/* 
 * 🔹 Versión paralela con OpenMP
 * Se paraleliza el bucle externo (i) para dividir filas entre hilos.
 * Cada hilo trabaja sobre filas distintas, sin interferir entre sí.
 */
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

void createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0700) != 0) {
            fprintf(stderr, "Error: No se pudo crear el directorio %s\n", dirPath);
            exit(EXIT_FAILURE);
        }
    }
}

char* generateFilename(const char* dirPath) {
    char* filename = (char*)malloc(256);
    if (filename == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nombre de archivo\n");
        exit(EXIT_FAILURE);
    }
    sprintf(filename, "%s/OpenMP_Results.csv", dirPath); // 🔹 Cambiado para diferenciar del secuencial
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
        fprintf(file, "size,user_time,threads\n");
        fclose(file);
    }
}

void writeResultsToCSV(const char* filename, int size, PerformanceStats stats, int threads) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo CSV para escritura\n");
        return;
    }
    fprintf(file, "%d,%.9f,%d\n", size, stats.user_time, threads);
    fclose(file);
}

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> <num_hilos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    int threads = atoi(argv[2]);
    if (size <= 0 || threads <= 0) {
        fprintf(stderr, "Error: Los parámetros deben ser números positivos\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    omp_set_num_threads(threads); // 🔹 Establecer número de hilos

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

    if (getrusage(RUSAGE_SELF, &start_usage) < 0) perror("Error en getrusage inicial");
    multiplyMatrices(A, B, C, size);
    if (getrusage(RUSAGE_SELF, &end_usage) < 0) perror("Error en getrusage final");

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
