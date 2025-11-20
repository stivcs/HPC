# Nombre del ejecutable
EXEC = mul_mat
# Compilador MPI
CC = mpicc
# Flags de compilación
CFLAGS = -O2

# Hosts donde se ejecutará
HOSTS = wn1,wn2,wn3
# Número de procesos (valor por defecto)
N = 7
# Tamaño de la matriz (valor por defecto)
SIZE = 1000

all: $(EXEC)

$(EXEC): mul_mat.c
	$(CC) $(CFLAGS) mul_mat.c -o $(EXEC)

run:
	mpiexec -n $(N) -host $(HOSTS) -oversubscribe ./$(EXEC) $(S)

test:
	python3 test.py

clean:
	rm -f $(EXEC) *.o
	rm -f results/*.csv

.PHONY: all run clean
