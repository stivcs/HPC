#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>  // Para getrusage
#include <string.h>
#include <sys/stat.h>      // Para mkdir/stat
#include <errno.h>         // Para manejo de errores
#include <unistd.h>        // Para sleep - útil para depuración

// Nombre del directorio para almacenamiento de datos
#define DATA_DIR "Lineal_Data"

// Estructura para almacenar estadísticas de rendimiento
typedef struct {
    double execution_time;
    double real_time;       // tiempo real en segundos (wall clock time)
    double user_time;       // tiempo de CPU en modo usuario
    double system_time;     // tiempo de CPU en modo kernel
    double total_cpu_time;  // suma de user_time y system_time
    long long total_operations;  // número total de operaciones aritméticas
    double gops; // operaciones de punto flotante por segundo (en miles de millones)
    double elements_per_second; // elementos procesados por segundo (millones)
    size_t memory_used; // memoria aproximada en MB
} PerformanceStats;

// Función para crear y llenar una matriz con números aleatorios
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
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
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
    for (int i = 0; i < size; i++) {
        free(matrix[i]);
    }
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
            fprintf(stderr, "Error: No se pudo asignar memoria para la fila %d del resultado\n", i);
            for (int j = 0; j < i; j++) {
                free(C[j]);
            }
            free(C);
            exit(EXIT_FAILURE);
        }
    }
    return C;
}

// Función que EXCLUSIVAMENTE realiza la multiplicación de matrices (sin cambios)
void multiplyMatrices(int** A, int** B, int** C, int size) {    
    for (int i = 0; i < size; i++) {
        for (int k = 0; k < size; k++) {
            for (int j = 0; j < size; j++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Función para verificar resultado
void validateResult(int** A, int** B, int** C, int size) {
    int expected, actual;
    int correct = 1;
    
    for (int i = 0; i < size && correct; i++) {
        for (int j = 0; j < size && correct; j++) {
            expected = 0;
            for (int k = 0; k < size; k++) {
                expected += A[i][k] * B[k][j];
            }
            actual = C[i][j];
            if (expected != actual) {
                printf("Error de validación en C[%d][%d]: esperado %d, obtenido %d\n", 
                       i, j, expected, actual);
                correct = 0;
            }
        }
    }
    
    if (correct) {
        printf("Validación: Correcta\n");
    }
}

// Función para crear el directorio si no existe
void createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        #ifdef _WIN32
            int status = _mkdir(dirPath);
        #else
            int status = mkdir(dirPath, 0700); // Versión Linux/Unix, con permisos
        #endif
        
        if (status != 0) {
            fprintf(stderr, "Error: No se pudo crear el directorio %s (errno: %d)\n", 
                    dirPath, errno);
            exit(EXIT_FAILURE);
        }
    }
}

// NUEVA FUNCIÓN: Generar nombre de archivo fijo
char* generateFilename(const char* dirPath) {
    char* filename = (char*)malloc(256);
    if (filename == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nombre de archivo\n");
        exit(EXIT_FAILURE);
    }
    sprintf(filename, "%s/Lineal_Results.csv", dirPath);
    return filename;
}

// NUEVA FUNCIÓN: Escribir cabecera solo si el archivo no existe
void writeCSVHeaderIfNotExists(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {  // Si no existe, lo creamos y escribimos cabecera
        FILE* file = fopen(filename, "w");
        if (file == NULL) {
            fprintf(stderr, "Error: No se pudo crear el archivo CSV %s\n", filename);
            exit(EXIT_FAILURE);
        }
        fprintf(file, "size,iteration,real_time,user_time,system_time,total_cpu_time,"
                      "total_operations,gops,elements_per_second_millions,memory_used_mb,num_processes,algorithm\n");
        fclose(file);
    }
}

// Función para escribir resultados en un archivo CSV
void writeResultsToCSV(const char* filename, int size, int iteration, PerformanceStats stats, int num_processes, const char* algorithm) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo CSV para escritura\n");
        return;
    }
    fprintf(file, "%d,%d,%.9f,%.9f,%.9f,%.9f,%lld,%.6f,%.6f,%lu,%d,%s\n",
            size,
            iteration,
            stats.real_time,
            stats.user_time,
            stats.system_time,
            stats.total_cpu_time,
            stats.total_operations,
            stats.gops,
            stats.elements_per_second,
            stats.memory_used,
            num_processes,
            algorithm);
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> [iteración]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Error: El tamaño de la matriz debe ser un número positivo\n");
        return EXIT_FAILURE;
    }
    
    int iteration = 1;
    if (argc == 3) {
        iteration = atoi(argv[2]);
    }
    
    srand(time(NULL));
    
    createDirectoryIfNotExists(DATA_DIR);
    
    // Usar archivo fijo
    char* csvFilename = generateFilename(DATA_DIR);
    
    // Escribir cabecera si no existe
    writeCSVHeaderIfNotExists(csvFilename);
    
    printf("Creando matrices de %dx%d...\n", size, size);
    int** A = createMatrix(size);
    int** B = createMatrix(size);
    int** C = createResultMatrix(size);
    
    PerformanceStats stats = {0};
    
    printf("Iniciando multiplicación de matrices...\n");
    
    struct timespec start_time, end_time;
    struct rusage start_usage, end_usage;
    
    fflush(stdout);
    fflush(stderr);
    
    if (getrusage(RUSAGE_SELF, &start_usage) < 0) {
        perror("Error en getrusage inicial");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    multiplyMatrices(A, B, C, size);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    if (getrusage(RUSAGE_SELF, &end_usage) < 0) {
        perror("Error en getrusage final");
    }
    
    stats.real_time = (end_time.tv_sec - start_time.tv_sec) + 
                      (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    stats.execution_time = stats.real_time;
    
    double timeval_to_seconds(struct timeval tv) {
        return tv.tv_sec + (tv.tv_usec / 1000000.0);
    }
    
    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);
    stats.system_time = timeval_to_seconds(end_usage.ru_stime) - timeval_to_seconds(start_usage.ru_stime);
    stats.total_cpu_time = stats.user_time + stats.system_time;
    
    stats.total_operations = (long long)size * size * (2 * size - 1);
    
    if (stats.execution_time > 1e-9) {
        stats.gops = (stats.total_operations / stats.execution_time) / 1000000000.0;
        stats.elements_per_second = (double)(size * size) / stats.execution_time / 1000000.0;
    } else {
        stats.gops = 0.0;
        stats.elements_per_second = 0.0;
    }
    
    stats.memory_used = (3 * size * size * sizeof(int)) / (1024 * 1024);
    
    writeResultsToCSV(csvFilename, size, iteration, stats, 1, "lineal");
    
    printf("\n===== ESTADÍSTICAS DE RENDIMIENTO =====\n");
    printf("Tamaño de la matriz: %d x %d\n", size, size);
    printf("Tiempo real (wall clock): %.9f segundos\n", stats.real_time);
    printf("Tiempo de CPU: %.9f segundos (%.2f%% de utilización)\n", 
        stats.total_cpu_time, 
        (stats.total_cpu_time / stats.real_time) * 100);
    printf("  └─ Tiempo de usuario: %.9f segundos (%.2f%%)\n", 
        stats.user_time, 
        (stats.user_time / stats.total_cpu_time) * 100);
    printf("  └─ Tiempo de sistema: %.9f segundos (%.2f%%)\n", 
        stats.system_time, 
        (stats.system_time / stats.total_cpu_time) * 100);
    printf("Total de operaciones: %lld\n", stats.total_operations);
    printf("Rendimiento: %.6f GOPS\n", stats.gops);
    printf("Elementos procesados por segundo: %.6f millones\n", stats.elements_per_second);
    printf("Memoria utilizada: aproximadamente %lu MB\n", stats.memory_used);
    printf("Datos guardados en: %s\n", csvFilename);
    
    freeMatrix(A, size);
    freeMatrix(B, size);
    freeMatrix(C, size);
    free(csvFilename);
    
    return EXIT_SUCCESS;
}
