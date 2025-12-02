#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <sys/resource.h>

double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + tv.tv_usec / 1e6;
}

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
    char* out_csv = argv[5];

    if (rank == 0) {
        system("mkdir -p results");
    }
    MPI_Barrier(MPI_COMM_WORLD);

    int local_N = N / size;
    if (rank == size - 1) local_N += N % size;

    int* road = calloc(local_N + 2, sizeof(int));
    int* next = calloc(local_N + 2, sizeof(int));

    srand(rank + 1);

    for (int i = 1; i <= local_N; i++) {
        double r = rand() / (double)RAND_MAX;
        road[i] = (r < density) ? 1 : 0;
    }

    long long local_cars = 0;
    for (int i = 1; i <= local_N; i++) local_cars += road[i];

    long long total_cars = 0;
    MPI_Reduce(&local_cars, &total_cars, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    struct rusage start_u, end_u;
    if (rank == 0) getrusage(RUSAGE_SELF, &start_u);

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    double comm_time = 0;

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

        comm_time += MPI_Wtime() - comm_s;

        memset(next, 0, (local_N + 2) * sizeof(int));

        for (int i = 1; i <= local_N; i++) {
            int right = i + 1;
            if (right > local_N) right = local_N + 1;

            int move = (road[i] == 1 && road[right] == 0);

            next[i] |= road[i] & !move;
            next[right] |= move;
        }

        int* tmp = road;
        road = next;
        next = tmp;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    if (rank == 0) {
        getrusage(RUSAGE_SELF, &end_u);

        double mpi_time = t1 - t0;

        double user_cpu =
            timeval_to_seconds(end_u.ru_utime) -
            timeval_to_seconds(start_u.ru_utime);

        double sys_cpu =
            timeval_to_seconds(end_u.ru_stime) -
            timeval_to_seconds(start_u.ru_stime);

        size_t mem_kb = end_u.ru_maxrss;

        FILE* f = fopen(out_csv, "w");
        fprintf(f,
            "N,mpi_time,user_cpu,sys_cpu,memory_kb,comm_time\n"
            "%d,%f,%f,%f,%zu,%f\n",
            N, mpi_time, user_cpu, sys_cpu, mem_kb, comm_time
        );
        fclose(f);
    }

    free(road);
    free(next);

    MPI_Finalize();
    return 0;
}
