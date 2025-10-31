import os
import pandas as pd
import matplotlib.pyplot as plt

BASE_DIR = "results/tablas"
OUTPUT_DIR = os.path.join(BASE_DIR, "graficas")
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
        # Convertimos cada columna (N) en promedio
        for n in df.columns:
            prom = df[n].mean(skipna=True)
            workers = None
            if modo == "hilos":
                workers = file.split("_")[-1].replace("hilos.csv", "")
            elif modo == "procesos":
                workers = file.split("_")[-1].replace("procesos.csv", "")
            data.setdefault(workers if workers else "seq", {})[int(n)] = prom
    return data

def graficar_comparacion(metodo, data_seq, data_otro, modo_otro):
    plt.figure(figsize=(10,6))

    # Secuencial (solo una línea)
    if "seq" in data_seq:
        Ns = sorted(data_seq["seq"].keys())
        tiempos = [data_seq["seq"][n] for n in Ns]
        plt.plot(Ns, tiempos, marker="o", linewidth=2, label="Secuencial")

    # Otro modo (hilos o procesos)
    for workers, valores in data_otro.items():
        Ns = sorted(valores.keys())
        tiempos = [valores[n] for n in Ns]
        plt.plot(Ns, tiempos, marker="s", linestyle="--", label=f"{workers} {modo_otro}")

    plt.xlabel("Tamaño N")
    plt.ylabel("Tiempo promedio (s)")
    plt.title(f"{metodo}: Secuencial vs {modo_otro.capitalize()}")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.tight_layout()

    out_file = os.path.join(OUTPUT_DIR, f"{metodo.lower()}_sec_vs_{modo_otro}.png")
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
        graficar_comparacion(metodo, data_seq, data_hilos, "hilos")

    # Cargar procesos
    data_procesos = cargar_tablas(os.path.join(base_metodo, "procesos"), "procesos")
    if data_procesos:
        graficar_comparacion(metodo, data_seq, data_procesos, "procesos")
