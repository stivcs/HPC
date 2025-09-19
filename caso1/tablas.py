import pandas as pd
i = 24  # Número de hilos
# Cargar datos
df = pd.read_csv(f"Procesos_Data/tiempos_{i}procesos.csv")

# Como todos los num_threads son 2, los ignoramos
# Creamos un índice que represente la corrida (grupo)
df["Dimensiones"] = df.groupby("tamañoMatriz").cumcount() + 1

# Pivotamos la tabla
tabla = df.pivot(index="Dimensiones", columns="tamañoMatriz", values="real_time")

# Guardar a CSV
tabla.to_csv(f"{i}procesos.csv", index=True)

print(f"Tabla organizada guardada en '{i}procesos.csv'")
print(tabla.head())