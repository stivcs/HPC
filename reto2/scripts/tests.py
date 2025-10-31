import subprocess
import time
import os
import csv

# ================== CONFIG ==================
# Crece logarítmicamente (x1.5 desde 400000)
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
    116771700,
    175157550,
    262736325,
    394104487,
    591156730,
    886735095,
    1330102642
]
workers = [2, 4, 8, 12]
iterations = 1

# Carpetas base
BIN_DIR = "bin"
RESULTS_DIR = "results"
PROFILE_DIR = os.path.join(RESULTS_DIR, "profile_reports")

# Crear carpetas base si no existen
os.makedirs(BIN_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)
os.makedirs(PROFILE_DIR, exist_ok=True)

# ================== SOLO OPENMP ==================
executables = {
    "OpenMP": {
        "Dartboard": os.path.join(BIN_DIR, "openmp_dartboard"),
        "Needles": os.path.join(BIN_DIR, "openmp_needles"),
    }
}

# ================== FUNCIONES ==================
def ensure_dir_exists(path):
    """Crea un directorio si no existe."""
    try:
        os.makedirs(path, exist_ok=True)
        print(f"✔ Carpeta verificada o creada: {path}")
    except Exception as e:
        print(f"❌ Error creando {path}: {e}")

def run_and_time(cmd):
    """Ejecuta un comando y devuelve su tiempo de ejecución."""
    start = time.time()
    subprocess.run(cmd, shell=True, check=True)
    end = time.time()
    return end - start

def init_csv(filename):
    """Crea un archivo CSV con encabezado."""
    path = os.path.join(RESULTS_DIR, filename)
    ensure_dir_exists(os.path.dirname(path))
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["modo", "algoritmo", "tamaño", "workers", "iteracion", "tiempo"])
    return path

def append_csv(path, row):
    """Agrega una fila al CSV."""
    with open(path, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(row)

# ================== EJECUCIÓN ==================
for mode, algos in executables.items():
    for name, exe in algos.items():
        print(f"\n===== EJECUTANDO {mode.upper()} - {name.upper()} =====")

        # Verificar ejecutable
        if not os.path.isfile(exe):
            print(f"⚠️  Ejecutable no encontrado: {exe}")
            continue

        # Crear carpeta específica de resultados
        algo_result_dir = os.path.join(RESULTS_DIR, mode.lower(), f"{name}_Data")
        ensure_dir_exists(algo_result_dir)

        csv_path = init_csv(f"{mode.lower()}_{name.lower()}.csv")

        for w in workers:
            for it in range(1, iterations + 1):
                for size in matrix_sizes:
                    cmd = f"{exe} {size} {w}"
                    try:
                        elapsed = run_and_time(cmd)
                        print(f"[{mode}-{name}] Tamaño {size}, Workers {w}, Iter {it} -> {elapsed:.4f} s")
                        append_csv(csv_path, [mode, name, size, w, it, f"{elapsed:.4f}"])
                    except subprocess.CalledProcessError as e:
                        print(f"❌ Error ejecutando {cmd}: {e}")
                    except Exception as e:
                        print(f"⚠️ Error inesperado: {e}")
