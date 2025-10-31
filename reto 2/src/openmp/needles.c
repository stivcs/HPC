#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <omp.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/openmp/Needles_Data"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    double real_time;
    double pi_est;
} PerformanceStats;

// Convierte timespec a segundos
static inline double timespec_to_seconds(struct timespec ts) {
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// Crear directorio si no existe
int createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0755) == -1) {
            fprintf(stderr, "Error creando %s: %s\n", dirPath, strerror(errno));
            return 0;
        }
    }
    return 1;
}

// Cabecera CSV
void writeCSVHeaderIfNeeded(const char* filename) {
    FILE* f = fopen(filename, "a+");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) fprintf(f, "N,num_threads,pi_est,real_time\n");
    fclose(f);
}

// Guardar resultados
void appendResults(const char* filename, long N, int num_threads, PerformanceStats stats) {
    FILE* f = fopen(filename, "a");
    if (!f) return;
    fprintf(f, "%ld,%d,%.9f,%.9f\n", N, num_threads, stats.pi_est, stats.real_time);
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N> <num_threads>\n", argv[0]);
        return 1;
    }

    long N = atol(argv[1]);
    int num_threads = atoi(argv[2]);

    // Parámetros fijos
    double needle_len = 1.0;
    double dist = 2.0;

    if (!createDirectoryIfNotExists(DATA_DIR)) return EXIT_FAILURE;

    char filename[256];
    snprintf(filename, sizeof(filename), DATA_DIR "/results_%dthreads.csv", num_threads);
    writeCSVHeaderIfNeeded(filename);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long total_hits = 0;

    // Paralelismo con OpenMP
    #pragma omp parallel for reduction(+:total_hits) num_threads(num_threads)
    for (long i = 0; i < N; i++) {
        unsigned int seed = (unsigned int)(time(NULL) ^ (omp_get_thread_num() * 7919) ^ i);
        double y = ((double)rand_r(&seed) / RAND_MAX) * (dist / 2.0);
        double theta = ((double)rand_r(&seed) / RAND_MAX) * (M_PI / 2.0);
        if (y <= (needle_len / 2.0) * sinf((float)theta)) {
            total_hits++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    PerformanceStats stats;
    stats.pi_est = (total_hits > 0) ? (2.0 * needle_len * N) / (dist * total_hits) : 0.0;
    stats.real_time = timespec_to_seconds(end) - timespec_to_seconds(start);

    appendResults(filename, N, num_threads, stats);

    printf("PI Buffon con %d hilos (OpenMP): %.9f\n", num_threads, stats.pi_est);
    printf("Tiempo real: %.9f s\n", stats.real_time);
    printf("Guardado en: %s\n", filename);

    return 0;
}

