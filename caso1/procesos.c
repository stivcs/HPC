#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#define DATA_DIR "Procesos_Data"

// Nota: ahora guardamos solo real_time (tiempo de reloj)
typedef struct {
    double real_time;   // tiempo real (wall-clock)
} PerformanceStats;

// Estructura para pasar datos a cada proceso
typedef struct {
    int process_id;
    int num_processes;
    int size;
    int block_size;
    int* A_flat;
    int* B_flat;
    int* C_flat;
} ProcessData;

#define get_element(matrix, row, col, size) ((matrix)[(row) * (size) + (col)])
#define add_to_element(matrix, row, col, size, value) ((matrix)[(row) * (size) + (col)] += (value))

// Crear directorio si no existe
int createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, 0755) == -1) {
            fprintf(stderr, "Error: No se pudo crear %s: %s\n", dirPath, strerror(errno));
            return 0;
        }
    }
    return 1;
}

// Crear matrices compartidas
int create_shared_matrix(int size, int** matrix_ptr) {
    size_t matrix_size = size * size * sizeof(int);
    int shmid = shmget(IPC_PRIVATE, matrix_size, IPC_CREAT | 0666);
    if (shmid < 0) { perror("shmget"); exit(EXIT_FAILURE); }
    *matrix_ptr = (int*)shmat(shmid, NULL, 0);
    if (*matrix_ptr == (int*)-1) { perror("shmat"); exit(EXIT_FAILURE); }
    return shmid;
}

// Llenar con valores aleatorios
void fillMatrix(int* matrix, int size) {
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            get_element(matrix, i, j, size) = rand() % 100 + 1;
}

// Inicializar en ceros
void initResultMatrix(int* matrix, int size) {
    memset(matrix, 0, size * size * sizeof(int));
}

// Multiplicación por proceso (bloques, versión que tenías)
void multiplyMatricesProcessOptimized(ProcessData* data) {
    int* A = data->A_flat;
    int* B = data->B_flat;
    int* C = data->C_flat;
    int size = data->size;
    int process_id = data->process_id;
    int num_processes = data->num_processes;
    int block_size = data->block_size;

    for (int i0 = 0; i0 < size; i0 += block_size) {
        int imax = (i0 + block_size < size) ? i0 + block_size : size;
        for (int j0 = process_id * block_size; j0 < size; j0 += num_processes * block_size) {
            int jmax = (j0 + block_size < size) ? j0 + block_size : size;
            for (int k0 = 0; k0 < size; k0 += block_size) {
                int kmax = (k0 + block_size < size) ? k0 + block_size : size;
                for (int i = i0; i < imax; i++) {
                    for (int k = k0; k < kmax; k++) {
                        int aik = get_element(A, i, k, size);
                        for (int j = j0; j < jmax; j++) {
                            add_to_element(C, i, j, size, aik * get_element(B, k, j, size));
                        }
                    }
                }
            }
        }
    }
}

// Multiplicar matrices con procesos (fork)
void multiplyMatricesWithProcesses(int* A, int* B, int* C, int size, int num_processes) {
    pid_t* pids = (pid_t*)malloc(num_processes * sizeof(pid_t));
    if (!pids) { fprintf(stderr, "Error malloc\n"); exit(EXIT_FAILURE); }

    int block_size = (size > 2000) ? 128 : (size > 1000) ? 64 : 32;

    ProcessData data;
    data.size = size;
    data.num_processes = num_processes;
    data.A_flat = A;
    data.B_flat = B;
    data.C_flat = C;
    data.block_size = block_size;

    for (int i = 0; i < num_processes; i++) {
        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); exit(EXIT_FAILURE); }
        else if (pids[i] == 0) {
            data.process_id = i;
            multiplyMatricesProcessOptimized(&data);
            _exit(EXIT_SUCCESS); // salir sin ejecutar código de limpieza del padre
        }
    }
    for (int i = 0; i < num_processes; i++) waitpid(pids[i], NULL, 0);
    free(pids);
}

// CPUs disponibles
int getNumCPUs() {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
}

// Convertir timespec a segundos (double)
double timespec_to_seconds(struct timespec ts) {
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// Escribir cabecera en CSV si está vacío (ahora real_time)
void writeCSVHeaderIfNeeded(const char* filename) {
    FILE* f = fopen(filename, "a+");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) fprintf(f, "tamañoMatriz,#procesos,real_time\n");
    fclose(f);
}

// Guardar resultados (real_time)
void appendResults(const char* filename, int size, int num_processes, double real_time) {
    FILE* f = fopen(filename, "a");
    if (!f) return;
    fprintf(f, "%d,%d,%.9f\n", size, num_processes, real_time);
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> [num_procesos]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int size = atoi(argv[1]);
    int num_processes = (argc == 3) ? atoi(argv[2]) : getNumCPUs();
    if (size <= 0 || num_processes <= 0) return EXIT_FAILURE;

    srand(time(NULL));

    if (!createDirectoryIfNotExists(DATA_DIR)) return EXIT_FAILURE;

    // Archivo dinámico según procesos: Procesos_Data/tiempos_Xprocesos.csv
    char filename[256];
    snprintf(filename, sizeof(filename), DATA_DIR "/tiempos_%dprocesos.csv", num_processes);

    writeCSVHeaderIfNeeded(filename);

    printf("Creando matrices de %dx%d...\n", size, size);
    int *A, *B, *C;
    int shmid_A = create_shared_matrix(size, &A);
    int shmid_B = create_shared_matrix(size, &B);
    int shmid_C = create_shared_matrix(size, &C);

    fillMatrix(A, size);
    fillMatrix(B, size);
    initResultMatrix(C, size);

    printf("Matrices creadas. Iniciando multiplicación con %d procesos...\n", num_processes);

    // MEDICIÓN DE TIEMPO REAL: usar CLOCK_MONOTONIC
    struct timespec start_ts, end_ts;
    if (clock_gettime(CLOCK_MONOTONIC, &start_ts) != 0) {
        perror("clock_gettime inicio");
        // aun así continuamos, pero la medicion será inválida
    }

    multiplyMatricesWithProcesses(A, B, C, size, num_processes);

    if (clock_gettime(CLOCK_MONOTONIC, &end_ts) != 0) {
        perror("clock_gettime fin");
    }

    PerformanceStats stats;
    stats.real_time = timespec_to_seconds(end_ts) - timespec_to_seconds(start_ts);

    appendResults(filename, size, num_processes, stats.real_time);

    if (size <= 500) {
        printf("Validando resultados...\n");
        int correct = 1;
        for (int i = 0; i < size && correct; i++) {
            for (int j = 0; j < size && correct; j++) {
                int expected = 0;
                for (int k = 0; k < size; k++) expected += get_element(A, i, k, size) * get_element(B, k, j, size);
                if (expected != get_element(C, i, j, size)) {
                    printf("Error en [%d][%d]\n", i, j);
                    correct = 0;
                }
            }
        }
        if (correct) printf("Validación correcta\n");
    }

    printf("\n===== Resultados =====\n");
    printf("Matriz %d x %d con %d procesos\n", size, size, num_processes);
    printf("Real time: %.9f s\n", stats.real_time);
    printf("Guardado en: %s\n", filename);

    // Desconectar y eliminar memoria compartida
    shmdt(A); shmdt(B); shmdt(C);
    shmctl(shmid_A, IPC_RMID, NULL);
    shmctl(shmid_B, IPC_RMID, NULL);
    shmctl(shmid_C, IPC_RMID, NULL);

    return EXIT_SUCCESS;
}

