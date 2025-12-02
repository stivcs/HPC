#!/usr/bin/env python3
import subprocess
import time
import os
import resource

RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)

# Tamaños de prueba
N_SIZES = [
    5000, 10000, 20000, 40000,
    60000, 80000
]

TRIALS = 10

# Parámetros del automata
STEPS = 300
DENSITY = 0.30
PRINT_FREQ = 100

# Configuración MPI
MPI_PROCS = [2, 4, 6, 8]
MPI_HOSTS = "wn1,wn2,wn3"


def get_memory_kb(pid):
    try:
        with open(f"/proc/{pid}/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    return int(line.split()[1])
    except:
        pass
    return -1


def run_and_profile(cmd):
    print(f"\n>>> Ejecutando:\n{cmd}")

    proc = subprocess.Popen(cmd, shell=True)
    pid = proc.pid

    start = time.time()
    proc.wait()
    end = time.time()

    real_time = end - start

    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    user_cpu = usage.ru_utime
    sys_cpu = usage.ru_stime

    mem_kb = get_memory_kb(pid)

    print(f"✔ Tiempo real {real_time:.4f}s | CPU {user_cpu:.4f}/{sys_cpu:.4f} | MEM {mem_kb} KB")

    return real_time, user_cpu, sys_cpu, mem_kb


def run_serial_tests():

    print("\n====================================")
    print("      PRUEBAS SECUENCIALES")
    print("====================================")

    out = os.path.join(RESULTS_DIR, "serial_times.csv")

    with open(out, "w") as f:
        f.write("N,trial,time_real,user_cpu,sys_cpu,memory_kb\n")

        for trial in range(1, TRIALS + 1):
            print(f"\n===== TRIAL {trial} (SECUENCIAL) =====\n")
            for N in N_SIZES:

                tmp_out = f"{RESULTS_DIR}/serial_{N}_{trial}.csv"
                cmd = f"./traffic_serial {N} {STEPS} {DENSITY} {PRINT_FREQ} {tmp_out}"

                real_t, user_t, sys_t, mem = run_and_profile(cmd)

                f.write(f"{N},{trial},{real_t},{user_t},{sys_t},{mem}\n")

    print(f"\n✔ Serial guardado en: {out}")


def run_mpi_tests():

    print("\n====================================")
    print("          PRUEBAS MPI")
    print("====================================")

    for p in MPI_PROCS:

        out = os.path.join(RESULTS_DIR, f"mpi_{p}p.csv")

        with open(out, "w") as f:
            f.write("N,trial,processes,time_real,user_cpu,sys_cpu,memory_kb\n")

            for trial in range(1, TRIALS + 1):
                print(f"\n===== TRIAL {trial} (MPI {p} PROCESOS) =====\n")

                for N in N_SIZES:

                    tmp_out = f"{RESULTS_DIR}/mpi_{p}p_{N}_{trial}.csv"

                    cmd = (
                        f"mpiexec -n {p} -host {MPI_HOSTS} -oversubscribe "
                        f"./traffic_mpi {N} {STEPS} {DENSITY} {PRINT_FREQ} {tmp_out}"
                    )

                    real_t, user_t, sys_t, mem = run_and_profile(cmd)

                    f.write(f"{N},{trial},{p},{real_t},{user_t},{sys_t},{mem}\n")

        print(f"✔ Archivo MPI {p} procesos guardado en: {out}")


def main():
    print("\n=======================================")
    print("        TEST HPC AUTOMÁTICO")
    print("=======================================\n")

    run_serial_tests()
    run_mpi_tests()

    print("\n=======================================")
    print("              COMPLETADO")
    print("=======================================\n")


if __name__ == "__main__":
    main()
