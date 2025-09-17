#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define DATA_DIR "Hilos_Data"
#define CSV_FILE DATA_DIR "/tiempos.csv"

// Convierte timeval a segundos
static inline double timeval_to_seconds(struct timeval tv) {
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

// Estructura de estadísticas (solo user time)
typedef struct {
    double user_time;
} PerformanceStats;

// Datos que recibe cada hilo
typedef struct {
    int thread_id;
    int num_threads;
    int size;
    int **A, **B, **C;
} ThreadData;

// Crear directorio si no existe
int createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0755) == -1) {
            fprintf(stderr, "Error creando directorio %s: %s\n",
                    dirPath, strerror(errno));
            return 0;
        }
    }
    return 1;
}

// Crear matrices
int** createMatrix(int size) {
    int **M = malloc(size * sizeof(int*));
    if (!M) { perror("malloc"); exit(1); }
    for (int i = 0; i < size; i++) {
        M[i] = malloc(size * sizeof(int));
        if (!M[i]) { perror("malloc fila"); exit(1); }
        for (int j = 0; j < size; j++) {
            M[i][j] = rand() % 100;
        }
    }
    return M;
}

int** createResultMatrix(int size) {
    int **M = malloc(size * sizeof(int*));
    if (!M) { perror("malloc"); exit(1); }
    for (int i = 0; i < size; i++) {
        M[i] = calloc(size, sizeof(int));
        if (!M[i]) { perror("calloc fila"); exit(1); }
    }
    return M;
}

void freeMatrix(int** M, int size) {
    for (int i = 0; i < size; i++) free(M[i]);
    free(M);
}

// Multiplicación parcial por hilo
void* multiplyThread(void* arg) {
    ThreadData* d = arg;
    for (int i = d->thread_id; i < d->size; i += d->num_threads) {
        for (int j = 0; j < d->size; j++) {
            int sum = 0;
            for (int k = 0; k < d->size; k++) {
                sum += d->A[i][k] * d->B[k][j];
            }
            d->C[i][j] = sum;
        }
    }
    return NULL;
}

// Multiplicación y tiempos
void multiplyMatrices(int** A, int** B, int** C, int size, int num_threads, PerformanceStats* stats) {
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *data = malloc(num_threads * sizeof(ThreadData));

    struct rusage start, end;
    getrusage(RUSAGE_SELF, &start);

    for (int t = 0; t < num_threads; t++) {
        data[t].thread_id = t;
        data[t].num_threads = num_threads;
        data[t].size = size;
        data[t].A = A;
        data[t].B = B;
        data[t].C = C;
        pthread_create(&threads[t], NULL, multiplyThread, &data[t]);
    }
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);

    getrusage(RUSAGE_SELF, &end);

    stats->user_time = timeval_to_seconds(end.ru_utime) - timeval_to_seconds(start.ru_utime);

    free(threads);
    free(data);
}

// Escribir cabecera si no existe
void ensureCSVHeader(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        // No existe, lo creo con cabecera
        f = fopen(filename, "w");
        if (!f) { perror("fopen"); exit(1); }
        fprintf(f, "matrix_size,num_threads,user_time\n");
    }
    if (f) fclose(f);
}

// Guardar resultados
void appendResult(const char* filename, int size, int num_threads, PerformanceStats stats) {
    FILE* f = fopen(filename, "a");
    if (!f) { perror("fopen"); exit(1); }
    fprintf(f, "%d,%d,%.9f\n", size, num_threads, stats.user_time);
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> <num_hilos>\n", argv[0]);
        return 1;
    }
    int size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    srand(time(NULL));
    createDirectoryIfNotExists(DATA_DIR);
    ensureCSVHeader(CSV_FILE);

    printf("Creando matrices de %dx%d...\n", size, size);
    int **A = createMatrix(size);
    int **B = createMatrix(size);
    int **C = createResultMatrix(size);

    printf("Matrices creadas. Iniciando multiplicación con %d hilos...\n", num_threads);
    PerformanceStats stats;
    multiplyMatrices(A, B, C, size, num_threads, &stats);

    appendResult(CSV_FILE, size, num_threads, stats);

    printf("\n===== Resultados =====\n");
    printf("Tamaño matriz: %d, Hilos: %d\n", size, num_threads);
    printf("User time: %.9f s\n", stats.user_time);

    freeMatrix(A, size);
    freeMatrix(B, size);
    freeMatrix(C, size);

    return 0;
}