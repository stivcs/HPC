#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <sys/resource.h>
#include <time.h>

/* ======================================== */
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + tv.tv_usec / 1e6;
}
/* ======================================== */

int main(int argc, char **argv) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 6) {
        if (rank == 0)
            fprintf(stderr, "Uso: %s N steps density print_freq output.csv\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int N = atoi(argv[1]);
    int steps = atoi(argv[2]);
    double density = atof(argv[3]);
    int print_freq = atoi(argv[4]);
    char* csv_path = argv[5];

    int local_N = N / size;
    if (rank == size - 1)
        local_N += N % size;

    int* road = calloc(local_N + 2, sizeof(int));  /* +2 ghost cells */
    int* next = calloc(local_N + 2, sizeof(int));

    srand(rank + 1);

    for (int i = 1; i <= local_N; i++) {
        double r = rand() / (double)RAND_MAX;
        road[i] = (r < density) ? 1 : 0;
    }

    long long local_cars = 0;
    for (int i = 1; i <= local_N; i++)
        local_cars += road[i];

    long long total_cars = 0;
    MPI_Reduce(&local_cars, &total_cars, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    /* ============================================== */
    /*      PROFILING: TIEMPO REAL GLOBAL MPI         */
    /* ============================================== */

    MPI_Barrier(MPI_COMM_WORLD);   // todos listos
    double mpi_start = MPI_Wtime(); // tiempo real correcto

    /* ============================================== */
    /*      PROFILING CPU/MEMORIA (solo rank 0)       */
    /* ============================================== */
    struct rusage start_u, end_u;

    if (rank == 0)
        getrusage(RUSAGE_SELF, &start_u);

    double total_comm_time = 0;

    /* ============================================== */
    /*  AUTÓMATA CELULAR MPI                           */
    /* ============================================== */
    for (int t = 0; t < steps; t++) {

        double comm_s = MPI_Wtime();

        int left_rank = (rank == 0) ? size - 1 : rank - 1;
        int right_rank = (rank + 1) % size;

        MPI_Sendrecv(&road[1], 1, MPI_INT, left_rank, 0,
                     &road[local_N + 1], 1, MPI_INT, right_rank, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(&road[local_N], 1, MPI_INT, right_rank, 1,
                     &road[0], 1, MPI_INT, left_rank, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        total_comm_time += MPI_Wtime() - comm_s;

        memset(next, 0, (local_N + 2) * sizeof(int));

        int moved_local = 0;

        for (int i = 1; i <= local_N; i++) {
            int right = i + 1;
            if (right > local_N) right = local_N + 1;

            int move = (road[i] == 1 && road[right] == 0);

            if (move) moved_local++;

            next[i] |= road[i] & !move;
            next[right] |= move;
        }

        int* tmp = road;
        road = next;
        next = tmp;

        int moved_global = 0;
        MPI_Reduce(&moved_local, &moved_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0 && t % print_freq == 0) {
            double avg_vel = (double)moved_global / total_cars;
            FILE* f = fopen(csv_path, (t == 0) ? "w" : "a");
            fprintf(f, "%d,%f,%d,%lld\n", t, avg_vel, moved_global, total_cars);
            fclose(f);
        }
    }

    /* ============================================== */
    /*  PROFILING: FIN TIEMPO GLOBAL                  */
    /* ============================================== */

    MPI_Barrier(MPI_COMM_WORLD);
    double mpi_end = MPI_Wtime();
    double mpi_real_time = mpi_end - mpi_start;

    /* ============================================== */
    /*  PROFILING: CPU + MEMORIA (solo rank 0)        */
    /* ============================================== */

    if (rank == 0) {
        getrusage(RUSAGE_SELF, &end_u);

        double user_cpu =
            timeval_to_seconds(end_u.ru_utime) -
            timeval_to_seconds(start_u.ru_utime);

        double sys_cpu =
            timeval_to_seconds(end_u.ru_stime) -
            timeval_to_seconds(start_u.ru_stime);

        size_t mem_kb = end_u.ru_maxrss;

        FILE* f = fopen("results/mpi_profiling.csv", "a");
        fprintf(f,
            "%d,%f,%f,%f,%zu,%f\n",
            N, mpi_real_time, user_cpu, sys_cpu, mem_kb, total_comm_time
        );
        fclose(f);
    }

    free(road);
    free(next);

    MPI_Finalize();
    return 0;
}
