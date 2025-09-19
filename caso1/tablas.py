import pandas as pd

# Cargar datos
df = pd.read_csv("Hilos_Data/tiempos_2hilos.csv")

# Como todos los num_threads son 2, los ignoramos
# Creamos un índice que represente la corrida (grupo)
df["corrida"] = df.groupby("matrix_size").cumcount() + 1

# Pivotamos la tabla
tabla = df.pivot(index="corrida", columns="matrix_size", values="user_time")

# Guardar a CSV
tabla.to_csv("tabla_organizada.csv", index=True)

print("Tabla organizada guardada en 'tabla_organizada.csv'")
print(tabla.head())