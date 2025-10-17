# ==========================================
#   Makefile — Proyecto HPC: Multiplicación de Matrices
# ==========================================
# Estructura esperada:
#   src/
#     secuencial/secuencial.c
#     openmp/matrixOpenMp.c
#   bin/
#   results/
#   scripts/verify.py
#
# Uso:
#   make all
#   make run prog=secuencial N=512
#   make verify prog=openmp_opt N=128 threads=4
# ==========================================

# Compilador
CC := gcc

# ==============================
#   FLAGS DE COMPILACIÓN
# ==============================

# --- Versión Secuencial ---
CFLAGS_SEQ  := -Wall -O3 -funroll-loops -ffast-math -march=native -pipe
LDFLAGS_SEQ := -lm

# --- Versión OpenMP ---
CFLAGS_OMP  := -Wall -O3 -funroll-loops -ffast-math -march=native -fopenmp -mtune=native
LDFLAGS_OMP := -lm -fopenmp

# ==============================
#   DIRECTORIOS
# ==============================
SRC_DIR     := src
BIN_DIR     := bin
RESULTS_DIR := $(abspath results)
SCRIPTS_DIR := scripts

# ==============================
#   ARCHIVOS FUENTE Y BINARIOS
# ==============================
SRC_SEQ := $(SRC_DIR)/secuencial/secuencial.c
SRC_OMP := $(SRC_DIR)/openmp/matrixOpenMp.c

BIN_SEQ := $(BIN_DIR)/secuencial
BIN_OMP := $(BIN_DIR)/openmp_opt

# ==============================
#   REGLAS PRINCIPALES
# ==============================
.PHONY: all clean run list help dirs verify

all: dirs $(BIN_SEQ) $(BIN_OMP)
	@echo "[OK] Compilación completa."

# ==============================
#   CREACIÓN DE DIRECTORIOS
# ==============================
dirs:
	@echo "Verificando estructura de carpetas..."
	@mkdir -p "$(BIN_DIR)"
	@mkdir -p "$(RESULTS_DIR)"
	@mkdir -p "$(SRC_DIR)/secuencial"
	@mkdir -p "$(SRC_DIR)/openmp"
	@echo "[OK] Directorios verificados o creados."

# ==============================
#   COMPILACIÓN
# ==============================

# --- Compilación Secuencial ---
$(BIN_SEQ): $(SRC_SEQ)
	@echo "Compilando versión Secuencial..."
	$(CC) $(CFLAGS_SEQ) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o "$@" $(LDFLAGS_SEQ)
	@echo "[OK] Binario generado: $@"

# --- Compilación OpenMP ---
$(BIN_OMP): $(SRC_OMP)
	@echo "Compilando versión OpenMP..."
	$(CC) $(CFLAGS_OMP) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o "$@" $(LDFLAGS_OMP)
	@echo "[OK] Binario generado: $@"

# ==============================
#   LIMPIEZA
# ==============================
clean:
	@echo "Eliminando binarios y resultados..."
	@rm -rf "$(BIN_DIR)" "$(RESULTS_DIR)"
	@echo "[Limpieza completa]"

# ==============================
#   EJECUCIÓN
# ==============================
# Ejemplos:
#   make run prog=secuencial N=512
#   make run prog=secuencial N=256 args=save
#   make run prog=openmp_opt N=512 threads=4
# ==============================
run:
	@if [ -z "$(prog)" ]; then \
		echo "Error: falta 'prog'. Ejemplo: make run prog=secuencial N=512"; \
		exit 1; \
	fi; \
	case "$(prog)" in \
		secuencial) \
			if [ -z "$(N)" ]; then echo "Error: falta N"; exit 1; fi; \
			mkdir -p "$(RESULTS_DIR)/Secuencial_Data"; \
			if [ -n "$(args)" ]; then \
				"$(BIN_DIR)/secuencial" $(N) $(args); \
			else \
				"$(BIN_DIR)/secuencial" $(N); \
			fi; \
			;; \
		openmp_opt) \
			if [ -z "$(N)" ] || [ -z "$(threads)" ]; then \
				echo "Error: falta N o threads. Ejemplo: make run prog=openmp_opt N=512 threads=4"; \
				exit 1; \
			fi; \
			mkdir -p "$(RESULTS_DIR)/OpenMp_Data"; \
			if [ -n "$(args)" ]; then \
				"$(BIN_DIR)/openmp_opt" $(N) $(threads) $(args); \
			else \
				"$(BIN_DIR)/openmp_opt" $(N) $(threads); \
			fi; \
			;; \
		*) \
			echo "Error: programa no reconocido: $(prog)"; \
			exit 1; \
			;; \
	esac

# ==============================
#   VERIFICACIÓN AUTOMÁTICA
# ==============================
# Ejecuta el binario con "save" y llama a scripts/verify.py
# Ejemplo:
#   make verify prog=secuencial N=64
#   make verify prog=openmp_opt N=128 threads=4
# ==============================
verify:
	@if [ -z "$(prog)" ]; then \
		echo "Error: falta 'prog'. Ejemplo: make verify prog=secuencial N=64"; \
		exit 1; \
	fi; \
	echo "🔧 Generando matrices para verificación..."; \
	case "$(prog)" in \
		secuencial) \
			N_VAL=$${N:-64}; \
			mkdir -p "$(RESULTS_DIR)/Secuencial_Data"; \
			"$(BIN_DIR)/secuencial" $$N_VAL save; \
			echo "🧠 Verificando resultados (Secuencial)..."; \
			python3 "$(SCRIPTS_DIR)/verify.py" secuencial; \
			;; \
		openmp_opt) \
			N_VAL=$${N:-64}; \
			T_VAL=$${threads:-4}; \
			mkdir -p "$(RESULTS_DIR)/OpenMp_Data"; \
			"$(BIN_DIR)/openmp_opt" $$N_VAL $$T_VAL save; \
			echo "🧠 Verificando resultados (OpenMP)..."; \
			python3 "$(SCRIPTS_DIR)/verify.py" openmp; \
			;; \
		*) \
			echo "Error: programa no reconocido. Usa prog=secuencial u openmp_opt."; \
			exit 1; \
			;; \
	esac

# ==============================
#   EJECUCIÓN DE TEST AUTOMATIZADOS (TAMAÑOS ESPECÍFICOS)
# ==============================
# Uso:
#   make test
# Ejecuta todos los tamaños definidos en test_suite_auto.py con 10 repeticiones cada uno
test:
	@echo "==========================================="
	@echo "Ejecutando test automatizado para tamaños específicos de matriz"
	@echo "10 repeticiones por configuración, hilos OpenMP: 2,4,8,12"
	@echo "==========================================="
	python3 "$(SCRIPTS_DIR)/test.py"

tablas:
	@echo "==========================================="
	@echo "Ejecutando tablas para tamaños específicos de matriz"
	@echo "==========================================="
	python3 "$(SCRIPTS_DIR)/tablas.py"

perfilado:
	@echo "==========================================="
	@echo "Ejecutando tablas para perfilado de código"
	@echo "==========================================="
	python3 "$(SCRIPTS_DIR)/perfilado.py"

graficas:
	@echo "==========================================="
	@echo "Ejecutando graficas"
	@echo "==========================================="
	python3 "$(SCRIPTS_DIR)/graficas.py"

graficasP:
	@echo "==========================================="
	@echo "Ejecutando graficas de perfilado"
	@echo "==========================================="
	python3 "$(SCRIPTS_DIR)/graficasPerfilado.py"

pruebas:
	python3 "$(SCRIPTS_DIR)/pruebas.py"

# ==============================
#   UTILIDADES
# ==============================
list:
	@echo "Binarios disponibles:"
	@for b in $(BIN_SEQ) $(BIN_OMP); do \
		if [ -f $$b ]; then echo "  $$(basename $$b)"; fi; \
	done

help:
	@echo "Makefile - Proyecto HPC: Multiplicación de Matrices"
	@echo ""
	@echo "Comandos principales:"
	@echo "  make all               -> Compila las versiones secuencial y OpenMP"
	@echo "  make run prog=secuencial N=512"
	@echo "  make run prog=openmp_opt N=512 threads=4"
	@echo "  make verify prog=secuencial N=64"
	@echo "  make verify prog=openmp_opt N=128 threads=4"
	@echo "  make clean             -> Elimina los binarios y resultados"
	@echo "  make list              -> Lista los binarios disponibles"
