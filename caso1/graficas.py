import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

# === Rutas ===
procesos_path = "Procesos_Data"   # carpeta donde guardas tus CSV de procesos
secuencial_path = "Secuencial_Data/Secuencial_Results.csv"

# === Cargar secuencial ===
seq_df = pd.read_csv(secuencial_path)  # formato: size,user_time
seq_avg = seq_df.groupby("size")["user_time"].mean().reset_index()

# === Cargar procesos ===
procesos_files = glob.glob(os.path.join(procesos_path, "tiempos_*procesos.csv"))
# si solo tienes un archivo como "Procesos.csv", cambia a:
# procesos_files = ["Procesos_Data/Procesos.csv"]

all_procesos = []

for file in procesos_files:
    df = pd.read_csv(file)  # formato: tamañoMatriz,procesos,real_time
    df.rename(columns={"tamañoMatriz": "matrix_size"}, inplace=True)
    all_procesos.append(df)

procesos_df = pd.concat(all_procesos, ignore_index=True)

# === Promediar por tamaño de matriz y #procesos ===
procesos_avg = procesos_df.groupby(["matrix_size", "procesos"])["real_time"].mean().reset_index()

# === Graficar ===
plt.figure(figsize=(10,6))

# Secuencial
plt.plot(seq_avg["size"], seq_avg["user_time"], marker="o", linewidth=2, label="Secuencial")

# Procesos
for n_proc in sorted(procesos_avg["procesos"].unique()):
    subset = procesos_avg[procesos_avg["procesos"] == n_proc]
    plt.plot(subset["matrix_size"], subset["real_time"], marker="o", label=f"{int(n_proc)} procesos")

plt.xlabel("Tamaño de matriz")
plt.ylabel("Tiempo promedio (s)")
plt.title("Comparación Secuencial vs Procesos")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.show()