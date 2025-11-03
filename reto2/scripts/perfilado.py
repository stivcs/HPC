import pandas as pd
import os

# === RUTAS BASE ===
base_path = "results"
perfil_path = os.path.join(base_path, "perfilamiento")
os.makedirs(perfil_path, exist_ok=True)

# === ARCHIVOS DE ENTRADA ===
files = {
    "OpenMP_Dartboard": os.path.join(base_path, "openmp", "Dartboard_Data", "OpenMP_Results.csv"),
    "OpenMP_Needles": os.path.join(base_path, "openmp", "Needles_Data", "OpenMP_Results.csv"),
}

# === PROCESAMIENTO DE CADA ARCHIVO ===
for name, path in files.items():
    if not os.path.exists(path):
        print(f"⚠️ No se encontró el archivo: {path}")
        continue

    df = pd.read_csv(path)

    # === Detectar que el formato sea válido ===
    if "size" not in df.columns or "threads" not in df.columns:
        print(f"⚠️ El archivo {path} no tiene columnas esperadas (size, threads).")
        continue

    # === Renombrar columnas ===
    df = df.rename(columns={
        "size": "matrix_size",
        "real_time": "real_time_sec",
        "user_time": "user_time_sec",
        "system_time": "system_time_sec",
        "total_cpu_time": "total_cpu_time_sec",
        "gops": "performance_gops",
        "elements_per_second_millions": "elements_per_second_million"
    })

    columnas_interes = [
        "matrix_size", "threads", "real_time_sec", "user_time_sec",
        "system_time_sec", "total_cpu_time_sec",
        "memory_used_mb", "performance_gops", "elements_per_second_million"
    ]

    df = df[columnas_interes]

    # === Agrupar por número de hilos y tamaño de problema ===
    for threads, grupo_hilos in df.groupby("threads"):
        resumen = grupo_hilos.groupby("matrix_size").mean(numeric_only=True).reset_index()

        # === Guardar tabla de perfilamiento ===
        output_path = os.path.join(perfil_path, f"{name}_perfilamiento_{threads}hilos.csv")
        resumen.to_csv(output_path, index=False)
        print(f"✅ Tabla de perfilamiento OpenMP creada: {output_path}")

print("\n🎯 Tablas de perfilamiento generadas correctamente en 'results/perfilamiento'")
