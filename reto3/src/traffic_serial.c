#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + tv.tv_usec / 1e6;
}

int* create_road(int N, double density) {
    int* road = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) {
        double r = rand() / (double)RAND_MAX;
        road[i] = (r < density) ? 1 : 0;
    }
    return road;
}

int main(int argc, char** argv) {

    if (argc < 6) {
        fprintf(stderr, "Uso: %s N steps density print_freq output.csv\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int steps = atoi(argv[2]);
    double density = atof(argv[3]);
    int print_freq = atoi(argv[4]);
    char* out_csv = argv[5];

    system("mkdir -p results");

    int* road = create_road(N, density);
    int* next = calloc(N, sizeof(int));

    long long total_cars = 0;
    for (int i = 0; i < N; i++)
        total_cars += road[i];

    struct rusage start_u, end_u;
    getrusage(RUSAGE_SELF, &start_u);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int t = 0; t < steps; t++) {

        memset(next, 0, N * sizeof(int));

        for (int i = 0; i < N; i++) {
            int right = (i + 1) % N;
            int move = (road[i] == 1 && road[right] == 0);

            next[i] |= road[i] & !move;
            next[right] |= move;
        }

        int* tmp = road;
        road = next;
        next = tmp;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    getrusage(RUSAGE_SELF, &end_u);

    double real_time =
        (t1.tv_sec - t0.tv_sec) +
        (t1.tv_nsec - t0.tv_nsec) / 1e9;

    double user_cpu =
        timeval_to_seconds(end_u.ru_utime) -
        timeval_to_seconds(start_u.ru_utime);

    double sys_cpu =
        timeval_to_seconds(end_u.ru_stime) -
        timeval_to_seconds(start_u.ru_stime);

    size_t mem_kb = end_u.ru_maxrss;

    FILE* f = fopen(out_csv, "w");
    fprintf(f,
        "N,real_time,user_cpu,sys_cpu,memory_kb\n"
        "%d,%f,%f,%f,%zu\n",
        N, real_time, user_cpu, sys_cpu, mem_kb
    );
    fclose(f);

    free(road);
    free(next);

    return 0;
}
