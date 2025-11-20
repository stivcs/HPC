#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

/* ======================================================
 * FUNCIONES AUXILIARES DE MATRICES
 * ====================================================== */
int** createMatrix(int n) {
    int** mat = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        mat[i] = malloc(n * sizeof(int));
        for (int j = 0; j < n; j++)
            mat[i][j] = rand() % 100 + 1;
    }
    return mat;
}

int* flattenMatrix(int** mat, int n) {
    int* flat = malloc(n * n * sizeof(int));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            flat[i*n + j] = mat[i][j];
    return flat;
}

int** unflattenMatrix(int* flat, int n) {
    int** mat = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        mat[i] = malloc(n * sizeof(int));
        for (int j = 0; j < n; j++)
            mat[i][j] = flat[i*n + j];
    }
    return mat;
}

void freeMatrix(int** mat, int n) {
    for (int i = 0; i < n; i++) free(mat[i]);
    free(mat);
}

/* ======================================================
 * PROGRAMA PRINCIPAL MPI
 * ====================================================== */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <tamaño_matriz>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n <= 0) return EXIT_FAILURE;

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows_per_proc = n / size;
    int remainder = n % size;
    int start_row = rank * rows_per_proc + (rank < remainder ? rank : remainder);
    int local_rows = rows_per_proc + (rank < remainder ? 1 : 0);

    int* local_A = malloc(local_rows * n * sizeof(int));
    int* B = malloc(n * n * sizeof(int));
    int* local_C = calloc(local_rows * n, sizeof(int));
    int* A_flat = NULL;
    int* C_flat = NULL;

    if (rank == 0) {
        srand(time(NULL));
        int** A = createMatrix(n);
        int** B_mat = createMatrix(n);

        A_flat = flattenMatrix(A, n);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                B[i*n + j] = B_mat[i][j];

        freeMatrix(A, n);
        freeMatrix(B_mat, n);
    }

    // Preparar sendcounts y desplazamientos para Scatterv
    int* sendcounts = malloc(size * sizeof(int));
    int* displs = malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        int r = rows_per_proc + (i < remainder ? 1 : 0);
        sendcounts[i] = r * n;
        displs[i] = (i * rows_per_proc + (i < remainder ? i : remainder)) * n;
    }

    // Distribuir filas de A y enviar B a todos
    MPI_Scatterv(A_flat, sendcounts, displs, MPI_INT,
                 local_A, local_rows*n, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(B, n*n, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    // Multiplicación local
    for (int i = 0; i < local_rows; i++)
        for (int k = 0; k < n; k++)
            for (int j = 0; j < n; j++)
                local_C[i*n + j] += local_A[i*n + k] * B[k*n + j];

    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();

    // Recolectar resultados en C_flat en el proceso 0
    if (rank == 0) C_flat = malloc(n * n * sizeof(int));

    MPI_Gatherv(local_C, local_rows*n, MPI_INT,
                C_flat, sendcounts, displs, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\n===== RESULTADOS MPI =====\n");
        printf("Tamaño de la matriz: %d x %d\n", n, n);
        printf("Tiempo real MPI: %.6f s\n", end_time - start_time);

        // Opcional: puedes reconstruir la matriz completa
        // int** C = unflattenMatrix(C_flat, n);
        // freeMatrix(C, n);
        free(C_flat);
        free(A_flat);
    }

    free(local_A);
    free(local_C);
    free(B);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return EXIT_SUCCESS;
}

