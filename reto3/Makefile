# Compiladores
CC = gcc
MPICC = mpicc

# Flags
CFLAGS = -O2
MPIFLAGS = -O2

# Ejecutables
SERIAL = traffic_serial
MPIEXEC = traffic_mpi

# Hosts del cluster
HOSTS = wn1,wn2,wn3

# Parámetros por defecto
N = 10000
STEPS = 5000
DENSITY = 0.3
PRINTFREQ = 50

# Rutas de salida
OUTDIR = results
SERIAL_OUT = $(OUTDIR)/serial.csv
MPI_OUT = $(OUTDIR)/mpi.csv

all: dirs $(SERIAL) $(MPIEXEC)

dirs:
	mkdir -p $(OUTDIR)

$(SERIAL): src/traffic_serial.c
	$(CC) $(CFLAGS) $< -o $(SERIAL)

$(MPIEXEC): src/traffic_mpi.c
	$(MPICC) $(MPIFLAGS) $< -o $(MPIEXEC)

# -----------------------
#   EJECUCIONES
# -----------------------

run-serial:
	./$(SERIAL) $(N) $(STEPS) $(DENSITY) $(PRINTFREQ) $(SERIAL_OUT)

run-mpi:
	mpiexec -n 7 -host $(HOSTS) -oversubscribe ./$(MPIEXEC) \
		$(N) $(STEPS) $(DENSITY) $(PRINTFREQ) $(MPI_OUT)

# -----------------------
#   TEST AUTOMÁTICO
# -----------------------

test:
	python3 scripts/test.py

# -----------------------
#   LIMPIEZA
# -----------------------

clean:
	rm -f $(SERIAL) $(MPIEXEC)
	rm -rf $(OUTDIR)

.PHONY: all dirs run-serial run-mpi test clean
