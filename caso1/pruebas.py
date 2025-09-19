import subprocess
import time

# Tamaños de matrices (escala logarítmica con factor ~1.35)
matrix_sizes = [500, 675, 911, 1229, 1658, 2239, 3023, 4081, 5510]# incluir

# Número de hilos/procesos a usar (potencias de 2 hasta 24)
workers = [2, 4, 8, 16, 24]#

# Rutas a los ejecutables compilados
secuencial_exe = "./secuencial"
hilos_exe = "./hilos"
procesos_exe = "./procesos"

# Iteraciones
iterations = 10

def run_and_time(cmd):
    start = time.time()
    subprocess.run(cmd, shell=True, check=True)
    end = time.time()
    return end - start

# ================== SECUENCIAL ==================
#print("\n===== EJECUTANDO SECUENCIAL =====")
#for it in range(1, iterations + 1):
#    print(f"\n--- Iteración {it} ---")
#    for size in matrix_sizes:
#        cmd = f"{secuencial_exe} {size}"
#        elapsed = run_and_time(cmd)
#        print(f"[Secuencial] Tamaño {size} -> {elapsed:.4f} s")

# ================== HILOS ==================
print("\n===== EJECUTANDO HILOS =====")
for w in workers:
    print(f"\n>>> HILOS = {w} <<<")
    for it in range(1, iterations + 1):
        print(f"\n--- Iteración {it} ---")
        for size in matrix_sizes:
            cmd = f"{hilos_exe} {size} {w}"
            elapsed = run_and_time(cmd)
            print(f"[Hilos] Tamaño {size}, hilos {w} -> {elapsed:.4f} s")

# ================== PROCESOS ==================
print("\n===== EJECUTANDO PROCESOS =====")
for w in workers:
    print(f"\n>>> PROCESOS = {w} <<<")
    for it in range(1, iterations + 1):
        print(f"\n--- Iteración {it} ---")
        for size in matrix_sizes:
            cmd = f"{procesos_exe} {size} {w}"
            elapsed = run_and_time(cmd)
            print(f"[Procesos] Tamaño {size}, procesos {w} -> {elapsed:.4f} s")