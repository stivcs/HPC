#!/usr/bin/env python3
import subprocess
import os

RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)

N_SIZES = [20000, 40000, 80000, 100000, 120000, 140000]
TRIALS = 10
STEPS = 300
DENSITY = 0.30
PRINT_FREQ = 100

MPI_PROCS = [2, 4, 6, 8]
MPI_HOSTS = "wn1,wn2,wn3"


def run(cmd):
    print(f"\n>>> Ejecutando comando:\n{cmd}")
    subprocess.run(cmd, shell=True, check=True)
    print("✔ Completado\n")


def run_serial():
    print("\n====================================")
    print("   EJECUTANDO PRUEBAS SECUENCIALES  ")
    print("====================================\n")

    out = os.path.join(RESULTS_DIR, "serial_times.csv")
    with open(out, "w") as f:
        f.write("N,trial,real_time,user_cpu,sys_cpu,memory_kb\n")

        for trial in range(1, TRIALS + 1):
            print(f"\n--- Trial serial #{trial}/{TRIALS} ---")

            for N in N_SIZES:
                print(f"[Serial] N = {N}")

                tmp = f"{RESULTS_DIR}/tmp_serial.csv"
                cmd = f"./traffic_serial {N} {STEPS} {DENSITY} {PRINT_FREQ} {tmp}"
                run(cmd)

                with open(tmp) as tf:
                    next(tf)  # skip header
                    values = tf.readline().strip().split(",")

                real_time = values[1]
                user_cpu = values[2]
                sys_cpu  = values[3]
                mem_kb   = values[4]

                f.write(f"{N},{trial},{real_time},{user_cpu},{sys_cpu},{mem_kb}\n")

    print(f"\n✔ Serial terminado. Guardado en {out}\n")


def run_mpi():
    print("\n====================================")
    print("       EJECUTANDO PRUEBAS MPI       ")
    print("====================================\n")

    for p in MPI_PROCS:
        print(f"\n==== NÚMERO DE PROCESOS: {p} ====\n")

        out = os.path.join(RESULTS_DIR, f"mpi_{p}p.csv")
        with open(out, "w") as f:
            f.write("N,trial,processes,mpi_time,user_cpu,sys_cpu,memory_kb,comm_time\n")

            for trial in range(1, TRIALS + 1):
                print(f"\n--- Trial MPI #{trial}/{TRIALS} con {p} procesos ---")

                for N in N_SIZES:
                    print(f"[MPI p={p}] N = {N}")

                    tmp = f"{RESULTS_DIR}/tmp_mpi.csv"
                    cmd = (
                        f"mpiexec -n {p} -host {MPI_HOSTS} -oversubscribe "
                        f"./traffic_mpi {N} {STEPS} {DENSITY} {PRINT_FREQ} {tmp}"
                    )
                    run(cmd)

                    with open(tmp) as tf:
                        next(tf)
                        values = tf.readline().strip().split(",")

                    mpi_time = values[1]
                    user_cpu = values[2]
                    sys_cpu  = values[3]
                    mem_kb   = values[4]
                    comm_time = values[5]

                    f.write(
                        f"{N},{trial},{p},{mpi_time},{user_cpu},{sys_cpu},{mem_kb},{comm_time}\n"
                    )

        print(f"✔ Guardado: {out}\n")

    print("\n✔ MPI terminado.\n")


def main():
    print("\n=======================================")
    print("     TEST HPC AUTOMÁTICO COMPLETO")
    print("=======================================\n")

    run_serial()
    run_mpi()

    print("\n=======================================")
    print("           TODO FINALIZADO")
    print("=======================================\n")


if __name__ == "__main__":
    main()
