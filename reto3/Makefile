# Ejecutables
SERIAL = traffic_serial
MPI = traffic_mpi

# Compilador C normal y MPI
CC = gcc
MPICC = mpicc

CFLAGS = -O2

# Hosts
HOSTS = wn1,wn2,wn3

all: $(SERIAL) $(MPI)

$(SERIAL): src/traffic_serial.c
	$(CC) $(CFLAGS) $< -o src/$(SERIAL)

$(MPI): src/traffic_mpi.c
	$(MPICC) $(CFLAGS) $< -o src/$(MPI)

run_serial:
	./src/$(SERIAL) 1000 100 0.3 50 results/test_serial.csv

run_mpi:
	mpiexec -n 7 -host $(HOSTS) -oversubscribe ./src/$(MPI) 1000 100 0.3 50 results/test_mpi.csv

test:
	python3 test.py

clean:
	rm -f src/$(SERIAL) src/$(MPI)
	rm -f results/*.csv

.PHONY: all run_serial run_mpi test clean
