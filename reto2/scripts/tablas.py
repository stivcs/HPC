import os
import pandas as pd

# ================== CONFIG ==================
RESULTS_DIR = "results/openmp"
TABLES_DIR = os.path.join("results", "tablas")
os.makedirs(TABLES_DIR, exist_ok=True)

def process_file(file_path, metodo):
    df = pd.read_csv(file_path)

    # Detectar columna de tiempos
    col_time = "real_time"

    # Obtener los hilos distintos
    threads_list = sorted(df["threads"].unique())
    print(f"\nProcesando {metodo}: {threads_list} hilos encontrados.")

    # Crear carpeta base para el método
    out_base = os.path.join(TABLES_DIR, "openmp", metodo)
    os.makedirs(out_base, exist_ok=True)

    # Generar un archivo separado por cada número de hilos
    for t in threads_list:
        df_thread = df[df["threads"] == t]

        # Agrupar por tamaño de problema (size)
        grouped = df_thread.groupby("size")[col_time].apply(list)

        # Convertir a tabla (cada size una columna)
        max_len = max(len(lst) for lst in grouped)
        data = {}
        for size, lst in grouped.items():
            data[size] = lst + [None] * (max_len - len(lst))

        table = pd.DataFrame(data)
        table.index = [f"iter{i+1}" for i in range(max_len)]

        # Crear carpeta destino por hilos
        out_dir = os.path.join(out_base, f"{t}_threads")
        os.makedirs(out_dir, exist_ok=True)

        # Nombre del archivo de salida
        out_name = f"tabla_openmp_{metodo.lower()}_{t}hilos.csv"
        out_path = os.path.join(out_dir, out_name)

        table.to_csv(out_path, index=True)
        print(f"✅ Guardado {out_path}")

# ================== ARCHIVOS A PROCESAR ==================
files = {
    "Dartboard": os.path.join(RESULTS_DIR, "Dartboard_Data", "OpenMP_Results.csv"),
    "Needles": os.path.join(RESULTS_DIR, "Needles_Data", "OpenMP_Results.csv"),
}

for metodo, path in files.items():
    if os.path.exists(path):
        process_file(path, metodo)
    else:
        print(f"⚠️ No se encontró el archivo: {path}")



