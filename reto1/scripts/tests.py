import subprocess
import time
import os
import csv

# ================== CONFIG ==================
#crece logaritmicamente 1.5 desde 100000 
matrix_sizes = [
    400000,
    600000,
    900000,
    1350000,
    2025000,
    3037500,
    4556250,
    6834375,
    10251562,
    15377343,
    23066015,
    34599022,
    51898533,
    77847800,
    116771700
]
workers = [2, 4, 8, 12]
iterations = 10

# Carpetas
BIN_DIR = "bin"
RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)

# Ejecutables (ya compilados en bin/)
executables = {
    "Secuencial": {
        "Dartboard": os.path.join(BIN_DIR, "secuencial_dartboard"),
        "Needles":   os.path.join(BIN_DIR, "secuencial_needles"),
    },
    "Hilos": {
        "Dartboard": os.path.join(BIN_DIR, "hilos_dartboard"),
        "Needles":   os.path.join(BIN_DIR, "hilos_needles"),
    },
    "Procesos": {
        "Dartboard": os.path.join(BIN_DIR, "procesos_dartboard"),
        "Needles":   os.path.join(BIN_DIR, "procesos_needles"),
    }
}

# ================== FUNCION ==================
def run_and_time(cmd):
    start = time.time()
    subprocess.run(cmd, shell=True, check=True)
    end = time.time()
    return end - start

def init_csv(filename):
    path = os.path.join(RESULTS_DIR, filename)
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["modo", "algoritmo", "tamaño", "workers", "iteracion", "tiempo"])
    return path

def append_csv(path, row):
    with open(path, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(row)

# ================== EJECUCION ==================
for mode, algos in executables.items():
    for name, exe in algos.items():
        print(f"\n===== EJECUTANDO {mode.upper()} - {name.upper()} =====")

        # Un archivo por combinación
        csv_path = init_csv(f"{mode.lower()}_{name.lower()}.csv")

        if mode == "Secuencial":
            for it in range(1, iterations + 1):
                for size in matrix_sizes:
                    cmd = f"{exe} {size}"
                    elapsed = run_and_time(cmd)
                    print(f"[{mode}-{name}] Tamaño {size}, Iter {it} -> {elapsed:.4f} s")
                    append_csv(csv_path, [mode, name, size, "-", it, f"{elapsed:.4f}"])

        else:  # Hilos o Procesos
            for w in workers:
                for it in range(1, iterations + 1):
                    for size in matrix_sizes:
                        cmd = f"{exe} {size} {w}"
                        elapsed = run_and_time(cmd)
                        print(f"[{mode}-{name}] Tamaño {size}, {mode} {w}, Iter {it} -> {elapsed:.4f} s")
                        append_csv(csv_path, [mode, name, size, w, it, f"{elapsed:.4f}"])
