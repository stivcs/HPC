// traffic_mpi.c
// Autómata celular 1D distribuido con MPI.
// Partición por bloques + ghost cells + periodicidad.

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 6) {
        if (rank==0)
            fprintf(stderr, "Uso: %s N steps density print_freq out.csv\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    long N = atol(argv[1]);
    int steps = atoi(argv[2]);
    double density = atof(argv[3]);
    int print_freq = atoi(argv[4]);
    const char* out_csv = argv[5];

    long base = N / size;
    int rem = N % size;
    long local_n = base + (rank < rem ? 1 : 0);

    long offset = rank * base + (rank < rem ? rank : rem);

    long alloc = local_n + 2;
    unsigned char *old = malloc(alloc);
    unsigned char *new = malloc(alloc);
    if (!old || !new) {
        perror("malloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    srand(time(NULL) + rank * 777);
    for (long i = 1; i <= local_n; ++i)
        old[i] = ((double)rand() / RAND_MAX) < density ? 1 : 0;

    old[0] = old[local_n+1] = 0;
    memset(new, 0, alloc);

    int left = (rank - 1 + size) % size;
    int right = (rank + 1) % size;

    FILE *f = NULL;
    if (rank == 0) {
        f = fopen(out_csv, "w");
        if (!f) { perror("fopen"); MPI_Abort(MPI_COMM_WORLD, 1); }
        fprintf(f, "step,avg_velocity,global_moved,global_cars,wall_time\n");
    }

    double t_start = MPI_Wtime();

    for (int t = 0; t < steps; t++) {
        unsigned char send_left = old[1];
        unsigned char send_right = old[local_n];
        unsigned char recv_left, recv_right;

        MPI_Sendrecv(&send_left, 1, MPI_UNSIGNED_CHAR, left, 0,
                     &recv_right,  1, MPI_UNSIGNED_CHAR, right, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(&send_right, 1, MPI_UNSIGNED_CHAR, right, 1,
                     &recv_left,   1, MPI_UNSIGNED_CHAR, left, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        old[0] = recv_left;
        old[local_n+1] = recv_right;

        long local_moved = 0, local_cars = 0;
        memset(new+1, 0, local_n);

        for (long i = 1; i <= local_n; i++) {
            if (old[i]) {
                local_cars++;
                if (old[i+1] == 0) {
                    new[i+1] = 1;
                    local_moved++;
                } else {
                    new[i] = 1;
                }
            }
        }

        long global_moved = 0, global_cars = 0;
        MPI_Allreduce(&local_moved, &global_moved, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&local_cars,  &global_cars,  1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);

        double wall = MPI_Wtime() - t_start;
        double avg_vel = global_cars ? (double)global_moved / global_cars : 0.0;

        if (rank == 0) {
            if (print_freq > 0 && (t % print_freq == 0))
                printf("step %d avg_vel %.6f\n", t, avg_vel);

            fprintf(f, "%d,%.6f,%ld,%ld,%.6f\n",
                    t, avg_vel, global_moved, global_cars, wall);
        }

        unsigned char *tmp = old; old = new; new = tmp;
    }

    if (rank == 0) {
        printf("MPI finalizado.\n");
        fclose(f);
    }

    free(old);
    free(new);

    MPI_Finalize();
    return 0;
}
