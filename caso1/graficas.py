import pandas as pd
import matplotlib.pyplot as plt

# Cargar el CSV (ajusta el nombre si tu archivo se llama distinto)
df = pd.read_csv("Secuencial_Data/Secuencial_Results.csv")

# Agrupar por tamaño de matriz y calcular el promedio del tiempo
df_grouped = df.groupby("size")["user_time"].mean().reset_index()

# Graficar
plt.figure(figsize=(10,6))
plt.plot(df_grouped["size"], df_grouped["user_time"], marker="o", linestyle="-", label="Tiempo promedio")

# Opcional: mostrar también los puntos individuales
plt.scatter(df["size"], df["user_time"], alpha=0.4, color="red", label="Valores individuales")

plt.title("Tamaño de la matriz vs Tiempo de ejecución")
plt.xlabel("Tamaño de la matriz")
plt.ylabel("Tiempo de usuario (s)")
plt.grid(True, linestyle="--", alpha=0.6)
plt.legend()
plt.tight_layout()
plt.show()