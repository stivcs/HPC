// traffic_serial.c
// Autómata celular 1D (tráfico) versión secuencial.
// Periodicidad: celda 0 <- celda N-1, celda N+1 <- celda 1
// CSV: step, avg_velocity, moved, cars, wall_time

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static inline double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    if (argc < 6) {
        fprintf(stderr, "Uso: %s N steps density print_freq out.csv\n", argv[0]);
        return EXIT_FAILURE;
    }

    long N = atol(argv[1]);
    int steps = atoi(argv[2]);
    double density = atof(argv[3]);
    int print_freq = atoi(argv[4]);
    const char* out_csv = argv[5];

    long alloc = N + 2;
    unsigned char *old = malloc(alloc);
    unsigned char *new = malloc(alloc);
    if (!old || !new) { perror("malloc"); return EXIT_FAILURE; }

    srand(time(NULL));
    for (long i = 1; i <= N; ++i)
        old[i] = ((double)rand() / RAND_MAX) < density ? 1 : 0;

    old[0] = old[N+1] = 0;
    memset(new, 0, alloc);

    FILE *f = fopen(out_csv, "w");
    if (!f) { perror("fopen"); return EXIT_FAILURE; }
    fprintf(f, "step,avg_velocity,moved,cars,wall_time\n");

    double t0 = now_seconds();

    for (int t = 0; t < steps; t++) {
        // periodic boundaries
        old[0] = old[N];
        old[N+1] = old[1];

        long moved = 0, cars = 0;
        memset(new+1, 0, N);

        for (long i = 1; i <= N; ++i) {
            if (old[i]) {
                cars++;
                if (old[i+1] == 0) {
                    new[i+1] = 1;
                    moved++;
                } else {
                    new[i] = 1;
                }
            }
        }

        double wall = now_seconds() - t0;
        double avg_vel = cars ? (double)moved / cars : 0.0;

        if (print_freq > 0 && (t % print_freq == 0))
            printf("step %d avg_vel %.6f moved %ld cars %ld\n",
                   t, avg_vel, moved, cars);

        fprintf(f, "%d,%.6f,%ld,%ld,%.6f\n",
                t, avg_vel, moved, cars, wall);

        unsigned char *tmp = old; old = new; new = tmp;
    }

    printf("Serial finalizado\n");

    fclose(f);
    free(old);
    free(new);
    return 0;
}
