#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>

// Definir M_PI si no está definido
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Convierte timeval a segundos
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// Simulación Buffon's Needle
double buffonNeedle(long N) {
    double L = 1.0;    // longitud de la aguja fija
    double d = 1.0;    // distancia entre líneas fija
    long count = 0;

    for (long i = 0; i < N; i++) {
        double x = ((double)rand() / RAND_MAX) * d;        // posición del centro
        double theta = ((double)rand() / RAND_MAX) * M_PI; // ángulo con la vertical

        double x_left  = x - (L / 2.0) * sin(theta);
        double x_right = x + (L / 2.0) * sin(theta);

        if (x_left < 0.0 || x_right > d) {
            count++;
        }
    }

    if (count == 0) return 0.0;
    return (2.0 * L * N) / (d * count); // estimación de pi
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <num_agujas>\n", argv[0]);
        return 1;
    }

    long N = atol(argv[1]);
    srand(time(NULL));

    struct rusage start_usage, end_usage;
    getrusage(RUSAGE_SELF, &start_usage);

    double pi_est = buffonNeedle(N);

    getrusage(RUSAGE_SELF, &end_usage);
    double user_time = timeval_to_seconds(end_usage.ru_utime) - timeval_to_seconds(start_usage.ru_utime);

    printf("PI aproximado (Buffon's Needle): %f\n", pi_est);
    printf("Tiempo de usuario: %.9f segundos\n", user_time);

    return 0;
}