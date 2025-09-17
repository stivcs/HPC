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

// Nombre del directorio para almacenamiento de datos
#define DATA_DIR "Procesos_Data"

// Archivo único donde se guardan las métricas
#define CSV_FILENAME DATA_DIR "/tiempos.csv"

// Solo guardamos el tiempo de usuario
typedef struct {
    double user_time;   // tiempo de CPU en modo usuario
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

// Multiplicación por proceso
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

// Multiplicar matrices con procesos
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
            exit(EXIT_SUCCESS);
        }
    }
    for (int i = 0; i < num_processes; i++) waitpid(pids[i], NULL, 0);
    free(pids);
}

// Validar resultados en matrices pequeñas
void validateResult(int* A, int* B, int* C, int size) {
    int correct = 1;
    int step = (size > 1000) ? size / 10 : 1;
    for (int i = 0; i < size && correct; i += step) {
        for (int j = 0; j < size && correct; j += step) {
            int expected = 0;
            for (int k = 0; k < size; k++) expected += get_element(A, i, k, size) * get_element(B, k, j, size);
            if (expected != get_element(C, i, j, size)) {
                printf("Error en [%d][%d]\n", i, j);
                correct = 0;
            }
        }
    }
    if (correct) printf("Validación correcta (muestra)\n");
}

// CPUs disponibles
int getNumCPUs() {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
}

// Convertir timeval a segundos
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + tv.tv_usec / 1e6;
}

// Escribir cabecera en CSV si está vacío
void writeCSVHeaderIfNeeded(const char* filename) {
    FILE* f = fopen(filename, "a+");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) fprintf(f, "tamañoMatriz,#procesos,user_time\n");
    fclose(f);
}

// Guardar resultados
void appendResults(const char* filename, int size, int num_processes, double user_time) {
    FILE* f = fopen(filename, "a");
    if (!f) return;
    fprintf(f, "%d,%d,%.9f\n", size, num_processes, user_time);
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
    printf("Creando matrices de %dx%d...\n", size, size);
    int *A, *B, *C;
    int shmid_A = create_shared_matrix(size, &A);
    int shmid_B = create_shared_matrix(size, &B);
    int shmid_C = create_shared_matrix(size, &C);

    fillMatrix(A, size);
    fillMatrix(B, size);
    initResultMatrix(C, size);
    printf("Matrices creadas. Iniciando multiplicación con %d procesos...\n", num_processes);
    struct rusage start, end;
    getrusage(RUSAGE_SELF, &start);
    getrusage(RUSAGE_CHILDREN, &start);

    multiplyMatricesWithProcesses(A, B, C, size, num_processes);

    getrusage(RUSAGE_SELF, &end);
    getrusage(RUSAGE_CHILDREN, &end);

    PerformanceStats stats;
    stats.user_time = (timeval_to_seconds(end.ru_utime) - timeval_to_seconds(start.ru_utime));

    writeCSVHeaderIfNeeded(CSV_FILENAME);
    appendResults(CSV_FILENAME, size, num_processes, stats.user_time);

    if (size <= 500) validateResult(A, B, C, size);

    printf("Matriz %d x %d con %d procesos\n", size, size, num_processes);
    printf("Tiempo de usuario: %.6f s\n", stats.user_time);

    shmdt(A); shmdt(B); shmdt(C);
    shmctl(shmid_A, IPC_RMID, NULL);
    shmctl(shmid_B, IPC_RMID, NULL);
    shmctl(shmid_C, IPC_RMID, NULL);

    return EXIT_SUCCESS;
}
