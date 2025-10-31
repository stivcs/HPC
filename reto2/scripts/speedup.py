import os
import pandas as pd
import matplotlib.pyplot as plt

BASE_DIR = "results/tablas"
OUTPUT_DIR = os.path.join(BASE_DIR, "graficas_speedup")
os.makedirs(OUTPUT_DIR, exist_ok=True)

def cargar_tablas(path, modo):
    """Carga tablas desde una carpeta dada (hilos, procesos, secuencial)."""
    data = {}
    if not os.path.isdir(path):
        return data

    for file in os.listdir(path):
        if not file.endswith(".csv"):
            continue
        file_path = os.path.join(path, file)
        df = pd.read_csv(file_path, index_col=0)  # iteraciones como índice
        for n in df.columns:
            prom = df[n].mean(skipna=True)
            workers = None
            if modo == "hilos":
                workers = file.split("_")[-1].replace("hilos.csv", "")
            elif modo == "procesos":
                workers = file.split("_")[-1].replace("procesos.csv", "")
            data.setdefault(workers if workers else "seq", {})[int(n)] = prom
    return data

def graficar_speedup(metodo, data_seq, data_otro, modo_otro):
    if "seq" not in data_seq:
        print(f"⚠ No hay datos secuenciales para {metodo}")
        return

    plt.figure(figsize=(10,6))

    T_seq = data_seq["seq"]

    for workers, valores in data_otro.items():
        Ns = sorted(valores.keys())
        speedups = []
        for n in Ns:
            if n in T_seq:
                speedups.append(T_seq[n] / valores[n])
            else:
                speedups.append(None)
        plt.plot(Ns, speedups, marker="o", linestyle="--", label=f"{workers} {modo_otro}")

    plt.xlabel("Tamaño N")
    plt.ylabel("Speedup")
    plt.title(f"{metodo}: Speedup {modo_otro.capitalize()} vs Secuencial")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.tight_layout()

    out_file = os.path.join(OUTPUT_DIR, f"{metodo.lower()}_speedup_{modo_otro}.png")
    plt.savefig(out_file, dpi=300)
    plt.close()
    print(f"✅ Guardado {out_file}")

# ================== MAIN ==================
for metodo in ["Dartboard", "Needles"]:
    base_metodo = os.path.join(BASE_DIR, metodo)

    # Cargar secuencial
    data_seq = cargar_tablas(os.path.join(base_metodo, "secuencial"), "secuencial")

    # Cargar hilos
    data_hilos = cargar_tablas(os.path.join(base_metodo, "hilos"), "hilos")
    if data_hilos:
        graficar_speedup(metodo, data_seq, data_hilos, "hilos")

    # Cargar procesos
    data_procesos = cargar_tablas(os.path.join(base_metodo, "procesos"), "procesos")
    if data_procesos:
        graficar_speedup(metodo, data_seq, data_procesos, "procesos")