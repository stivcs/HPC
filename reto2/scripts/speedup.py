import os
import pandas as pd
import matplotlib.pyplot as plt

BASE_DIR = "results/tablas"
OUTPUT_DIR = os.path.join(BASE_DIR, "graficas_speedup")
os.makedirs(OUTPUT_DIR, exist_ok=True)

def cargar_tablas(path, modo):
    """Carga tablas desde una carpeta dada (hilos, procesos, secuencial, openmp)."""
    data = {}
    if not os.path.isdir(path):
        print(f"Info: No se encontró la carpeta {path}")
        return data

    # OpenMP tiene una estructura de carpetas diferente (ej: path/8_threads/tabla.csv)
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
    
    # LÓGICA ANTIGUA (Hilos, Procesos, Secuencial)
    for file in os.listdir(path):
        if not file.endswith(".csv"):
            continue
        
        file_path = os.path.join(path, file)
        df = pd.read_csv(file_path, index_col=0)  
        
        workers = None
        if modo == "hilos":
            workers = file.split("_")[-1].replace("hilos.csv", "")
        elif modo == "procesos":
            workers = file.split("_")[-1].replace("procesos.csv", "")
        
        for n in df.columns:
            prom = df[n].mean(skipna=True)
            data.setdefault(workers if workers else "seq", {})[int(n)] = prom
    
    return data

def graficar_speedup(metodo, data_seq, data_otro, modo_otro):
    if "seq" not in data_seq or not data_seq["seq"]:
        print(f"⚠ No hay datos secuenciales válidos para {metodo}")
        return

    plt.figure(figsize=(10,6))

    T_seq = data_seq["seq"]

    try:
        workers_ordenados = sorted(data_otro.keys(), key=int)
    except ValueError:
        workers_ordenados = sorted(data_otro.keys()) 

    for workers in workers_ordenados:
        if workers == 'seq': continue 
        
        valores = data_otro[workers]
        Ns = sorted(valores.keys())
        speedups = []
        Ns_validos = [] 

        for n in Ns:
            if n in T_seq and T_seq[n] > 0 and n in valores and valores[n] > 0:
                speedups.append(T_seq[n] / valores[n])
                Ns_validos.append(n)
            
        label_modo = modo_otro
        if modo_otro == "openmp":
            label_modo = "threads (OpenMP)"
        
        if Ns_validos: 
            plt.plot(Ns_validos, speedups, marker="o", linestyle="--", label=f"{workers} {label_modo}")

    plt.xlabel("Tamaño N (Problema)")
    plt.ylabel("Speedup (T_sec / T_par)")
    plt.title(f"{metodo}: Speedup {modo_otro.capitalize()} vs Secuencial")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.xscale('log') 
    plt.tight_layout()

    out_file = os.path.join(OUTPUT_DIR, f"{metodo.lower()}_speedup_{modo_otro}.png")
    plt.savefig(out_file, dpi=300)
    plt.close()
    print(f"✅ Guardado {out_file}")

# ================== MAIN ==================
for metodo in ["Dartboard", "Needles"]:
    # 'base_metodo' se usa para secuencial, hilos y procesos
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
        graficar_speedup(metodo, data_seq, data_hilos, "hilos")
    else:
        print(f"ℹ No hay datos de hilos para {metodo}")

    # Cargar procesos
    path_procesos = os.path.join(base_metodo, "procesos")
    data_procesos = cargar_tablas(path_procesos, "procesos")
    if data_procesos:
        graficar_speedup(metodo, data_seq, data_procesos, "procesos")
    else:
        print(f"ℹ No hay datos de procesos para {metodo}")

    # --- INICIO DE LA MODIFICACIÓN ---
    # Cargar OpenMP (con la ruta corregida)
    # La ruta es: results/tablas/openmp/[metodo]/
    path_openmp = os.path.join(BASE_DIR, "openmp", metodo)
    data_openmp = cargar_tablas(path_openmp, "openmp")
    # --- FIN DE LA MODIFICACIÓN ---
    
    if data_openmp:
        graficar_speedup(metodo, data_seq, data_openmp, "openmp")
    else:
        print(f"ℹ No hay datos de openmp para {metodo}")