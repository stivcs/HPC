#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/resource.h>
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
    double user_time;
    double system_time;
    double total_cpu_time;
    long long total_operations;
    double gops;
    double elements_per_second;
    size_t memory_used;
    double pi_est;
} PerformanceStats;

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1e6);
}

double timespec_to_seconds(struct timespec ts) {
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}

void createDirectoryIfNotExists(const char* dirPath) {
    if (dirPath == NULL) return;
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0755) == -1) {
            fprintf(stderr, "Error creando %s: %s\n", dirPath, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void writeCSVHeaderIfNotExists(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        FILE* f = fopen(filename, "w");
        if (!f) {
            fprintf(stderr, "Error creando CSV %s\n", filename);
            exit(EXIT_FAILURE);
        }
        fprintf(f, "size,threads,real_time,user_time,system_time,total_cpu_time,"
                   "total_operations,gops,elements_per_second_millions,memory_used_mb,algorithm,pi_est\n");
        fclose(f);
    }
}

void appendResults(const char* filename, long N, int threads, PerformanceStats s, const char* algorithm) {
    FILE* f = fopen(filename, "a");
    if (!f) return;
    fprintf(f, "%ld,%d,%.9f,%.9f,%.9f,%.9f,%lld,%.6f,%.6f,%lu,%s,%.9f\n",
            N, threads,
            s.real_time, s.user_time, s.system_time, s.total_cpu_time,
            s.total_operations, s.gops, s.elements_per_second,
            s.memory_used, algorithm, s.pi_est);
    fclose(f);
}

/* ==========================================
 * Cálculo de PI por método de Buffon
 * ========================================== */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long N = atol(argv[1]);
    int threads = atoi(argv[2]);
    const char* algorithm = "openmp_needles";

    double needle_len = 1.0, dist = 2.0;

    createDirectoryIfNotExists(DATA_DIR);
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/OpenMP_Results.csv", DATA_DIR);
    writeCSVHeaderIfNotExists(filename);

    PerformanceStats stats = {0};
    struct rusage start_usage, end_usage;
    struct timespec start_time, end_time;

    long total_hits = 0;

    getrusage(RUSAGE_SELF, &start_usage);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    #pragma omp parallel for reduction(+:total_hits) num_threads(threads)
    for (long i = 0; i < N; i++) {
        unsigned int seed = (unsigned int)(time(NULL) ^ (omp_get_thread_num() * 7919) ^ i);
        double y = ((double)rand_r(&seed) / RAND_MAX) * (dist / 2.0);
        double theta = ((double)rand_r(&seed) / RAND_MAX) * (M_PI / 2.0);
        if (y <= (needle_len / 2.0) * sin(theta)) total_hits++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    getrusage(RUSAGE_SELF, &end_usage);

    stats.real_time = timespec_to_seconds(end_time) - timespec_to_seconds(start_time);
    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);
    stats.system_time = timeval_to_seconds(end_usage.ru_stime) - timeval_to_seconds(start_usage.ru_stime);
    stats.total_cpu_time = stats.user_time + stats.system_time;

    stats.total_operations = N;
    stats.gops = (stats.total_operations / stats.real_time) / 1e9;
    stats.elements_per_second = (double)N / stats.real_time / 1e6;
    stats.memory_used = end_usage.ru_maxrss / 1024;
    stats.pi_est = (total_hits > 0) ? (2.0 * needle_len * N) / (dist * total_hits) : 0.0;

    appendResults(filename, N, threads, stats, algorithm);

    printf("===== RESULTADOS NEEDLES =====\n");
    printf("N: %ld | Hilos: %d\n", N, threads);
    printf("PI estimado: %.9f\n", stats.pi_est);
    printf("Tiempo real: %.9f s\n", stats.real_time);
    printf("Tiempo usuario: %.9f s | sistema: %.9f s\n", stats.user_time, stats.system_time);
    printf("Total operaciones: %lld | GOPS: %.6f\n", stats.total_operations, stats.gops);
    printf("Memoria usada: %lu MB\n", stats.memory_used);
    printf("Guardado en: %s\n", filename);

    return 0;
}


