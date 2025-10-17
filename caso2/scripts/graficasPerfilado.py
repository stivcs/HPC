import pandas as pd
import matplotlib.pyplot as plt
import os

# === RUTA BASE ===
base_path = "results/perfilamiento"
os.makedirs("results/graficas", exist_ok=True)
graficas_path = "results/graficas"

# === ARCHIVOS DE ENTRADA ===
m1_path = os.path.join(base_path, "OpenMp_Data_perfilamiento_8hilos.csv")
m2_path = os.path.join(base_path, "maquina2_OpenMP_perfilamiento_8hilos.csv")

# === CARGA DE DATOS ===
m1 = pd.read_csv(m1_path)
m2 = pd.read_csv(m2_path)

# === NORMALIZAR COLUMNAS ===
m1.columns = m1.columns.str.lower()
m2.columns = m2.columns.str.lower()

# === ORDENAR POR TAMAÑO DE MATRIZ ===
m1 = m1.sort_values(by="matrix_size")
m2 = m2.sort_values(by="matrix_size")

# === GRAFICA 1: TIEMPO REAL ===
plt.figure(figsize=(8, 5))
plt.plot(m1["matrix_size"], m1["real_time_sec"], marker='o', label="Máquina 1 (Data)")
plt.plot(m2["matrix_size"], m2["real_time_sec"], marker='s', label="Máquina 2")
plt.xlabel("Tamaño de matriz")
plt.ylabel("Tiempo real (s)")
plt.title("Comparación de Tiempo Real - OpenMP (8 hilos)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(graficas_path, "Comparacion_TiempoReal_8hilos.png"))
plt.close()
print("✅ Gráfica de Tiempo Real guardada.")

# === GRAFICA 2: TIEMPO TOTAL DE CPU ===
plt.figure(figsize=(8, 5))
plt.plot(m1["matrix_size"], m1["total_cpu_time_sec"], marker='o', label="Máquina 1 (Data)")
plt.plot(m2["matrix_size"], m2["total_cpu_time_sec"], marker='s', label="Máquina 2")
plt.xlabel("Tamaño de matriz")
plt.ylabel("Tiempo total de CPU (s)")
plt.title("Comparación de Tiempo Total de CPU - OpenMP (8 hilos)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(graficas_path, "Comparacion_CPUTime_8hilos.png"))
plt.close()
print("✅ Gráfica de Tiempo CPU guardada.")

# === GRAFICA 3: USO DE MEMORIA ===
plt.figure(figsize=(8, 5))
plt.plot(m1["matrix_size"], m1["memory_used_mb"], marker='o', label="Máquina 1 (Data)")
plt.plot(m2["matrix_size"], m2["memory_used_mb"], marker='s', label="Máquina 2")
plt.xlabel("Tamaño de matriz")
plt.ylabel("Memoria usada (MB)")
plt.title("Comparación de Uso de Memoria - OpenMP (8 hilos)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(graficas_path, "Comparacion_Memoria_8hilos.png"))
plt.close()
print("✅ Gráfica de Memoria guardada.")

print("\n🎯 ¡Gráficas generadas correctamente en 'results/graficas'!")
