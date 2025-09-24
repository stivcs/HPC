# ==============================
#   Makefile para HPC Reto1
# ==============================

# Compilador
CC := gcc

# Flags por implementación
CFLAGS_SEQ   ?= -Wall -O2
LDFLAGS_SEQ  ?= -lm

CFLAGS_HILOS ?= -Wall -O2
LDFLAGS_HILOS ?= -lm -pthread

CFLAGS_PROC  ?= -Wall -O2
LDFLAGS_PROC ?= -lm

# Directorios
SRC_DIR     := src
BIN_DIR     := bin
RESULTS_DIR := $(abspath results)
SCRIPTS_DIR := scripts

SUBDIRS     := secuencial hilos procesos
TARGETS     := needles dartboard

# Generar nombres de binarios automáticamente:
BINARIES := $(foreach d,$(SUBDIRS),$(foreach t,$(TARGETS),$(BIN_DIR)/$(d)_$(t)))

# ==============================
#   Reglas principales
# ==============================

.PHONY: all clean run list help test

# Compilar todo
all: $(BINARIES)

# Compilación genérica de dartboard
$(BIN_DIR)/secuencial_dartboard: $(SRC_DIR)/secuencial/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_SEQ) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o $@ $(LDFLAGS_SEQ)

$(BIN_DIR)/hilos_dartboard: $(SRC_DIR)/hilos/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_HILOS) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o $@ $(LDFLAGS_HILOS)

$(BIN_DIR)/procesos_dartboard: $(SRC_DIR)/procesos/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_PROC) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o $@ $(LDFLAGS_PROC)

# Compilación genérica de needles
$(BIN_DIR)/secuencial_needles: $(SRC_DIR)/secuencial/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_SEQ) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o $@ $(LDFLAGS_SEQ)

$(BIN_DIR)/hilos_needles: $(SRC_DIR)/hilos/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_HILOS) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o $@ $(LDFLAGS_HILOS)

$(BIN_DIR)/procesos_needles: $(SRC_DIR)/procesos/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_PROC) -DRESULTS_DIR=\"$(RESULTS_DIR)\" $< -o $@ $(LDFLAGS_PROC)

# Crear carpeta bin si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ==============================
#   Utilidades
# ==============================

# Limpiar binarios
clean:
	rm -rf $(BIN_DIR)

# Ejecutar un binario con parámetros automáticos
# Uso:
#   make run prog=secuencial_needles N=100000
#   make run prog=hilos_dartboard N=200000 workers=8
#   make run prog=procesos_needles N=300000 workers=4
run:
	@if [ -z "$(prog)" ]; then \
		echo "Error: falta 'prog'. Ej: make run prog=secuencial_needles N=100000"; \
	else \
		case $(prog) in \
			secuencial_*) \
				if [ -z "$(N)" ]; then \
					echo "Error: falta N. Ej: make run prog=$(prog) N=100000"; exit 1; \
				fi; \
				$(BIN_DIR)/$(prog) $(N); \
				;; \
			hilos_*|procesos_*) \
				if [ -z "$(N)" ] || [ -z "$(workers)" ]; then \
					echo "Error: falta N o workers. Ej: make run prog=$(prog) N=200000 workers=8"; exit 1; \
				fi; \
				$(BIN_DIR)/$(prog) $(N) $(workers); \
				;; \
			*) \
				echo "Error: programa no reconocido: $(prog)"; \
				;; \
		esac; \
	fi

# Listar todos los binarios compilados
list:
	@echo "Binarios disponibles:"; \
	for b in $(BINARIES); do echo "  $$(basename $$b)"; done

# Ejecutar todos los tests con el script Python
test: all
	@echo "Ejecutando pruebas con scripts/tests.py..."
	@python3 $(SCRIPTS_DIR)/tests.py

# Ayuda
help:
	@echo "Opciones disponibles en este Makefile:"
	@echo "  make all              -> Compila todos los binarios"
	@echo "  make clean            -> Elimina los binarios compilados"
	@echo "  make list             -> Lista los binarios disponibles"
	@echo "  make run prog=...     -> Ejecuta un binario con parámetros"
	@echo "  make test             -> Compila y corre todas las pruebas con Python"
	@echo ""
	@echo "Ejemplos de ejecución:"
	@echo "  make run prog=secuencial_needles N=100000"
	@echo "  make run prog=hilos_dartboard N=200000 workers=8"
	@echo "  make run prog=procesos_needles N=300000 workers=4"
	@echo ""
	@echo "Variables de compilación personalizables:"
	@echo "  CFLAGS_SEQ, LDFLAGS_SEQ"
	@echo "  CFLAGS_HILOS, LDFLAGS_HILOS"
	@echo "  CFLAGS_PROC, LDFLAGS_PROC"

verify: all
	@python3 $(SCRIPTS_DIR)/verify.py