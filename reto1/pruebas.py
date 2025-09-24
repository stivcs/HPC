import subprocess
import time
import os

# ================== CONFIG ==================
matrix_sizes = [100000, 200000, 300000]
workers = [2, 4, 8, 12]
iterations = 3

# Ejecutables
secuencial_dartboard = os.path.join("secuencial", "dartboard")
secuencial_needles   = os.path.join("secuencial", "needles")

hilos_dartboard = os.path.join("hilos", "dartboard")
hilos_needles   = os.path.join("hilos", "needles")

procesos_dartboard = os.path.join("procesos", "dartboard")
procesos_needles   = os.path.join("procesos", "needles")

# ================== FUNCION ==================
def run_and_time(cmd):
    start = time.time()
    subprocess.run(cmd, shell=True, check=True)
    end = time.time()
    return end - start

# ================== SECUENCIAL ==================
print("\n===== EJECUTANDO SECUENCIAL =====")
for exe, name in [(secuencial_dartboard, "Dartboard"), (secuencial_needles, "Needles")]:
    print(f"\n### {name.upper()} ###")
    for it in range(1, iterations + 1):
        print(f"\n--- Iteración {it} ---")
        for size in matrix_sizes:
            cmd = f"./{exe} {size}"
            elapsed = run_and_time(cmd)
            print(f"[Secuencial-{name}] Tamaño {size} -> {elapsed:.4f} s")

# ================== HILOS ==================
print("\n===== EJECUTANDO HILOS =====")
for exe, name in [(hilos_dartboard, "Dartboard"), (hilos_needles, "Needles")]:
    print(f"\n### {name.upper()} ###")
    for w in workers:
        print(f"\n>>> HILOS = {w} <<<")
        for it in range(1, iterations + 1):
            print(f"\n--- Iteración {it} ---")
            for size in matrix_sizes:
                cmd = f"./{exe} {size} {w}"
                elapsed = run_and_time(cmd)
                print(f"[Hilos-{name}] Tamaño {size}, hilos {w} -> {elapsed:.4f} s")

# ================== PROCESOS ==================
print("\n===== EJECUTANDO PROCESOS =====")
for exe, name in [(procesos_dartboard, "Dartboard"), (procesos_needles, "Needles")]:
    print(f"\n### {name.upper()} ###")
    for w in workers:
        print(f"\n>>> PROCESOS = {w} <<<")
        for it in range(1, iterations + 1):
            print(f"\n--- Iteración {it} ---")
            for size in matrix_sizes:
                cmd = f"./{exe} {size} {w}"
                elapsed = run_and_time(cmd)
                print(f"[Procesos-{name}] Tamaño {size}, procesos {w} -> {elapsed:.4f} s")