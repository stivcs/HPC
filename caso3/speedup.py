import os
import pandas as pd
import matplotlib.pyplot as plt

TABLAS_DIR = "tablas"
OUTPUT_DIR = "graficas"

def main():
    archivos = sorted([f for f in os.listdir(TABLAS_DIR) if f.endswith(".csv")])

    # Verificación necesaria
    if "tabla_mul_1_procesos.csv" not in archivos:
        print("No existe 'tabla_mul_1_procesos.csv' en la carpeta 'tablas/'.")
        return

    # Leyendo la tabla base (1 proceso)
    base_df = pd.read_csv(os.path.join(TABLAS_DIR, "tabla_mul_1_procesos.csv"))
    tamanos = base_df.columns[1:]                     # columnas de tamaños
    base_prom = base_df[tamanos].mean()               # promedio por tamaño

    plt.figure(figsize=(10, 6))

    for archivo in archivos:
        if archivo == "tabla_mul_1_procesos.csv":
            continue  # no graficamos speedup contra sí mismo

        df = pd.read_csv(os.path.join(TABLAS_DIR, archivo))
        prom = df[tamanos].mean()

        # Calcular speedup
        speedup = base_prom / prom

        label = archivo.replace("tabla_", "").replace(".csv", "")
        plt.plot(tamanos, speedup, marker="o", label=label)

    # Configuración del gráfico
    plt.xlabel("Tamaño de matriz")
    plt.ylabel("Speedup (T1 / Tp)")
    plt.title("Speedup comparado contra 1 proceso")
    plt.legend()
    plt.grid(True)

    # Guardar
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    output_path = os.path.join(OUTPUT_DIR, "speedup_comparado.png")
    plt.savefig(output_path, dpi=300)

    print(f"✔ Gráfica guardada en: {output_path}")

if __name__ == "__main__":
    main()
