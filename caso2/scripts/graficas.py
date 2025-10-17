import pandas as pd
import matplotlib.pyplot as plt
import os
import re

# === RUTAS BASE ===
base_path = "results"
tablas_path = os.path.join(base_path, "tablas")
graficas_path = os.path.join(base_path, "graficas")
os.makedirs(graficas_path, exist_ok=True)

# === DETECTAR ARCHIVOS ===
archivos = [f for f in os.listdir(tablas_path) if f.endswith(".csv")]

# Clasificación de archivos
maquina2_openmp = [f for f in archivos if "maquina2_OpenMP_hilos" in f]
data_openmp = [f for f in archivos if "OpenMp_Data_hilos" in f]
maquina2_secuencial = [f for f in archivos if "maquina2_Secuencial_tabla" in f]
data_secuencial = [f for f in archivos if "Secuencial_Data_tabla" in f]

def promedio_tiempos(df):
    """Promedia los tiempos de todas las iteraciones."""
    df_mean = df.mean(numeric_only=True)
    df_mean = df_mean.drop("iteracion", errors="ignore")
    return df_mean

def extraer_hilos(nombre):
    """Extrae el número de hilos del nombre de archivo."""
    match = re.search(r"hilos_(\d+)", nombre)
    return int(match.group(1)) if match else None

def cargar_datos(lista_archivos):
    """Carga los datos de cada archivo de OpenMP agrupados por hilos."""
    data = {}
    for file in lista_archivos:
        path = os.path.join(tablas_path, file)
        hilos = extraer_hilos(file)
        if hilos:
            df = pd.read_csv(path)
            data[hilos] = promedio_tiempos(df)
    return data

# === Cargar datos ===
maquina2_omp = cargar_datos(maquina2_openmp)
data_omp = cargar_datos(data_openmp)

# Secuenciales
maquina2_seq = promedio_tiempos(pd.read_csv(os.path.join(tablas_path, maquina2_secuencial[0]))) if maquina2_secuencial else None
data_seq = promedio_tiempos(pd.read_csv(os.path.join(tablas_path, data_secuencial[0]))) if data_secuencial else None

# === 1️⃣ Gráfica Secuencial vs OpenMP (todas las curvas juntas por máquina) ===
for machine, seq, omp_data in [("maquina2", maquina2_seq, maquina2_omp), ("Data", data_seq, data_omp)]:
    if seq is None or not omp_data:
        continue

    plt.figure(figsize=(9, 5))
    plt.plot(seq.index, seq.values, 'o-', label="Secuencial", linewidth=2, color="black")

    for hilos, valores in sorted(omp_data.items()):
        plt.plot(valores.index, valores.values, '--', marker='s', label=f"OpenMP ({hilos} hilos)")

    plt.title(f"Comparación Secuencial vs OpenMP ({machine})")
    plt.xlabel("Tamaño de matriz")
    plt.ylabel("Tiempo real promedio (s)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    out_path = os.path.join(graficas_path, f"{machine}_Comparacion_TodosHilos.png")
    plt.savefig(out_path)
    plt.close()
    print(f"✅ Gráfica creada: {out_path}")

# === 2️⃣ Gráfica de Speedup (todas las curvas por máquina) ===
for machine, seq, omp_data in [("maquina2", maquina2_seq, maquina2_omp), ("Data", data_seq, data_omp)]:
    if seq is None or not omp_data:
        continue

    plt.figure(figsize=(9, 5))
    for hilos, valores in sorted(omp_data.items()):
        speedup = seq / valores
        plt.plot(speedup.index, speedup.values, marker='o', label=f"{hilos} hilos")

    plt.title(f"Speedup OpenMP vs Secuencial ({machine})")
    plt.xlabel("Tamaño de matriz")
    plt.ylabel("Speedup (Tseq / Topenmp)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    out_path = os.path.join(graficas_path, f"{machine}_Speedup_TodosHilos.png")
    plt.savefig(out_path)
    plt.close()
    print(f"✅ Gráfica de speedup creada: {out_path}")

# === 3️⃣ Comparación entre máquinas (todos los hilos en una sola gráfica) ===
hilos_comunes = sorted(set(maquina2_omp.keys()).intersection(set(data_omp.keys())))

plt.figure(figsize=(10, 6))
for h in hilos_comunes:
    plt.plot(maquina2_omp[h].index, maquina2_omp[h].values, 'o-', label=f"Máquina 2 - {h} hilos")
    plt.plot(data_omp[h].index, data_omp[h].values, 's--', label=f"Máquina 1 - {h} hilos")

plt.title("Comparación entre máquinas (todos los hilos)")
plt.xlabel("Tamaño de matriz")
plt.ylabel("Tiempo real promedio (s)")
plt.legend()
plt.grid(True)
plt.tight_layout()
out_path = os.path.join(graficas_path, "Comparacion_Maquinas_TodosHilos.png")
plt.savefig(out_path)
plt.close()
print(f"✅ Gráfica comparativa con todos los hilos creada: {out_path}")

print("\n🎯 ¡Gráficas consolidadas correctamente en 'results/graficas'!")

