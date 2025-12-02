#!/usr/bin/env python3
import subprocess
import time
import os

RESULTS_DIR = "results"

# Asegurar carpeta de resultados
os.makedirs(RESULTS_DIR, exist_ok=True)

# Hosts fijos para MPI
MPI_HOSTS = "wn1,wn2,wn3"

# Pruebas seriales quemadas
SERIAL_TESTS = [
    # (N, steps, density, print_freq, output_file)
    (10000, 200, 0.25, 50, "serial_10k.csv"),
    (50000, 400, 0.30, 50, "serial_50k.csv"),
    (100000, 500, 0.30, 50, "serial_100k.csv"),
]

# Pruebas MPI quemadas
MPI_TESTS = [
    # (N, steps, density, print_freq, processes, output_file)
    (10000, 200, 0.25, 50, 4, "mpi_4p_10k.csv"),
    (50000, 400, 0.30, 50, 7, "mpi_7p_50k.csv"),
    (100000, 500, 0.30, 50, 9, "mpi_9p_100k.csv"),
]

def run(cmd):
    print(f"\n>>> Ejecutando:\n{cmd}\n")
    start = time.time()
    try:
        out = subprocess.run(cmd, shell=True, check=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             text=True)
        elapsed = time.time() - start
        print(f"✔ Finalizado en {elapsed:.3f} s")
        return out.stdout
    except subprocess.CalledProcessError as e:
        print("✘ Error al ejecutar comando")
        print(e.stderr)
        return None


def run_serial_tests():
    print("\n============================")
    print("   PRUEBAS SERIALES")
    print("============================")
    for N, steps, density, freq, outname in SERIAL_TESTS:
        path = os.path.join(RESULTS_DIR, outname)
        cmd = f"./src/traffic_serial {N} {steps} {density} {freq} {path}"
        run(cmd)


def run_mpi_tests():
    print("\n============================")
    print("   PRUEBAS MPI")
    print("============================")
    for N, steps, density, freq, procs, outname in MPI_TESTS:
        path = os.path.join(RESULTS_DIR, outname)
        cmd = (
            f"mpiexec -n {procs} "
            f"-host {MPI_HOSTS} -oversubscribe "
            f"./src/traffic_mpi {N} {steps} {density} {freq} {path}"
        )
        run(cmd)


def main():
    print("\n=======================================")
    print("      TEST AUTOMÁTICO COMPLETO")
    print("=======================================\n")

    run_serial_tests()
    run_mpi_tests()

    print("\n=======================================")
    print("   TODAS LAS PRUEBAS HAN FINALIZADO")
    print("=======================================\n")


if __name__ == "__main__":
    main()
