import os
import pandas as pd
import matplotlib.pyplot as plt

BASE_DIR = "results/tablas"
OUTPUT_DIR = os.path.join(BASE_DIR, "graficas") # Tu carpeta original
os.makedirs(OUTPUT_DIR, exist_ok=True)

def cargar_tablas(path, modo):
    """Carga tablas desde una carpeta dada (hilos, procesos, secuencial, openmp)."""
    data = {}
    if not os.path.isdir(path):
        print(f"Info: No se encontró la carpeta {path}")
        return data

    # Lógica para OpenMP (estructura de subcarpetas)
    if modo == "openmp":
        for sub_dir_name in os.listdir(path):
            sub_dir_path = os.path.join(path, sub_dir_name)
            
            if not os.path.isdir(sub_dir_path):
                continue
            
            workers = None
            try:
                workers = str(int(sub_dir_name.split('_')[0]))
            except (ValueError, IndexError):
                continue 

            csv_file_name = None
            for f in os.listdir(sub_dir_path):
                if f.endswith(".csv"):
                    csv_file_name = f
                    break 
            
            if csv_file_name is None:
                continue 

            file_path = os.path.join(sub_dir_path, csv_file_name)
            try:
                df = pd.read_csv(file_path, index_col=0)  # iteraciones como índice
                for n in df.columns:
                    prom = df[n].mean(skipna=True)
                    data.setdefault(workers, {})[int(n)] = prom
            except Exception as e:
                print(f"Error procesando {file_path}: {e}")
        
        return data
    
    # Lógica para Hilos, Procesos, Secuencial (archivos en la carpeta)
    for file in os.listdir(path):
        if not file.endswith(".csv"):
            continue
        file_path = os.path.join(path, file)
        df = pd.read_csv(file_path, index_col=0)  
        
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
    """
    Esta es tu función original, sin modificaciones en los ejes.
    Dejará que Matplotlib elija el formato (ej. 1e9 con 0.2, 0.4...).
    """
    plt.figure(figsize=(10,6))

    # Secuencial (solo una línea)
    if "seq" in data_seq and data_seq["seq"]:
        Ns = sorted(data_seq["seq"].keys())
        tiempos = [data_seq["seq"][n] for n in Ns]
        plt.plot(Ns, tiempos, marker="o", linewidth=2, label="Secuencial", zorder=10)

    # Otro modo (hilos, procesos, u openmp)
    try:
        workers_ordenados = sorted(data_otro.keys(), key=int)
    except ValueError:
        workers_ordenados = sorted(data_otro.keys())

    for workers in workers_ordenados:
        valores = data_otro[workers]
        if not valores: continue
        
        Ns = sorted(valores.keys())
        tiempos = [valores[n] for n in Ns]
        
        label_modo = modo_otro
        if modo_otro == "openmp":
            label_modo = "threads (OpenMP)"

        plt.plot(Ns, tiempos, marker="s", linestyle="--", label=f"{workers} {label_modo}")

    plt.xlabel("Tamaño N")
    plt.ylabel("Tiempo promedio (s)")
    plt.title(f"{metodo}: Secuencial vs {modo_otro.capitalize()}")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.6)
    
    # --- NO HAY CÓDIGO DE FORMATO DE EJES AQUÍ ---
    # Esto permite que matplotlib decida automáticamente.
    
    plt.tight_layout()

    out_file = os.path.join(OUTPUT_DIR, f"{metodo.lower()}_sec_vs_{modo_otro}.png")
    plt.savefig(out_file, dpi=300)
    plt.close()
    print(f"✅ Guardado {out_file}")

# ================== MAIN ==================
for metodo in ["Dartboard", "Needles"]:
    base_metodo = os.path.join(BASE_DIR, metodo)
    print(f"\n--- Procesando {metodo} ---")

    # Cargar secuencial
    path_seq = os.path.join(base_metodo, "secuencial")
    data_seq = cargar_tablas(path_seq, "secuencial")
    if not data_seq:
        print(f"⚠ No se encontraron datos secuenciales para {metodo}, saltando...")
        continue

    # Cargar hilos
    path_hilos = os.path.join(base_metodo, "hilos")
    data_hilos = cargar_tablas(path_hilos, "hilos")
    if data_hilos:
        graficar_comparacion(metodo, data_seq, data_hilos, "hilos")

    # Cargar procesos
    path_procesos = os.path.join(base_metodo, "procesos")
    data_procesos = cargar_tablas(path_procesos, "procesos")
    if data_procesos:
        graficar_comparacion(metodo, data_seq, data_procesos, "procesos")
        
    # Cargar OpenMP
    path_openmp = os.path.join(BASE_DIR, "openmp", metodo)
    data_openmp = cargar_tablas(path_openmp, "openmp")
    
    if data_openmp:
        graficar_comparacion(metodo, data_seq, data_openmp, "openmp")
    else:
        print(f"ℹ No hay datos de openmp para {metodo}")