import pandas as pd
import os

# === RUTAS BASE ===
base_path = "results"
perfil_path = os.path.join(base_path, "perfilamiento")
os.makedirs(perfil_path, exist_ok=True)

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

        # === Agrupar por hilos y tamaño de matriz ===
        for threads, grupo_hilos in df.groupby("threads"):
            resumen = grupo_hilos.groupby("matrix_size").mean(numeric_only=True).reset_index()

            # Guardar
            output_path = os.path.join(perfil_path, f"{name}_perfilamiento_{threads}hilos.csv")
            resumen.to_csv(output_path, index=False)
            print(f"✅ Tabla de perfilamiento OpenMP creada: {output_path}")

    else:  # Secuencial
        columnas_interes = [
            "matrix_size", "real_time_sec", "user_time_sec",
            "system_time_sec", "total_cpu_time_sec",
            "memory_used_mb", "performance_gops", "elements_per_second_million"
        ]
        df = df[columnas_interes]

        resumen = df.groupby("matrix_size").mean(numeric_only=True).reset_index()

        # Guardar
        output_path = os.path.join(perfil_path, f"{name}_perfilamiento.csv")
        resumen.to_csv(output_path, index=False)
        print(f"✅ Tabla de perfilamiento Secuencial creada: {output_path}")

print("\n🎯 Tablas de perfilamiento generadas correctamente en 'results/perfilamiento'")
