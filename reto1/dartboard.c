#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

// Convierte timeval a segundos
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// Simulación Dartboard
double dartboard(long N) {
    long count = 0;

    for (long i = 0; i < N; i++) {
        double x = (double)rand() / RAND_MAX;
        double y = (double)rand() / RAND_MAX;

        if (x * x + y * y <= 1.0) {
            count++;
        }
    }
    return (4.0 * count) / N;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <num_puntos>\n", argv[0]);
        return 1;
    }

    long N = atol(argv[1]);
    srand(time(NULL));

    struct rusage start_usage, end_usage;
    getrusage(RUSAGE_SELF, &start_usage);

    double pi_est = dartboard(N);

    getrusage(RUSAGE_SELF, &end_usage);
    double user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);

    printf("PI aproximado (Dartboard): %f\n", pi_est);
    printf("Tiempo de usuario: %.9f segundos\n", user_time);

    return 0;
}