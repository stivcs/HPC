#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/hilos/Hilos_Dartboard_Data"

typedef struct {
    int thread_id;
    int num_threads;
    long N;
    long local_hits;
} ThreadData;

typedef struct {
    double real_time;
    double pi_est;
} PerformanceStats;

static inline double timeval_to_seconds(struct timeval tv) {
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

// Función que ejecuta cada hilo
void* dartboardThread(void* arg) {
    ThreadData* d = (ThreadData*)arg;
    long hits = 0;

    long chunk = d->N / d->num_threads;
    unsigned int seed = time(NULL) ^ (d->thread_id * 7919);

    for (long i = 0; i < chunk; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1.0) hits++;
    }
    d->local_hits = hits;
    return NULL;
}

// Crear directorio si no existe
void ensureDir(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0755) == -1) {
            fprintf(stderr, "Error creando directorio %s: %s\n",
                    dirPath, strerror(errno));
            exit(1);
        }
    }
}

// Asegurar cabecera del CSV
void ensureCSVHeader(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        f = fopen(filename, "w");
        if (!f) { perror("fopen"); exit(1); }
        fprintf(f, "N,num_threads,pi_est,real_time\n");
    }
    if (f) fclose(f);
}

// Guardar resultados en CSV
void appendResult(const char* filename, long N, int num_threads, PerformanceStats stats) {
    FILE* f = fopen(filename, "a");
    if (!f) { perror("fopen"); exit(1); }
    fprintf(f, "%ld,%d,%.9f,%.9f\n", N, num_threads, stats.pi_est, stats.real_time);
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N> <num_hilos>\n", argv[0]);
        return 1;
    }

    long N = atol(argv[1]);
    int num_threads = atoi(argv[2]);

    ensureDir(DATA_DIR);
    char filename[256];
    snprintf(filename, sizeof(filename), DATA_DIR "/results_%dhilos.csv", num_threads);
    ensureCSVHeader(filename);

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData* data = malloc(num_threads * sizeof(ThreadData));

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int t = 0; t < num_threads; t++) {
        data[t].thread_id = t;
        data[t].num_threads = num_threads;
        data[t].N = N;
        data[t].local_hits = 0;
        pthread_create(&threads[t], NULL, dartboardThread, &data[t]);
    }
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);

    gettimeofday(&end, NULL);

    long total_hits = 0;
    for (int t = 0; t < num_threads; t++) total_hits += data[t].local_hits;

    PerformanceStats stats;
    stats.pi_est = (4.0 * total_hits) / N;
    stats.real_time = timeval_to_seconds(end) - timeval_to_seconds(start);

    appendResult(filename, N, num_threads, stats);

    printf("PI Dartboard hilos=%d: %.9f\n", num_threads, stats.pi_est);
    printf("Tiempo real: %.9f s\n", stats.real_time);
    printf("Guardado en: %s\n", filename);

    free(threads);
    free(data);
    return 0;
}