#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/secuencial/Secuencial_Dartboard_Data"

typedef struct {
    double user_time;
    double pi_est;
} PerformanceStats;

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// RNG rápido tipo xorshift
static unsigned int rng_seed;
static inline double fast_rand() {
    rng_seed ^= rng_seed << 13;
    rng_seed ^= rng_seed >> 17;
    rng_seed ^= rng_seed << 5;
    return (double)(rng_seed & 0x7FFFFFFF) / 0x7FFFFFFF;
}

// Función optimizada de Dartboard
double dartboard(long N) {
    long count = 0;
    for (long i = 0; i < N; i++) {
        double x = fast_rand();
        double y = fast_rand();
        if (x*x + y*y <= 1.0) count++;
    }
    return (4.0 * count) / N;
}

void createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0700) != 0) {
            fprintf(stderr, "Error: no se pudo crear el directorio %s\n", dirPath);
            exit(EXIT_FAILURE);
        }
    }
}

char* generateFilename(const char* dirPath) {
    char* filename = (char*)malloc(256);
    sprintf(filename, "%s/Dartboard_Results.csv", dirPath);
    return filename;
}

void writeCSVHeaderIfNotExists(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        FILE* file = fopen(filename, "w");
        fprintf(file, "N,pi_est,user_time\n");
        fclose(file);
    }
}

void writeResultsToCSV(const char* filename, long N, PerformanceStats stats) {
    FILE* file = fopen(filename, "a");
    fprintf(file, "%ld,%.9f,%.9f\n", N, stats.pi_est, stats.user_time);
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <num_puntos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long N = atol(argv[1]);
    rng_seed = (unsigned int)time(NULL);

    createDirectoryIfNotExists(DATA_DIR);
    char* csvFilename = generateFilename(DATA_DIR);
    writeCSVHeaderIfNotExists(csvFilename);

    PerformanceStats stats = {0};
    printf("Iniciando simulación de Dartboard...\n");
    struct rusage start_usage, end_usage;
    getrusage(RUSAGE_SELF, &start_usage);

    stats.pi_est = dartboard(N);

    getrusage(RUSAGE_SELF, &end_usage);
    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);

    writeResultsToCSV(csvFilename, N, stats);

    printf("PI aproximado (Dartboard): %.9f\n", stats.pi_est);
    printf("Tiempo de usuario: %.9f segundos\n", stats.user_time);

    free(csvFilename);
    return EXIT_SUCCESS;
}
