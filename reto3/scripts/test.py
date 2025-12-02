#!/usr/bin/env python3
import subprocess
import os

RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)

N_SIZES = [5000, 10000, 20000, 40000, 60000, 80000]
TRIALS = 10
STEPS = 300
DENSITY = 0.30
PRINT_FREQ = 100

MPI_PROCS = [2, 4, 6, 8]
MPI_HOSTS = "wn1,wn2,wn3"


def run(cmd):
    subprocess.run(cmd, shell=True, check=True)


def run_serial():
    out = os.path.join(RESULTS_DIR, "serial_times.csv")
    with open(out, "w") as f:
        f.write("N,trial,real_time,user_cpu,sys_cpu,memory_kb\n")

        for trial in range(1, TRIALS + 1):
            for N in N_SIZES:
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


def run_mpi():
    for p in MPI_PROCS:
        out = os.path.join(RESULTS_DIR, f"mpi_{p}p.csv")
        with open(out, "w") as f:
            f.write("N,trial,processes,mpi_time,user_cpu,sys_cpu,memory_kb,comm_time\n")

            for trial in range(1, TRIALS + 1):
                for N in N_SIZES:
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

                    f.write(f"{N},{trial},{p},{mpi_time},{user_cpu},{sys_cpu},{mem_kb},{comm_time}\n")


def main():
    print("=== SERIAL ===")
    run_serial()
    print("=== MPI ===")
    run_mpi()
    print("=== FIN ===")


if __name__ == "__main__":
    main()
