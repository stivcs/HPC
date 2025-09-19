import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

# === Rutas ===
secuencial_path = "Secuencial_Data/Secuencial_Results.csv"
hilos_path = "Hilos_Data"
procesos_path = "Procesos_Data"

# === Cargar secuencial ===
seq_df = pd.read_csv(secuencial_path)  # columnas: size,user_time
seq_avg = seq_df.groupby("size")["user_time"].mean().reset_index()

# === Cargar hilos ===
hilos_files = glob.glob(os.path.join(hilos_path, "tiempos_*hilos.csv"))
all_hilos = []
for file in hilos_files:
    df = pd.read_csv(file)  # columnas: matrix_size,num_threads,real_time
    all_hilos.append(df)
hilos_df = pd.concat(all_hilos, ignore_index=True)
hilos_avg = hilos_df.groupby(["matrix_size", "num_threads"])["real_time"].mean().reset_index()

# === Cargar procesos ===
procesos_files = glob.glob(os.path.join(procesos_path, "tiempos_*procesos.csv"))
all_procesos = []
for file in procesos_files:
    df = pd.read_csv(file)  # columnas: tamañoMatriz,procesos,real_time
    all_procesos.append(df)
procesos_df = pd.concat(all_procesos, ignore_index=True)
procesos_avg = procesos_df.groupby(["tamañoMatriz", "procesos"])["real_time"].mean().reset_index()

# === Graficar Tiempos ===
plt.figure(figsize=(12,7))

# Secuencial
plt.plot(seq_avg["size"], seq_avg["user_time"], marker="o", linewidth=2, label="Secuencial")

# Hilos
for n_hilos in sorted(hilos_avg["num_threads"].unique()):
    subset = hilos_avg[hilos_avg["num_threads"] == n_hilos]
    plt.plot(subset["matrix_size"], subset["real_time"], marker="o", linestyle="--",
             label=f"{n_hilos} hilos")

# Procesos
for n_procesos in sorted(procesos_avg["procesos"].unique()):
    subset = procesos_avg[procesos_avg["procesos"] == n_procesos]
    label = f"{n_procesos} proceso" if n_procesos == 1 else f"{n_procesos} procesos"
    plt.plot(subset["tamañoMatriz"], subset["real_time"], marker="s", linestyle="-",
             label=label)

plt.xlabel("Tamaño de matriz")
plt.ylabel("Tiempo de ejecución promedio (s)")
plt.title("Comparación de tiempos: Secuencial vs Hilos vs Procesos")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.show()
