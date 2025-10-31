# HPC Reto1 - Simulación Buffon & Dartboard

Este proyecto implementa dos simulaciones para estimar π:

1. **Buffon's Needle** (`needles`)
2. **Dartboard** (`dartboard`)

Cada algoritmo tiene tres implementaciones:

* **Secuencial**
* **Hilos** (con paralelización por threads)
* **Procesos** (paralelización por procesos)

---

## Estructura del proyecto

```
reto1/
├── src/
│   ├── secuencial/
│   │   ├── needles.c
│   │   └── dartboard.c
│   ├── hilos/
│   │   ├── needles.c
│   │   └── dartboard.c
│   └── procesos/
│       ├── needles.c
│       └── dartboard.c
├── bin/                # Contiene los binarios compilados
├── results/            # Salidas y CSV de resultados
├── scripts/
│   └── verify.py       # Script de verificación de π
└── Makefile
```

---

## Requisitos

* Linux / macOS
* `gcc` con soporte para threads (`-pthread`)
* Python 3 para el script de verificación
* `make`

---

## Compilar todo

```bash
make all
```

Esto genera los binarios en `bin/`:

* `bin/secuencial_needles`
* `bin/secuencial_dartboard`
* `bin/hilos_needles`
* `bin/hilos_dartboard`
* `bin/procesos_needles`
* `bin/procesos_dartboard`

---

## Limpiar binarios

```bash
make clean
```

---

## Listar binarios disponibles

```bash
make list
```

---

## Ejecutar un binario

### Secuencial

```bash
make run prog=secuencial_needles N=100000
make run prog=secuencial_dartboard N=100000
```

### Hilos o Procesos

```bash
make run prog=hilos_dartboard N=200000 workers=8
make run prog=procesos_needles N=300000 workers=4
```

> Nota: `workers` indica la cantidad de hilos o procesos a usar.

---

## Verificación rápida de π

El proyecto incluye un script para verificar que los resultados sean razonables comparando con `math.pi`.

```bash
make verify
```

Opcionalmente puedes ajustar:

```bash
make verify N=50000 workers=4
```

> El script recorrerá todos los binarios compilados y marcará OK/FAIL dependiendo del error relativo.

---

## Resultados

Los resultados se guardan automáticamente en CSV dentro de `results/`:

* `results/secuencial/Secuencial_Needles_Data/Needles_Results.csv`
* `results/hilos/Hilos_Dartboard_Data/Dartboard_Results.csv`
* `results/procesos/Procesos_Needles_Data/Needles_Results.csv`
* …y así para cada combinación

Cada CSV contiene columnas:

```
N, pi_est, user_time
```

---

## Flags de compilación

Puedes personalizar flags por implementación editando el Makefile:

```makefile
# Secuencial
CFLAGS_SEQ   ?= -Wall -O2
LDFLAGS_SEQ  ?= -lm

# Hilos
CFLAGS_HILOS ?= -Wall -O2
LDFLAGS_HILOS ?= -lm -pthread

# Procesos
CFLAGS_PROC  ?= -Wall -O2
LDFLAGS_PROC ?= -lm
```

---

## Ayuda

```bash
make help
```

Muestra un resumen de todos los comandos y ejemplos.

---

Con esto tu equipo puede:

* Compilar
* Ejecutar cualquier binario
* Revisar resultados rápidamente
* Verificar consistencia de π
* Exportar CSV para análisis o gráficos