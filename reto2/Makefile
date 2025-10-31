# ==============================
#   Makefile para HPC Reto2
# ==============================

# Compilador
CC := gcc

# Flags por implementación
CFLAGS_SEQ = -Wall -O3 -ffast-math -march=native -flto
LDFLAGS_SEQ = -lm -flto

CFLAGS_HILOS = -Wall -O3 -ffast-math -march=native -flto
LDFLAGS_HILOS = -lm -pthread -flto

CFLAGS_PROC ?= -Wall -O3 -ffast-math -march=native -flto
LDFLAGS_PROC ?= -lm -flto

# >>> NUEVOS FLAGS PARA OPENMP <<<
CFLAGS_OMP = -Wall -O3 -ffast-math -march=native -flto -fopenmp
LDFLAGS_OMP = -lm -flto -fopenmp

# >>> FLAGS DE PERFILADO OPENMP (sin optimización) <<<
CFLAGS_OMP_PROFILE = -Wall -g -fopenmp -O0 -DRESULTS_DIR='"$(RESULTS_DIR)"'
LDFLAGS_OMP_PROFILE = -lm -fopenmp

# Directorios
SRC_DIR     := src
BIN_DIR     := bin
RESULTS_DIR := $(abspath results)
SCRIPTS_DIR := scripts
PROFILE_DIR := $(RESULTS_DIR)/profile_reports

# >>> Se agrega openmp a los subdirectorios <<<
SUBDIRS     := secuencial hilos procesos openmp
TARGETS     := needles dartboard

# Generar nombres de binarios automáticamente:
BINARIES := $(foreach d,$(SUBDIRS),$(foreach t,$(TARGETS),$(BIN_DIR)/$(d)_$(t)))

# ==============================
#   Reglas principales
# ==============================

.PHONY: all clean run list help test profile_perf profile_gprof verify tablas graficas speedup

# Compilar todo
all: $(BINARIES)

# ==============================
#   Reglas de compilación
# ==============================

# ---- DARTBOARD ----
$(BIN_DIR)/secuencial_dartboard: $(SRC_DIR)/secuencial/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_SEQ) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_SEQ)

$(BIN_DIR)/hilos_dartboard: $(SRC_DIR)/hilos/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_HILOS) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_HILOS)

$(BIN_DIR)/procesos_dartboard: $(SRC_DIR)/procesos/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_PROC) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_PROC)

# >>> NUEVA: OPENMP DARTBOARD <<<
$(BIN_DIR)/openmp_dartboard: $(SRC_DIR)/openmp/dartboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS_OMP) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_OMP)


# ---- NEEDLES ----
$(BIN_DIR)/secuencial_needles: $(SRC_DIR)/secuencial/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_SEQ) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_SEQ)

$(BIN_DIR)/hilos_needles: $(SRC_DIR)/hilos/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_HILOS) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_HILOS)

$(BIN_DIR)/procesos_needles: $(SRC_DIR)/procesos/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_PROC) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_PROC)

# >>> NUEVA: OPENMP NEEDLES <<<
$(BIN_DIR)/openmp_needles: $(SRC_DIR)/openmp/needles.c | $(BIN_DIR)
	$(CC) $(CFLAGS_OMP) -DRESULTS_DIR='"$(RESULTS_DIR)"' $< -o $@ $(LDFLAGS_OMP)


# Crear carpeta bin si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ==============================
#   Utilidades
# ==============================

clean:
	rm -rf $(BIN_DIR) $(PROFILE_DIR)

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
			hilos_*|procesos_*|openmp_*) \
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

list:
	@echo "Binarios disponibles:"; \
	for b in $(BINARIES); do echo "  $$(basename $$b)"; done

test: all
	@echo "Ejecutando pruebas con scripts/tests.py..."
	@python3 $(SCRIPTS_DIR)/tests.py

help:
	@echo "Opciones disponibles en este Makefile:"
	@echo "  make all              -> Compila todos los binarios"
	@echo "  make clean            -> Elimina los binarios compilados"
	@echo "  make list             -> Lista los binarios disponibles"
	@echo "  make run prog=...     -> Ejecuta un binario con parámetros"
	@echo "  make profile_perf     -> Perfila con perf (solo OpenMP)"
	@echo "  make profile_gprof    -> Perfila con gprof (solo OpenMP)"
	@echo "  make test             -> Compila y corre todas las pruebas con Python"
	@echo ""
	@echo "Ejemplos:"
	@echo "  make run prog=openmp_dartboard N=500000 workers=8"
	@echo "  make profile_gprof prog=openmp_needles N=100000 workers=4"
	@echo ""
	@echo "Los reportes de gprof se guardan en $(PROFILE_DIR)"

verify: all
	@python3 $(SCRIPTS_DIR)/verify.py

tablas: all
	@echo "Creando tablas con scripts/tablas.py..."
	@python3 $(SCRIPTS_DIR)/tablas.py

graficas: all
	@echo "Creando graficas con scripts/graficas.py..."
	@python3 $(SCRIPTS_DIR)/graficas.py

speedup: all
	@echo "Creando graficas de speedup con scripts/speedup.py..."
	@python3 $(SCRIPTS_DIR)/speedup.py


# ==============================
#   PERFILADO AUTOMÁTICO OPENMP
# ==============================

# ---- Perf con perf (requiere herramientas instaladas) ----
profile_perf:
	@if [ -z "$(prog)" ]; then \
		echo "❌ Falta 'prog'. Ej: make profile_perf prog=openmp_needles N=100000 workers=8"; exit 1; \
	fi; \
	if [ "$${prog%%_*}" != "openmp" ]; then \
		echo "⚠️ Solo se puede perfilar implementaciones OpenMP."; exit 1; \
	fi; \
	if [ -z "$(N)" ]; then \
		echo "❌ Falta 'N'."; exit 1; \
	fi; \
	if [ -z "$(workers)" ]; then workers=8; fi; \
	echo "🔍 Compilando $$prog sin optimizaciones para perfilado (perf)..."; \
	src_file="$(SRC_DIR)/openmp/$$(echo $(prog) | cut -d'_' -f2).c"; \
	out_file="$(BIN_DIR)/$$(basename $(prog))_profile"; \
	$(CC) $(CFLAGS_OMP_PROFILE) $$src_file -o $$out_file $(LDFLAGS_OMP_PROFILE); \
	echo "⚙️  Ejecutando con perf..."; \
	perf stat -d $$out_file $(N) $$workers

# ---- Perf con gprof (más portátil) ----
profile_gprof:
	@if [ -z "$(prog)" ]; then \
		echo "❌ Debes especificar el programa (prog=openmp_*)"; \
		exit 1; \
	fi; \
	if echo "$(prog)" | grep -q '^openmp_'; then \
		echo "🔍 Compilando $(prog) con gprof (sin optimización -O0)..."; \
		mkdir -p bin; \
		mkdir -p results/profile_reports; \
		gcc -Wall -g -fopenmp -O0 -pg -DRESULTS_DIR=\"results\" src/openmp/$$(echo $(prog) | sed 's/openmp_//').c -o bin/$(prog)_profile -lm -fopenmp; \
		echo "⚙️  Ejecutando $(prog) con N=$(N) y workers=$(workers)..."; \
		./bin/$(prog)_profile $(N) $(workers); \
		echo "📊 Generando reporte con gprof..."; \
		gprof ./bin/$(prog)_profile gmon.out > results/profile_reports/profile_$(prog).txt; \
		echo "✅ Reporte guardado en results/profile_reports/profile_$(prog).txt"; \
	else \
		echo "⚠️  Esta regla solo está pensada para programas OpenMP (nombre debe empezar con openmp_)"; \
		exit 1; \
	fi


