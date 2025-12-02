#!/usr/bin/env python3
import subprocess
import time
import os
import resource

RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)

# 10 tamaños
N_SIZES = [
    5000, 10000, 20000, 30000, 40000,
    50000, 60000, 70000, 80000, 100000
]

# Parámetros fijos del modelo
STEPS = 300
DENSITY = 0.30
PRINT_FREQ = 100

# Números de procesos para MPI
MPI_PROCS = [1,2,3,4,5,6,7,8,10,12]
MPI_HOSTS = "wn1,wn2,wn3"


def get_memory_kb(pid):
    """Lee la memoria desde /proc/<pid>/status"""
    try:
        with open(f"/proc/{pid}/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    return int(line.split()[1])
    except:
        pass
    return -1


def run_and_profile(cmd):
    """
    Ejecuta un comando, mide:
    - tiempo real
    - tiempo CPU (hijos)
    - memoria usada
    """
    print(f"\n>>> Ejecutando:\n{cmd}")

    # Ejecutar proceso bajo Popen para obtener PID
    proc = subprocess.Popen(cmd, shell=True)
    pid = proc.pid

    start = time.time()
    proc.wait()
    end = time.time()

    # Tiempo real
    real_time = end - start

    # CPU (usuario y sistema)
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    user_cpu = usage.ru_utime
    sys_cpu = usage.ru_stime

    # Memoria
    mem_kb = get_memory_kb(pid)

    print(f"✔ Tiempo real: {real_time:.4f}s  CPU: {user_cpu:.4f}/{sys_cpu:.4f}s  Mem: {mem_kb} KB")

    return real_time, user_cpu, sys_cpu, mem_kb


def run_serial_tests():
    print("\n====================================")
    print("     PRUEBAS SECUENCIALES (10)")
    print("====================================")

    csv_path = os.path.join(RESULTS_DIR, "serial_times.csv")
    with open(csv_path, "w") as f:
        f.write("N,time_real,user_cpu,sys_cpu,memory_kb\n")

        for N in N_SIZES:
            out_file = f"{RESULTS_DIR}/serial_{N}.csv"
            cmd = f"./src/traffic_serial {N} {STEPS} {DENSITY} {PRINT_FREQ} {out_file}"

            real_t, user_t, sys_t, mem = run_and_profile(cmd)

            f.write(f"{N},{real_t},{user_t},{sys_t},{mem}\n")

    print(f"\n✔ Resultados seriales guardados en {csv_path}")


def run_mpi_tests():
    print("\n====================================")
    print("         PRUEBAS MPI (10)")
    print("====================================")

    csv_path = os.path.join(RESULTS_DIR, "mpi_times.csv")
    with open(csv_path, "w") as f:
        f.write("N,processes,time_real,user_cpu,sys_cpu,memory_kb\n")

        for p in MPI_PROCS:
            for N in N_SIZES:
                out_file = f"{RESULTS_DIR}/mpi_{p}p_{N}.csv"
                cmd = (
                    f"mpiexec -n {p} -host {MPI_HOSTS} -oversubscribe "
                    f"./src/traffic_mpi {N} {STEPS} {DENSITY} {PRINT_FREQ} {out_file}"
                )

                real_t, user_t, sys_t, mem = run_and_profile(cmd)

                f.write(f"{N},{p},{real_t},{user_t},{sys_t},{mem}\n")

    print(f"\n✔ Resultados MPI guardados en {csv_path}")


def main():
    print("\n=======================================")
    print("     TEST HPC AUTOMÁTICO COMPLETO")
    print("=======================================\n")

    run_serial_tests()
    run_mpi_tests()

    print("\n=======================================")
    print("           TODO FINALIZADO")
    print("=======================================\n")


if __name__ == "__main__":
    main()
