# ==========================================
#   Makefile — Proyecto HPC: Multiplicación de Matrices
# ==========================================
# Estructura esperada:
#   src/
#     secuencial/secuencial.c
#     openmp/matrixOpenMp.c
#   bin/
#   results/
#
# Uso:
#   make all
#   make run prog=secuencial N=512
#   make run prog=openmp_opt N=512 threads=4
#   make clean
# ==========================================

# Compilador
CC := gcc

# ==============================
#   Flags de compilación
# ==============================
CFLAGS_BASE := -Wall -O3 -ffast-math -march=native -flto
LDFLAGS_BASE := -lm -flto

# Versión secuencial
CFLAGS_SEQ  := $(CFLAGS_BASE)
LDFLAGS_SEQ := $(LDFLAGS_BASE)

# Versión OpenMP
CFLAGS_OMP  := $(CFLAGS_BASE) -fopenmp -funroll-loops
LDFLAGS_OMP := $(LDFLAGS_BASE) -fopenmp

# ==============================
#   Directorios
# ==============================
SRC_DIR     := src
BIN_DIR     := bin
RESULTS_DIR := $(abspath results)

# ==============================
#   Archivos fuente y binarios
# ==============================
SRC_SEQ := $(SRC_DIR)/secuencial/secuencial.c
SRC_OMP := $(SRC_DIR)/openmp/matrixOpenMp.c

BIN_SEQ := $(BIN_DIR)/secuencial
BIN_OMP := $(BIN_DIR)/openmp_opt

# ==============================
#   Reglas principales
# ==============================
.PHONY: all clean run list help dirs

# Compila ambas versiones
all: dirs $(BIN_SEQ) $(BIN_OMP)

# ==============================
#   Creación de directorios
# ==============================
dirs:
	@echo "Verificando estructura de carpetas..."
	@mkdir -p "$(BIN_DIR)"
	@mkdir -p "$(RESULTS_DIR)"
	@mkdir -p "$(SRC_DIR)/secuencial"
	@mkdir -p "$(SRC_DIR)/openmp"
	@echo "[OK] Directorios verificados o creados"

# ---- Compilación de versión secuencial ----
$(BIN_SEQ): $(SRC_SEQ)
	@echo "Compilando versión secuencial..."
	$(CC) $(CFLAGS_SEQ) -DRESULTS_DIR=\"$(abspath $(RESULTS_DIR))\" $< -o "$@" $(LDFLAGS_SEQ)
	@echo "[OK] Generado: $@"


# ---- Compilación de versión OpenMP ----
$(BIN_OMP): $(SRC_OMP)
	@echo "Compilando versión OpenMP..."
	$(CC) $(CFLAGS_OMP) "-DRESULTS_DIR=\"$(abspath $(RESULTS_DIR))\"" $< -o "$@" $(LDFLAGS_OMP)
	@echo "[OK] Generado: $@"

# ==============================
#   Utilidades
# ==============================

# Limpia los binarios generados y los resultados
clean:
	rm -rf "$(BIN_DIR)"
	rm -rf "$(RESULTS_DIR)"
	@echo "[Limpieza completa de binarios y resultados]"

# Ejecutar con parámetros desde la línea de comandos
# Ejemplos:
#   make run prog=secuencial N=512
#   make run prog=openmp_opt N=512 threads=4
run:
	@if [ -z "$(prog)" ]; then \
		echo "Error: falta 'prog'. Ejemplo: make run prog=secuencial N=512"; \
	else \
		case $(prog) in \
			secuencial) \
				if [ -z "$(N)" ]; then echo "Error: falta N"; exit 1; fi; \
				mkdir -p "$(RESULTS_DIR)/Secuencial_Data"; \
				"$(BIN_DIR)/$(prog)" $(N); \
				;; \
			openmp_opt) \
				if [ -z "$(N)" ] || [ -z "$(threads)" ]; then \
					echo "Error: falta N o threads. Ejemplo: make run prog=openmp_opt N=512 threads=4"; exit 1; fi; \
				mkdir -p "$(RESULTS_DIR)/OpenMp_Data"; \
				"$(BIN_DIR)/$(prog)" $(N) $(threads); \
				;; \
			*) \
				echo "Error: programa no reconocido: $(prog)"; \
				;; \
		esac; \
	fi

# Lista los binarios generados
list:
	@echo "Binarios disponibles:"; \
	for b in $(BIN_SEQ) $(BIN_OMP); do \
		if [ -f $$b ]; then echo "  $$(basename $$b)"; fi; \
	done

# Ayuda general
help:
	@echo "Makefile - Proyecto HPC: Multiplicación de Matrices"
	@echo ""
	@echo "Comandos principales:"
	@echo "  make all               -> Compila las versiones secuencial y OpenMP"
	@echo "  make clean             -> Elimina los binarios y resultados"
	@echo "  make list              -> Lista los binarios disponibles"
	@echo "  make dirs              -> Crea las carpetas necesarias"
	@echo ""
	@echo "Ejecución de ejemplos:"
	@echo "  make run prog=secuencial N=512"
	@echo "  make run prog=openmp_opt N=512 threads=4"
