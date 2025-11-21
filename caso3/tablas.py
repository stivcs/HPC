import os
import pandas as pd

# Directorios
RESULTS_DIR = "results"
OUTPUT_DIR = "tablas"

def procesar_archivo(path_csv, output_name):
    df = pd.read_csv(path_csv)

    # Cada tamaño aparece 10 veces -> dividimos en bloques
    tamanos = sorted(df["tamaño"].unique())
    iteraciones = len(df) // len(tamanos)

    # Crear estructura: una fila por iteración
    tabla = {"iteracion": list(range(iteraciones))}

    for tam in tamanos:
        tiempos = df[df["tamaño"] == tam]["tiempo"].reset_index(drop=True)
        tabla[str(tam)] = tiempos

    # Convertimos a dataframe final
    df_final = pd.DataFrame(tabla)

    # Guardamos
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    output_path = os.path.join(OUTPUT_DIR, output_name)
    df_final.to_csv(output_path, index=False)

    print(f"✔ Archivo generado: {output_path}")


def main():
    # Archivos dentro de /results
    archivos = [f for f in os.listdir(RESULTS_DIR) if f.endswith(".csv")]

    for archivo in archivos:
        input_path = os.path.join(RESULTS_DIR, archivo)

        # Nombre del archivo de salida
        base = archivo.replace(".csv", "")
        output_name = f"tabla_{base}.csv"

        procesar_archivo(input_path, output_name)


if __name__ == "__main__":
    main()
