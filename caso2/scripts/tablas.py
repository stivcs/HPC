import pandas as pd
import os

# === RUTAS BASE ===
base_path = "results"
tablas_path = os.path.join(base_path, "tablas")
os.makedirs(tablas_path, exist_ok=True)

# === ARCHIVOS DE ENTRADA ===
files = {
    "maquina2_OpenMP": os.path.join(base_path, "maquina2", "OpenMP_Results.csv"),
    "maquina2_Secuencial": os.path.join(base_path, "maquina2", "Secuencial_Results.csv"),
    "OpenMp_Data": os.path.join(base_path, "OpenMp_Data", "OpenMP_Results.csv"),
    "Secuencial_Data": os.path.join(base_path, "Secuencial_Data", "Secuencial_Results.csv")
}

# === PROCESAMIENTO DE CADA ARCHIVO ===
for name, path in files.items():
    if not os.path.exists(path):
        print(f"⚠️ No se encontró el archivo: {path}")
        continue

    df = pd.read_csv(path)

    # Detectar si es OpenMP o Secuencial
    if "size" in df.columns:  # formato OpenMP
        tipo = "openmp"
        df = df[["size", "threads", "real_time"]].rename(columns={
            "size": "matrix_size",
            "real_time": "real_time_sec"
        })
    else:
        tipo = "secuencial"
        df = df[["matrix_size", "real_time_sec"]]

    # Ordenar
    df = df.sort_values(by="matrix_size")

    if tipo == "openmp":
        # Agrupar por cantidad de hilos
        for threads, grupo_hilos in df.groupby("threads"):
            grupos = grupo_hilos.groupby("matrix_size")

            # Crear tabla final
            resultados = pd.DataFrame()
            sizes = sorted(grupos.groups.keys())
            max_iters = max(len(grupos.get_group(size)) for size in sizes)
            resultados["iteracion"] = range(1, max_iters + 1)

            for size in sizes:
                tiempos = grupos.get_group(size)["real_time_sec"].reset_index(drop=True)
                while len(tiempos) < max_iters:
                    tiempos.loc[len(tiempos)] = None
                resultados[str(size)] = tiempos

            # Guardar una tabla por cantidad de hilos
            output_path = os.path.join(tablas_path, f"{name}_hilos_{threads}.csv")
            resultados.to_csv(output_path, index=False)
            print(f"✅ Tabla OpenMP creada: {output_path}")

    else:
        # Caso secuencial normal
        grupos = df.groupby("matrix_size")
        resultados = pd.DataFrame()
        sizes = sorted(grupos.groups.keys())
        max_iters = max(len(grupos.get_group(size)) for size in sizes)
        resultados["iteracion"] = range(1, max_iters + 1)

        for size in sizes:
            tiempos = grupos.get_group(size)["real_time_sec"].reset_index(drop=True)
            while len(tiempos) < max_iters:
                tiempos.loc[len(tiempos)] = None
            resultados[str(size)] = tiempos

        output_path = os.path.join(tablas_path, f"{name}_tabla.csv")
        resultados.to_csv(output_path, index=False)
        print(f"✅ Tabla Secuencial creada: {output_path}")

print("\n🎯 Tablas generadas correctamente en 'results/tablas'")

