#!/usr/bin/env python3
import csv
import os
import random
import sys
import time
import numpy as np

def load_matrix_csv(path):
    """Carga una matriz CSV en forma de lista de listas (enteros)."""
    with open(path, 'r') as f:
        reader = csv.reader(f)
        matrix = [[int(x) for x in row] for row in reader]
    return np.array(matrix, dtype=np.int64)

def verify_multiplication(A, B, C, sample_fraction=0.01, max_samples=1000):
    """Verifica si C = A * B. Usa muestreo para matrices grandes."""
    n = A.shape[0]
    total_elements = n * n

    # Si la matriz es pequeña, revisar todo
    if total_elements <= 300 * 300:
        print(f"Verificando {total_elements:,} elementos (validación completa)...")
        expected = np.dot(A, B)
        diff = np.abs(expected - C)
        errors = np.count_nonzero(diff)
        return errors, total_elements

    # Si es grande, muestreo
    print(f"Verificando matriz grande ({n}x{n}), aplicando muestreo...")
    sample_size = min(int(total_elements * sample_fraction), max_samples)
    indices = random.sample(range(total_elements), sample_size)

    errors = 0
    for idx in indices:
        i, j = divmod(idx, n)
        expected = int(np.dot(A[i, :], B[:, j]))
        if expected != C[i, j]:
            errors += 1

    return errors, sample_size

def main():
    if len(sys.argv) < 2:
        print("Uso: python3 scripts/verify.py <tipo>")
        print("  tipo: secuencial | openmp")
        sys.exit(1)

    mode = sys.argv[1].lower()
    if mode not in ["secuencial", "openmp"]:
        print("Error: modo inválido. Usa 'secuencial' o 'openmp'.")
        sys.exit(1)

    base_dir = {
        "secuencial": "results/Secuencial_Data/matrices",
        "openmp": "results/OpenMp_Data/matrices"
    }[mode]

    # Verificar existencia de archivos
    paths = [os.path.join(base_dir, f"{name}.csv") for name in ["A", "B", "C_resultado"]]
    for p in paths:
        if not os.path.exists(p):
            print(f"Error: no se encuentra {p}")
            sys.exit(1)

    print(f"Cargando matrices desde: {base_dir}")
    start_time = time.time()
    A, B, C = map(load_matrix_csv, paths)

    if A.shape != B.shape or A.shape != C.shape:
        print("Error: las matrices no tienen dimensiones compatibles.")
        sys.exit(1)

    print(f"Dimensiones detectadas: {A.shape[0]} x {A.shape[1]}")

    errors, checked = verify_multiplication(A, B, C)
    elapsed = time.time() - start_time

    if errors == 0:
        print(f"✅ Verificación exitosa — {checked:,} elementos revisados en {elapsed:.2f} s")
    else:
        print(f"❌ Se detectaron {errors} errores en {checked:,} elementos revisados ({(errors/checked)*100:.4f}%)")
        sys.exit(1)

if __name__ == "__main__":
    main()
