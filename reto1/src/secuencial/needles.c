#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/secuencial/Secuencial_Needles_Data"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    double user_time;
    double pi_est;
} PerformanceStats;

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

double buffonNeedle(long N) {
    double L = 1.0; // longitud fija
    double d = 1.0; // distancia fija
    long count = 0;

    const double halfL = L / 2.0;

    for (long i = 0; i < N; i++) {
        double x = ((double)rand() / RAND_MAX) * d;
        double theta = ((double)rand() / RAND_MAX) * M_PI;
        double s = sin(theta);
        double x_left  = x - halfL * s;
        double x_right = x + halfL * s;

        if (x_left < 0.0 || x_right > d) {
            count++;
        }
    }

    if (count == 0) return 0.0;
    return (2.0 * L * N) / (d * count);
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
    sprintf(filename, "%s/Needles_Results.csv", dirPath);
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
        fprintf(stderr, "Uso: %s <num_agujas>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long N = atol(argv[1]);
    srand(time(NULL));

    createDirectoryIfNotExists(DATA_DIR);
    char* csvFilename = generateFilename(DATA_DIR);
    writeCSVHeaderIfNotExists(csvFilename);

    PerformanceStats stats = {0};
    printf("Iniciando simulación de Buffon's Needle...\n");
    struct rusage start_usage, end_usage;
    getrusage(RUSAGE_SELF, &start_usage);

    stats.pi_est = buffonNeedle(N);

    getrusage(RUSAGE_SELF, &end_usage);
    stats.user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);

    writeResultsToCSV(csvFilename, N, stats);

    printf("PI aproximado (Buffon's Needle): %.9f\n", stats.pi_est);
    printf("Tiempo de usuario: %.9f segundos\n", stats.user_time);

    free(csvFilename);
    return EXIT_SUCCESS;
}