#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#ifndef RESULTS_DIR
#define RESULTS_DIR "results"
#endif

#define DATA_DIR RESULTS_DIR "/procesos/Procesos_Dartboard_Data"

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
    if (ftell(f) == 0) fprintf(f, "N,num_procesos,pi_est,real_time\n");
    fclose(f);
}

// Guardar resultados
void appendResults(const char* filename, long N, int num_procs, PerformanceStats stats) {
    FILE* f = fopen(filename, "a");
    if (!f) return;
    fprintf(f, "%ld,%d,%.9f,%.9f\n", N, num_procs, stats.pi_est, stats.real_time);
    fclose(f);
}

// Cada proceso ejecuta su parte
void dartboardProcess(long chunk, int* shm_hits, int proc_id) {
    long local_hits = 0;
    unsigned int seed = time(NULL) ^ (proc_id * 7919);

    for (long i = 0; i < chunk; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1.0) local_hits++;
    }
    shm_hits[proc_id] = local_hits;
    _exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N> <num_procesos>\n", argv[0]);
        return 1;
    }

    long N = atol(argv[1]);
    int num_procs = atoi(argv[2]);

    if (!createDirectoryIfNotExists(DATA_DIR)) return EXIT_FAILURE;

    char filename[256];
    snprintf(filename, sizeof(filename), DATA_DIR "/results_%dprocesos.csv", num_procs);
    writeCSVHeaderIfNeeded(filename);

    // Memoria compartida para acumular resultados
    int shmid = shmget(IPC_PRIVATE, num_procs * sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) { perror("shmget"); exit(1); }
    int* shm_hits = (int*)shmat(shmid, NULL, 0);
    if (shm_hits == (int*)-1) { perror("shmat"); exit(1); }

    memset(shm_hits, 0, num_procs * sizeof(int));

    long chunk = N / num_procs;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Lanzar procesos
    for (int p = 0; p < num_procs; p++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            dartboardProcess(chunk, shm_hits, p);
        }
    }

    // Esperar procesos
    for (int p = 0; p < num_procs; p++) wait(NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    long total_hits = 0;
    for (int p = 0; p < num_procs; p++) total_hits += shm_hits[p];

    PerformanceStats stats;
    stats.pi_est = (4.0 * total_hits) / (chunk * num_procs);
    stats.real_time = timespec_to_seconds(end) - timespec_to_seconds(start);

    appendResults(filename, N, num_procs, stats);

    printf("PI Dartboard con %d procesos: %.9f\n", num_procs, stats.pi_est);
    printf("Tiempo real: %.9f s\n", stats.real_time);
    printf("Guardado en: %s\n", filename);

    // Liberar memoria compartida
    shmdt(shm_hits);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}