#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>  // Para getrusage
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>         // Para manejo de errores

// Nombre del directorio para almacenamiento de datos
#define DATA_DIR "Procesos_Data"

// Archivo único donde se guardan las métricas
#define CSV_FILENAME DATA_DIR "/tiempos.csv"

// Estructura para almacenar estadísticas de rendimiento mejorada
typedef struct {
    double real_time;       // tiempo real en segundos (wall-clock time)
    double user_time;       // tiempo de CPU en modo usuario
    double system_time;     // tiempo de CPU en modo kernel
    double total_cpu_time;  // suma de user_time y system_time
    long long total_operations;  // número total de operaciones aritméticas
    double gops;            // operaciones de punto flotante por segundo (en miles de millones)
    double elements_per_second; // elementos procesados por segundo (millones)
    size_t memory_used;     // memoria aproximada en MB
    double parallel_efficiency;  // Eficiencia paralela (0-1)
    double speedup; 
} PerformanceStats;

// Estructura para pasar datos a cada proceso
typedef struct {
    int process_id;
    int num_processes;
    int size;
    int block_size;  // Tamaño del bloque para bloqueo
    int* A_flat;     // Matriz A aplanada para memoria compartida
    int* B_flat;     // Matriz B aplanada para memoria compartida
    int* C_flat;     // Matriz C aplanada para memoria compartida
} ProcessData;

#define get_element(matrix, row, col, size) ((matrix)[(row) * (size) + (col)])
#define set_element(matrix, row, col, size, value) ((matrix)[(row) * (size) + (col)] = (value))
#define add_to_element(matrix, row, col, size, value) ((matrix)[(row) * (size) + (col)] += (value))

// Función para crear directorio si no existe
int createDirectoryIfNotExists(const char* dirPath) {
    struct stat st = {0};
    
    // Verificar si el directorio ya existe
    if (stat(dirPath, &st) == -1) {
        // Crear el directorio con permisos 0755
        if (mkdir(dirPath, 0755) == -1) {
            fprintf(stderr, "Error: No se pudo crear el directorio %s: %s\n", 
                    dirPath, strerror(errno));
            return 0;
        }
        printf("Directorio creado: %s\n", dirPath);
    } else {
        printf("Directorio ya existe: %s\n", dirPath);
    }
    
    return 1;
}

// Función para crear matrices compartidas
int create_shared_matrix(int size, int** matrix_ptr) {
    size_t matrix_size = size * size * sizeof(int);
    
    // Crear segmento de memoria compartida
    int shmid = shmget(IPC_PRIVATE, matrix_size, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    
    // Adjuntar segmento
    *matrix_ptr = (int*)shmat(shmid, NULL, 0);
    if (*matrix_ptr == (int*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    
    return shmid;
}

// Función para llenar una matriz con números aleatorios
void fillMatrix(int* matrix, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            set_element(matrix, i, j, size, rand() % 100 + 1);
        }
    }
}

// Función para inicializar la matriz resultado con ceros
void initResultMatrix(int* matrix, int size) {
    memset(matrix, 0, size * size * sizeof(int));
}

// Función que ejecutará cada proceso - Utilizando bloqueo y optimizaciones
void multiplyMatricesProcessOptimized(ProcessData* data) {
    int* A = data->A_flat;
    int* B = data->B_flat;
    int* C = data->C_flat;
    int size = data->size;
    int process_id = data->process_id;
    int num_processes = data->num_processes;
    int block_size = data->block_size;
    
    // Multiplicación por bloques, cada proceso procesa bloques específicos
    for (int i0 = 0; i0 < size; i0 += block_size) {
        int imax = (i0 + block_size < size) ? i0 + block_size : size;
        
        // Asignación de bloques por proceso (por columnas para mejor paralelismo)
        for (int j0 = process_id * block_size; j0 < size; j0 += num_processes * block_size) {
            int jmax = (j0 + block_size < size) ? j0 + block_size : size;
            
            // Bloques de k
            for (int k0 = 0; k0 < size; k0 += block_size) {
                int kmax = (k0 + block_size < size) ? k0 + block_size : size;
                
                // Recorrer dentro del bloque
                for (int i = i0; i < imax; i++) {
                    for (int k = k0; k < kmax; k++) {
                        int aik = get_element(A, i, k, size); // Almacenar en registro para localidad
                        
                        // Usar bucle autovectorizable
                        for (int j = j0; j < jmax; j++) {
                            add_to_element(C, i, j, size, aik * get_element(B, k, j, size));
                        }
                    }
                }
            }
        }
    }
}

// Función para multiplicar matrices en paralelo con procesos y optimizaciones
void multiplyMatricesWithProcesses(int* A, int* B, int* C, int size, int num_processes) {
    pid_t* pids = (pid_t*)malloc(num_processes * sizeof(pid_t));
    if (pids == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para los IDs de procesos\n");
        exit(EXIT_FAILURE);
    }
    
    // Determinar tamaño de bloque según el tamaño de caché L1/L2
    // Esto es una heurística simple que puede ajustarse
    int block_size = 32;  // Un buen valor inicial para la mayoría de CPUs
    
    if (size > 1000) block_size = 64;
    if (size > 2000) block_size = 128;
    
    // Estructura para datos de proceso
    ProcessData data;
    data.size = size;
    data.num_processes = num_processes;
    data.A_flat = A;
    data.B_flat = B;
    data.C_flat = C;
    data.block_size = block_size;
    
    // Crear procesos hijos
    for (int i = 0; i < num_processes; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Error al crear el proceso
            fprintf(stderr, "Error: No se pudo crear el proceso %d\n", i);
            for (int j = 0; j < i; j++) {
                // Matar procesos ya creados
                kill(pids[j], SIGTERM);
            }
            exit(EXIT_FAILURE);
        } else if (pids[i] == 0) {
            // Código del proceso hijo
            data.process_id = i;
            multiplyMatricesProcessOptimized(&data);
            
            // El proceso hijo termina aquí
            exit(EXIT_SUCCESS);
        }
    }
    
    // Proceso padre espera a que todos los hijos terminen
    for (int i = 0; i < num_processes; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
            fprintf(stderr, "Advertencia: El proceso %d no terminó correctamente\n", i);
        }
    }
    
    free(pids);
}

// Función para verificar resultado
void validateResult(int* A, int* B, int* C, int size) {
    // Cálculo de validación de algunos elementos
    int expected, actual;
    int correct = 1;
    
    // Para matrices grandes, solo comprobamos una muestra
    int step = (size > 1000) ? size / 10 : 1;
    
    for (int i = 0; i < size && correct; i += step) {
        for (int j = 0; j < size && correct; j += step) {
            expected = 0;
            for (int k = 0; k < size; k++) {
                expected += get_element(A, i, k, size) * get_element(B, k, j, size);
            }
            actual = get_element(C, i, j, size);
            if (expected != actual) {
                printf("Error de validación en C[%d][%d]: esperado %d, obtenido %d\n", 
                       i, j, expected, actual);
                correct = 0;
            }
        }
    }
    
    if (correct) {
        printf("Validación: Correcta (muestra)\n");
    }
}

// Función para obtener el número de CPUs disponibles
int getNumCPUs() {
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus <= 0) {
        return 1; // Por defecto, usar 1 si no se puede determinar
    }
    return (int)num_cpus;
}

// Función para convertir timeval a segundos
double timeval_to_seconds(struct timeval tv) {
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

// Función para obtener tiempos de usuario y sistema
void get_cpu_times(struct rusage* start_rusage, struct rusage* end_rusage, double* user_time, double* system_time) {
    *user_time = timeval_to_seconds(end_rusage->ru_utime) - timeval_to_seconds(start_rusage->ru_utime);
    *system_time = timeval_to_seconds(end_rusage->ru_stime) - timeval_to_seconds(start_rusage->ru_stime);
}

// ------------------------------------------------------------------
// NUEVAS funciones para guardar en un único CSV (append) con 5 columnas
// matrix_size,num_processes,real_time,user_time,system_time
// ------------------------------------------------------------------

// Escribe la cabecera si el archivo no existe o está vacío
void writeSimpleCSVHeaderIfNeeded(const char* filename) {
    FILE* file = fopen(filename, "a+");
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir %s para escribir cabecera: %s\n", filename, strerror(errno));
        return;
    }
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) { // archivo vacío -> escribir cabecera
        fprintf(file, "tamañoMatriz,#procesos,tiempo_Usuario,tiempo_Sistema\n");
    }
    fclose(file);
}

// Escribe únicamente las columnas solicitadas en modo append
void appendSimpleResults(const char* filename, int matrix_size, int num_processes, double real_time, double user_time, double system_time) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir %s para append: %s\n", filename, strerror(errno));
        return;
    }
    fprintf(file, "%d,%d,%.9f,%.9f\n", matrix_size, num_processes, user_time, system_time);
    fclose(file);
}

// Función para capturar estadísticas de memoria usando getrusage
size_t get_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    // maxrss está en kilobytes, convertimos a megabytes
    return usage.ru_maxrss / 1024;
}

int main(int argc, char* argv[]) {
    // Verificar argumentos de línea de comandos
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Uso: %s <tamaño_matriz> [iteración] [num_procesos]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Error: El tamaño de la matriz debe ser un número positivo\n");
        return EXIT_FAILURE;
    }
    
    // Si se proporciona el número de iteración, lo usamos para el CSV (aunque ya no lo guardamos)
    int iteration = 1;
    if (argc >= 3) {
        iteration = atoi(argv[2]);
    }
    
    // Si se proporciona el número de procesos, lo configuramos
    int num_processes = getNumCPUs(); // Por defecto, usar el número de CPUs disponibles
    if (argc == 4) {
        int requested_processes = atoi(argv[3]);
        if (requested_processes > 0) {
            num_processes = requested_processes;
        }
    }
    
    srand(time(NULL));
    
    // Crear directorio para almacenar datos si no existe
    if (!createDirectoryIfNotExists(DATA_DIR)) {
        fprintf(stderr, "No se pudo garantizar el directorio %s\n", DATA_DIR);
        return EXIT_FAILURE;
    }
    
    // IDs de los segmentos de memoria compartida
    int shmid_A, shmid_B, shmid_C;
    // Punteros a las matrices compartidas
    int *A, *B, *C;
    
    printf("Creando matrices compartidas de %dx%d...\n", size, size);
    
    // Crear matrices en memoria compartida
    shmid_A = create_shared_matrix(size, &A);
    shmid_B = create_shared_matrix(size, &B);
    shmid_C = create_shared_matrix(size, &C);
    
    // Llenar matrices A y B con datos aleatorios
    printf("Llenando matrices con datos aleatorios...\n");
    fillMatrix(A, size);
    fillMatrix(B, size);
    
    // Inicializar matriz C con ceros
    initResultMatrix(C, size);
    
    // Variables para medir el tiempo real
    struct timespec start_time, end_time;
    
    // Variables para medir tiempos de CPU de procesos padre e hijos
    struct rusage start_rusage_self, end_rusage_self;
    struct rusage start_rusage_children, end_rusage_children;
    
    PerformanceStats stats = {0};

    printf("Iniciando multiplicación de matrices con %d procesos (algoritmo optimizado)...\n", num_processes);
    
    // Obtener recursos utilizados antes de la operación para el proceso padre y los hijos
    if (getrusage(RUSAGE_SELF, &start_rusage_self) < 0) {
        perror("Error en getrusage inicial (SELF)");
    }
    
    if (getrusage(RUSAGE_CHILDREN, &start_rusage_children) < 0) {
        perror("Error en getrusage inicial (CHILDREN)");
    }
    
    
    // Obtener tiempo real antes de la operación
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Ejecutar la multiplicación de matrices
    multiplyMatricesWithProcesses(A, B, C, size, num_processes);
    
    // Obtener tiempo real después de la operación
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Obtener recursos utilizados después de la operación para el proceso padre y los hijos
    if (getrusage(RUSAGE_SELF, &end_rusage_self) < 0) {
        perror("Error en getrusage final (SELF)");
    }
    
    if (getrusage(RUSAGE_CHILDREN, &end_rusage_children) < 0) {
        perror("Error en getrusage final (CHILDREN)");
    }
    
    // // Debug: Imprimir valores finales de rusage
    // printf("DEBUG - Valores finales de rusage:\n");
    // printf("  SELF: user_sec=%ld user_usec=%ld sys_sec=%ld sys_usec=%ld\n", 
    //        (long)end_rusage_self.ru_utime.tv_sec, (long)end_rusage_self.ru_utime.tv_usec, 
    //        (long)end_rusage_self.ru_stime.tv_sec, (long)end_rusage_self.ru_stime.tv_usec);
    // printf("  CHILDREN: user_sec=%ld user_usec=%ld sys_sec=%ld sys_usec=%ld\n", 
    //        (long)end_rusage_children.ru_utime.tv_sec, (long)end_rusage_children.ru_utime.tv_usec, 
    //        (long)end_rusage_children.ru_stime.tv_sec, (long)end_rusage_children.ru_stime.tv_usec);
    
    // Calcular tiempo real transcurrido
    stats.real_time = (end_time.tv_sec - start_time.tv_sec) + 
                      (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    // Calcular tiempos de CPU combinados
    double user_time_self, system_time_self;
    double user_time_children, system_time_children;
    
    get_cpu_times(&start_rusage_self, &end_rusage_self, &user_time_self, &system_time_self);
    get_cpu_times(&start_rusage_children, &end_rusage_children, &user_time_children, &system_time_children);
    
    stats.user_time = user_time_self + user_time_children;
    stats.system_time = system_time_self + system_time_children;
    stats.total_cpu_time = stats.user_time + stats.system_time;

    // Evitar división por cero al calcular speedup si real_time es muy pequeño o cero
    if (stats.real_time > 1e-12) {
        stats.speedup = stats.total_cpu_time / stats.real_time;
    } else {
        stats.speedup = 0.0;
    }
    stats.parallel_efficiency = (num_processes > 0) ? (stats.speedup / num_processes) : 0.0;
    
    // Calcular operaciones y rendimiento
    stats.total_operations = (long long)size * size * (2 * size - 1);
    
    // Obtener uso de memoria
    stats.memory_used = get_memory_usage();
    
    // Validar que real_time no sea cero o extremadamente pequeño
    if (stats.real_time > 1e-9) {  // Evita divisiones por números muy pequeños
        stats.gops = (stats.total_operations / stats.real_time) / 1000000000.0;
        stats.elements_per_second = (double)(size * size) / stats.real_time / 1000000.0;
    } else {
        stats.gops = 0.0;
        stats.elements_per_second = 0.0;
    }
    
    // Escribir resultados en el archivo único (append)
    writeSimpleCSVHeaderIfNeeded(CSV_FILENAME);
    appendSimpleResults(CSV_FILENAME, size, num_processes, stats.real_time, stats.user_time, stats.system_time);
    
    // Validar el resultado para matrices pequeñas
    if (size <= 500) {
        validateResult(A, B, C, size);
    }
    
    printf("\n===== ESTADÍSTICAS DE RENDIMIENTO =====\n");
    printf("Tamaño de la matriz: %d x %d\n", size, size);
    printf("Número de procesos: %d\n", num_processes);
    printf("Tiempo real (wall clock): %.9f segundos\n", stats.real_time);
    printf("Tiempo de CPU: %.9f segundos (%.2f%% de utilización)\n", 
           stats.total_cpu_time, 
           (stats.total_cpu_time / stats.real_time) * 100);
    printf("  └─ Tiempo de usuario: %.9f segundos (%.2f%%)\n", 
           stats.user_time, 
           (stats.user_time / stats.total_cpu_time) * 100);
    printf("  └─ Tiempo de sistema: %.9f segundos (%.2f%%)\n", 
           stats.system_time, 
           (stats.system_time / stats.total_cpu_time) * 100);
    printf("Total de operaciones: %lld\n", stats.total_operations);
    printf("Rendimiento: %.6f GOPS\n", stats.gops);
    printf("Elementos procesados por segundo: %.6f millones\n", stats.elements_per_second);
    printf("Memoria utilizada: aproximadamente %lu MB\n", stats.memory_used);
    printf("Aceleración (speedup): %.2f\n", stats.speedup);
    printf("Eficiencia paralela: %.2f%%\n", stats.parallel_efficiency * 100);
    printf("Datos guardados en: %s\n", CSV_FILENAME);
    
    // Desadjuntar y liberar la memoria compartida
    shmdt(A);
    shmdt(B);
    shmdt(C);
    shmctl(shmid_A, IPC_RMID, NULL);
    shmctl(shmid_B, IPC_RMID, NULL);
    shmctl(shmid_C, IPC_RMID, NULL);
    
    return EXIT_SUCCESS;
}
