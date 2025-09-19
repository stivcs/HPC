import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

# === Rutas ===
procesos_path = "Procesos_Data"
secuencial_path = "Secuencial_Data/Secuencial_Results.csv"

# === Cargar secuencial ===
# Formato esperado: size,user_time
seq_df = pd.read_csv(secuencial_path)
seq_avg = seq_df.groupby("size")["user_time"].mean().reset_index()

# === Cargar procesos ===
procesos_files = glob.glob(os.path.join(procesos_path, "tiempos_*procesos.csv"))
all_procesos = []

for file in procesos_files:
    df = pd.read_csv(file)  # formato: tamañoMatriz,procesos,real_time
    all_procesos.append(df)

procesos_df = pd.concat(all_procesos, ignore_index=True)

# Promedios
procesos_avg = procesos_df.groupby(["tamañoMatriz", "procesos"])["real_time"].mean().reset_index()

# === Calcular Speedup ===
speedup_data = procesos_avg.merge(seq_avg, left_on="tamañoMatriz", right_on="size")
speedup_data["speedup"] = speedup_data["user_time"] / speedup_data["real_time"]

# === Graficar Speedup ===
plt.figure(figsize=(10,6))

for n_procesos in sorted(speedup_data["procesos"].unique()):
    subset = speedup_data[speedup_data["procesos"] == n_procesos]
    label = f"{n_procesos} proceso" if n_procesos == 1 else f"{n_procesos} procesos"
    plt.plot(subset["tamañoMatriz"], subset["speedup"], marker="o", label=label)

plt.xlabel("Tamaño de matriz")
plt.ylabel("Speedup (T_secuencial / T_paralelo)")
plt.title("Speedup en función del tamaño de matriz (Procesos)")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.show()

