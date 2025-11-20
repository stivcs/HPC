import subprocess

# Configuración
matrix_sizes = [911, 1229, 1658, 2039, 3023, 4081, 5510]
process_counts = [2, 4, 8, 12, 16]  # Ajusta según tus nodos disponibles
hosts = "wn1,wn2,wn3"  # Ajusta a tus hosts
repetitions = 10
executable = "./mul_mat"

for n_procs in process_counts:
    for rep in range(1, repetitions + 1):
        for size in matrix_sizes:
            print(f"Ejecutando {executable} con {n_procs} procesos, tamaño {size}, repetición {rep}")
            cmd = [
                "mpiexec",
                "-n", str(n_procs),
                "-host", hosts,
                "-oversubscribe",
                executable,
                str(size)
            ]
            try:
                subprocess.run(cmd, check=True)
            except subprocess.CalledProcessError as e:
                print(f"Error en ejecución: {e}")
