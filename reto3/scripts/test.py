#!/usr/bin/env python3
import subprocess
import os
import time

# ============================
# CONFIGURACIÓN DE PRUEBAS
# ============================

RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)

# 10 tamaños para pruebas
N_SIZES = [
    5000, 10000, 20000, 30000, 40000,
    50000, 60000, 70000, 80000, 100000
]

# Steps, densidad y print_freq constantes para todas las pruebas
STEPS = 300
DENSITY = 0.30
PRINT_FREQ = 100

# Hosts para MPI
MPI_HOSTS = "wn1,wn2,wn3"

# Número de procesos para MPI (10 pruebas)
MPI_PROCS = [1, 2, 3, 4, 5, 6, 7, 8, 10, 12]


# ============================
# FUNCIONES AUXILIARES
# ============================

def run(cmd):
    """Ejecuta un comando de shell y retorna el tiempo total."""
    print(f"\n>>> Ejecutando:\n{cmd}")
    start = time.time()

    try:
        out = subprocess.run(cmd, shell=True, check=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             text=True)
        elapsed = time.time() - start
        print(f"✔ Finalizado en {elapsed:.4f} s")
        return elapsed

    except subprocess.CalledProcessError as e:
        print("✘ Error en ejecución")
        print(e.stderr)
        return None


def run_perf(cmd, output_name):
    """Perf profiling."""
    perf_cmd = f"perf stat -o {output_name} {cmd}"
    print(f"\n>>> PERF:\n{perf_cmd}\n")
    subprocess.run(perf_cmd, shell=True)


def run_massif(cmd, output_name):
    """Valgrind massif profiling."""
    massif_cmd = f"valgrind --tool=massif --massif-out-file={output_name} {cmd}"
    print(f"\n>>> MASSIF:\n{massif_cmd}\n")
    subprocess.run(massif_cmd, shell=True)


# ============================
# PRUEBAS SECUENCIALES
# ============================

def run_serial_tests():
    print("\n====================================")
    print("  PRUEBAS SECUENCIALES (10 pruebas)")
    print("====================================")

    csv_path = os.path.join(RESULTS_DIR, "serial_times.csv")

    with open(csv_path, "w") as f:
        f.write("N,time_seconds\n")

        for N in N_SIZES:
            output_temp = f"results/serial_{N}.csv"
            cmd = f"./src/traffic_serial {N} {STEPS} {DENSITY} {PRINT_FREQ} {output_temp}"

            t = run(cmd)
            f.write(f"{N},{t}\n")

            # Profiling
            run_perf(cmd, f"results/perf_serial_{N}.txt")
            run_massif(cmd, f"results/massif_serial_{N}.out")

    print(f"\n✔ CSV serial generado en: {csv_path}")


# ============================
# PRUEBAS MPI
# ============================

def run_mpi_tests():
    print("\n====================================")
    print("  PRUEBAS MPI (10 pruebas)")
    print("====================================")

    csv_path = os.path.join(RESULTS_DIR, "mpi_times.csv")

    with open(csv_path, "w") as f:
        f.write("N,processes,time_seconds\n")

        for p in MPI_PROCS:
            for N in N_SIZES:
                output_temp = f"results/mpi_{p}p_{N}.csv"

                cmd = (
                    f"mpiexec -n {p} -host {MPI_HOSTS} -oversubscribe "
                    f"./src/traffic_mpi {N} {STEPS} {DENSITY} {PRINT_FREQ} {output_temp}"
                )

                t = run(cmd)
                f.write(f"{N},{p},{t}\n")

                # Profiling
                run_perf(cmd, f"results/perf_mpi_{p}p_{N}.txt")
                run_massif(cmd, f"results/massif_mpi_{p}p_{N}.out")

    print(f"\n✔ CSV mpi generado en: {csv_path}")


# ============================
# MAIN
# ============================

def main():
    print("\n=======================================")
    print("    TEST AUTOMÁTICO COMPLETO MPI/SEQ")
    print("=======================================\n")

    run_serial_tests()
    run_mpi_tests()

    print("\n=======================================")
    print("        TODAS LAS PRUEBAS TERMINARON")
    print("=======================================\n")


if __name__ == "__main__":
    main()
