#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>

/* ======================================================
 * ESTRUCTURA DE MÉTRICAS
 * ====================================================== */
typedef struct {
    double real_time;
    double user_time;
    double system_time;
    double total_cpu_time;
    long long total_cells;
    size_t memory_used_kb;
} PerformanceStats;


/* ======================================================
 * FUNCIÓN: TIEMPO A SEGUNDOS
 * ====================================================== */
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}


/* ======================================================
 * GENERAR CARRETERA INICIAL
 * ====================================================== */
int* create_road(int N, double density) {
    int* road = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) {
        double r = rand() / (double)RAND_MAX;
        road[i] = (r < density) ? 1 : 0;
    }
    return road;
}


/* ======================================================
 * AUTÓMATA CELULAR (REGLA 184)
 * ====================================================== */
void update_road(int* road, int* next, int N) {
    for (int i = 0; i < N; i++) {
        int right = (i + 1) % N;
        int move = (road[i] == 1 && road[right] == 0);

        next[i] = road[i] & !move;
        next[right] |= move;
    }
}


/* ======================================================
 * PROGRAMA PRINCIPAL
 * ====================================================== */
int main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Uso: %s N steps density print_freq output.csv\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int steps = atoi(argv[2]);
    double density = atof(argv[3]);
    int print_freq = atoi(argv[4]);
    char* csv_path = argv[5];

    srand(time(NULL));

    int* road = create_road(N, density);
    int* next = calloc(N, sizeof(int));

    FILE* f = fopen(csv_path, "w");
    fprintf(f, "step,avg_velocity,moved,total_cars\n");

    long long total_cars = 0;
    for (int i = 0; i < N; i++) total_cars += road[i];

    /* ============================ */
    /*   PROFILING: inicio CPU     */
    /* ============================ */
    struct rusage start_usage, end_usage;
    struct timespec start_real, end_real;

    getrusage(RUSAGE_SELF, &start_usage);
    clock_gettime(CLOCK_MONOTONIC, &start_real);

    /* ============================ */
    /*   SIMULACIÓN                 */
    /* ============================ */
    for (int t = 0; t < steps; t++) {
        memset(next, 0, N * sizeof(int));
        int moved = 0;

        for (int i = 0; i < N; i++) {
            int right = (i + 1) % N;
            int move = (road[i] == 1 && road[right] == 0);

            if (move) moved++;

            next[i] |= road[i] & !move;
            next[right] |= move;
        }

        int* tmp = road;
        road = next;
        next = tmp;

        if (t % print_freq == 0) {
            double avg_vel = (double)moved / total_cars;
            fprintf(f, "%d,%f,%d,%lld\n", t, avg_vel, moved, total_cars);
        }
    }

    /* ============================ */
    /*   PROFILING: fin CPU        */
    /* ============================ */
    clock_gettime(CLOCK_MONOTONIC, &end_real);
    getrusage(RUSAGE_SELF, &end_usage);

    PerformanceStats stats;

    stats.real_time =
        (end_real.tv_sec - start_real.tv_sec) +
        (end_real.tv_nsec - start_real.tv_nsec) / 1e9;

    stats.user_time =
        timeval_to_seconds(end_usage.ru_utime) -
        timeval_to_seconds(start_usage.ru_utime);

    stats.system_time =
        timeval_to_seconds(end_usage.ru_stime) -
        timeval_to_seconds(start_usage.ru_stime);

    stats.total_cpu_time = stats.user_time + stats.system_time;
    stats.memory_used_kb = end_usage.ru_maxrss;

    fclose(f);

    free(road);
    free(next);

    return 0;
}
