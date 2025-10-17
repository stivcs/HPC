#!/usr/bin/env python3
"""
test.py — Ejecuta pruebas para tamaños específicos de matrices
Secuencial + OpenMP (2,4,8,12 hilos), 10 repeticiones por configuración
"""

import subprocess
from pathlib import Path

# Tamaños específicos
SIZES = [675, 911, 1229, 1658, 2239, 3023, 4081]
OMP_THREADS = [2, 4, 8, 12]
RUNS = 10

BASE_DIR = Path(__file__).resolve().parent.parent
BIN_DIR = BASE_DIR / "bin"
RESULTS_DIR = BASE_DIR / "results"
SEQ_BIN = BIN_DIR / "secuencial"
OMP_BIN = BIN_DIR / "openmp_opt"

def compile_if_needed():
    if not SEQ_BIN.exists() or not OMP_BIN.exists():
        print("Compilando binarios...")
        subprocess.run(["make", "all"], check=True)

def run_tests(sizes, runs):
    compile_if_needed()

    for size in sizes:
        print(f"\n===== Tamaño {size}x{size} =====")

        # Secuencial
        for r in range(1, runs + 1):
            print(f"  Secuencial - ejecución {r}")
            subprocess.run([str(SEQ_BIN), str(size)], check=True)

        # OpenMP
        for t in OMP_THREADS:
            for r in range(1, runs + 1):
                print(f"  OpenMP - hilos {t}, ejecución {r}")
                subprocess.run([str(OMP_BIN), str(size), str(t)], check=True)

    print("\nPruebas completadas. Los resultados se guardan directamente en CSV.")

if __name__ == "__main__":
    run_tests(SIZES, RUNS)
