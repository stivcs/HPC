import os
import pandas as pd

RESULTS_DIR = "results"
TABLES_DIR = os.path.join(RESULTS_DIR, "tablas")
os.makedirs(TABLES_DIR, exist_ok=True)

def process_file(file_path, mode, metodo, file):
    df = pd.read_csv(file_path)

    # Detectar columna de tiempos
    if mode == "secuencial":
        col_time = "user_time"
    else:  # hilos o procesos
        col_time = "real_time"

    times = df[["N", col_time]]

    # Agrupamos por N y recolectamos todas las iteraciones
    grouped = times.groupby("N")[col_time].apply(list)

    # Convertir a DataFrame (cada N una columna)
    max_len = max(len(lst) for lst in grouped)
    data = {}
    for n, lst in grouped.items():
        data[n] = lst + [None] * (max_len - len(lst))

    table = pd.DataFrame(data)
    table.index = [f"iter{i+1}" for i in range(max_len)]

    # Carpeta destino
    out_dir = os.path.join(TABLES_DIR, metodo, mode)
    os.makedirs(out_dir, exist_ok=True)

    # Nombre de salida
    out_name = f"tabla_{mode}_{file}"
    out_path = os.path.join(out_dir, out_name)

    table.to_csv(out_path)
    print(f"Guardado {out_path}")

# Recorremos carpetas de resultados
for mode in ["secuencial", "hilos", "procesos"]:
    mode_dir = os.path.join(RESULTS_DIR, mode)
    if not os.path.isdir(mode_dir):
        continue

    for root, _, files in os.walk(mode_dir):
        for file in files:
            if file.endswith(".csv"):
                file_path = os.path.join(root, file)

                # Detectar método por el nombre de la carpeta o archivo
                if "Dartboard" in root or "dartboard" in file.lower():
                    metodo = "Dartboard"
                elif "Needles" in root or "needles" in file.lower():
                    metodo = "Needles"
                else:
                    metodo = "Otros"

                process_file(file_path, mode, metodo, file)
