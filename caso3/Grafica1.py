import os
import pandas as pd
import matplotlib.pyplot as plt

TABLAS_DIR = "tablas"
OUTPUT_DIR = "graficas"

def main():
    archivos = [f for f in os.listdir(TABLAS_DIR) if f.endswith(".csv")]

    if not archivos:
        print("No se encontraron archivos en 'tablas/'.")
        return

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    plt.figure(figsize=(10, 6))

    for archivo in archivos:
        path = os.path.join(TABLAS_DIR, archivo)
        df = pd.read_csv(path)

        # columnas excepto "iteracion"
        tamanos = df.columns[1:]

        # promedios por tamaño
        promedios = df[tamanos].mean()

        # nombre bonito para la gráfica
        label = archivo.replace("tabla_", "").replace(".csv", "")

        # Graficar
        plt.plot(tamanos, promedios, marker="o", label=label)

    plt.xlabel("Tamaño de matriz")
    plt.ylabel("Tiempo promedio (s)")
    plt.title("Comparación de tiempos por número de procesos")
    plt.legend()
    plt.grid(True)

    output_path = os.path.join(OUTPUT_DIR, "tiempos_comparados.png")
    plt.savefig(output_path, dpi=300)

    print(f"✔ Gráfica guardada en: {output_path}")


if __name__ == "__main__":
    main()
