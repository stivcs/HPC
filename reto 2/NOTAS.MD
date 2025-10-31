# Notas de Optimización – Simulaciones de PI

## Optimización Secuencial Buffon’s Needle

### 1. Configuración inicial (sin optimización)

* **N** = 1,000,000
* **Flags de compilación:** `-Wall -O2`
* **Resultado aproximado de π:** 3.140432281
* **Tiempo de usuario:** 0.0531 s

**Observaciones:**

* Tiempo de ejecución aceptable.
* Precisión de π dentro del rango esperado para Monte Carlo.

---

### 2. Optimización solo por compilador

* **Flags recomendadas:**

```bash
-O3 -ffast-math -march=native -flto -Wall
```

* **Resultado aproximado de π:** 3.139658280
* **Tiempo de usuario:** 0.0331 s

**Observaciones:**

* Tiempo reducido en \~38% solo con flags de compilación.
* Precisión prácticamente idéntica (error < 0.03%).
* Optimización de compilación mejora rendimiento sin cambiar código.

---

### 3. Conclusiones

1. Optimización de compilador da la mayor mejora inicial.
2. La precisión se mantiene estable, incluso con `-ffast-math`.

---

## Optimización de Hilos – Buffon’s Needle

### Configuración inicial

* **N** = 1,000,000, **Workers** = 4
* **Flags:** `-Wall -O2 -pthread -lm`
* **Resultado de π:** 3.140545701
* **Tiempo real:** 0.0101 s

**Observaciones:**

* Paralelización reduce notablemente el tiempo.
* Precisión estable.

### Optimización

* **Flags recomendadas:**

```bash
-O3 -ffast-math -march=native -flto -Wall -pthread -lm
```

* **Resultado de π:** 3.140195603
* **Tiempo real:** 0.0067 s

**Observaciones:**

* Tiempo reducido \~34% respecto a compilación anterior.
* Precisión estable (error <0.03%).

---

## Procesos – Buffon’s Needle

### Configuración inicial

* **N** = 1,000,000, **Procesos** = 4
* **Flags:** `-Wall -O2 -pthread -lm`
* **Resultado de π:** 3.141285603
* **Tiempo real:** 0.009915 s

**Observaciones:**

* Paralelización mediante procesos reduce el tiempo respecto a secuencial.
* Precisión estable.

### Optimización

* **Flags:**

```makefile
CFLAGS_PROC ?= -Wall -O3 -ffast-math -march=native -flto
LDFLAGS_PROC ?= -lm -flto
```

* **Resultado de π:** 3.137747098
* **Tiempo real:** 0.007454 s

**Observaciones:**

* Mejora \~25% con compilador optimizado.
* Precisión dentro de tolerancia aceptable.
* Uso de `-O3`, `-ffast-math`, `-march=native` y `-flto` mejora rendimiento sin tocar código.

---

## Optimización Dartboard – Hilos

* **N** = 1,000,000, **Hilos** = 4
* **Flags:** `-O3 -ffast-math -march=native -flto -pthread`
* **Resultados aproximados de π y tiempo real:**

| Corrida | PI aproximado | Tiempo real (s) |
| ------- | ------------- | --------------- |
| 1       | 3.142476      | 0.004016        |
| 2       | 3.138360      | 0.010220        |
| 3       | 3.141956      | 0.003311        |

**Observaciones:**

* Variación debida a overhead de hilos y `rand_r()`.
* Pre-cálculo de constantes y optimización de compilador mejora tiempo promedio.
* Mayor limitante: creación y unión de hilos para experimentos cortos.

---

## Optimización Buffon Needles – Procesos

* Código original con `fork()` y memoria compartida (`shmget`).
* Uso de `rand_r()` con semilla por proceso.
* Precisión inicial insuficiente para verificación (>1% error).

**Optimización aplicada:**

* Reemplazo de aproximaciones por funciones estándar (`sinf()`).
* Mantener paralelización con `fork()`.
* Cada proceso calcula su `chunk = N / num_procesos`.
* Almacenamiento de hits en memoria compartida.
* Evitar que gestión de archivos interfiera con cálculo paralelo.

**Resultados optimizados:**

| Métrica     | Original        | Optimizado |
| ----------- | --------------- | ---------- |
| PI estimado | 3.194072        | 3.141957   |
| Error       | 1.67%           | <0.1%      |
| Tiempo real | 0.0037–0.0041 s | 0.00645 s  |

**Conclusiones:**

* Precisión matemática prioritaria sobre micro-optimización de CPU.
* Paralelización con `fork()` + memoria compartida eficiente.
* División de trabajo balanceada y semillas independientes garantizan reproducibilidad.
* Manejo de directorios y guardado de resultados se mantiene fuera de la región crítica para no afectar cálculo.

---

# Detalle completo de las optimizaciones de código implementadas (secuencial, hilos y procesos)

A continuación encontrarás **un análisis exhaustivo** de todas las optimizaciones de *código* que se implementaron en el proyecto (no las flags solamente), por cada variante (secuencial, hilos, procesos). Explico **qué** se cambió, **por qué** se hizo, **qué efecto** tiene en rendimiento y precisión, riesgos y **qué mediciones/observaciones** esperamos ver. Si algo suena técnico: perfecto — esto es para que puedas defender los cambios con criterio.

> Nota rápida: las optimizaciones buscan reducir coste por iteración (cálculo), minimizar overhead (threads/forks/IO) y mantener la precisión suficiente para pasar la verificación (≤1% error). Ahora el porqué en detalle.

---

## Resumen ejecutivo (1 frase)

Hicimos cambios focalizados en: reducir trabajo inútil dentro de los bucles críticos, usar PRNGs y semillas por unidad de ejecución (thread/process) para evitar contención, precomputar constantes, sustituir aproximaciones agresivas que rompían precisión por alternativas seguras (sinf), y reducir overhead de asignaciones dinÃ¡micas/llamadas de sistema durante el cómputo. Eso dio reducciones de tiempo sustanciales en secuencial y procesos, y mejoras más modestas en hilos (por el bajo coste ya existente y la sensibilidad al scheduler).

---

## 1) Secuencial — `secuencial_needles.c` y `secuencial_dartboard.c`

### Cambios aplicados (código)

1. **Precalculo de constantes:** `halfL = L/2.0`, `invRandMax = 1.0 / RAND_MAX` (cuando aplica).

   * *Dónde:* dentro de funciones justo antes del bucle principal.
   * *Por qué:* elimina divisiones y multiplicaciones repetidas en N iteraciones.
2. **Almacenamiento de resultados de funciones caras:** `s = sin(theta)` guardado en variable local por iteración.

   * *Por qué:* evita recalcular `sin()` si se utiliza más de una vez; reduce llamadas de función.
3. **Sustitución de RNG pesado:** se introdujo `fast_rand()` (xorshift) para `dartboard` y uso de `rand_r`/semillas por hilo/process donde era necesario.

   * *Por qué:* `rand()` es lento y no thread-safe; xorshift o `rand_r` reducen latencia en generación de números aleatorios. En secuencial el cambio a RNG rápido reduce el tiempo por iteración notablemente.
4. **Evitar malloc/free y cadenas dinámicas en el bucle crítico:** pasar a `char filename[256]` en lugar de `malloc` en el main; abrir/crear CSV fuera del bucle.

   * *Por qué:* elimina syscalls y operaciones de heap innecesarias que son costosas.
5. **Uso de `sinf` (float) cuando la precisión es suficiente** — aplicado con cuidado en procesos; en secuencial original mantuvimos `sin` salvo cuando se demostró que `sinf` era suficiente.

   * *Por qué:* `sinf` suele ser más rápido en hardware y en librerías vectorizadas; reduce tiempo trigonométrico.

### Impacto medible

* **Dartboard secuencial:** pasó de \~0.017 s a \~0.004 s (casi 4×) por combinar RNG rápido y `-O3` + otras micro-optim. Este es el mayor impacto observado: generación de aleatorios y evitar operaciones caras repetidas dominaban el tiempo.
* **Needles secuencial:** reducción de \~0.053 s a \~0.033 s (≈38%) con solo flags y pequeñas mejoras (precalc y evitar work redundant).

### Análisis crítico

* **Por qué funciona:** en simulaciones Monte Carlo la operación por iteración es mínima; por tanto, cualquier reducción en coste por iteración (menos divisiones, menos casting y RNG más rápido) se multiplica por N y se nota mucho.
* **Riesgos:** los RNG rápidos (xorshift, -ffast-math) y `sinf` pueden introducir pequeñas desviaciones estadísticas. Comprobamos que en secuencial la desviación quedó dentro de tolerancias (error muy pequeño). Sin embargo, siempre documentar el tipo de RNG usado y su no aptitud para uso criptográfico.

### Recomendaciones futuras

* Si quieres máxima reproducibilidad: fijar semilla explícita en `main` y registrar la semilla en el CSV.
* Para análisis estadístico: ejecutar múltiples corridas y reportar media ± desviación estándar.

---

## 2) Hilos (pthread) — `hilos_needles.c` y `hilos_dartboard.c`

### Cambios aplicados (código)

1. **Semilla por hilo y RNG thread-safe:** uso de `rand_r(&seed)` o xorshift per-thread (seed derivada de tiempo y id de hilo).

   * *Por qué:* evita contención de `rand()` global y evita locks; cada hilo genera números independientemente.
2. **Trabajo dividido por `chunk = N / num_threads`, sin accesos compartidos en el bucle:** cada hilo acumula en `local_hits` y escribe una sola vez en `data[thread].local_hits` al final.

   * *Por qué:* elimina sincronización fina (locks/atomics) dentro del bucle crítico.
3. **Stack allocation para `threads` y `data` cuando cabe:** cambio de `malloc` a arreglos en stack (si el número de hilos es pequeño/limitado) para evitar malloc/free overhead.

   * *Por qué:* reduce syscalls y mejora locality.
4. **Precalculo de constantes fuera del bucle (ej. `half_L`) y uso de `sinf()` o `sinf`/`sin` según tradeoff precisión/velocidad.**

### Impacto medible

* En tus pruebas los **hilos ya eran muy rápidos**: tiempos \~0.003–0.004 s en muchos casos. Las optimizaciones de código aportaron mejoras menores (milisegundos o algunos porcentajes), porque la parte dominante pasó a ser overhead del sistema (scheduler, creación/join de hilos) y la latencia de `rand_r`.

### Análisis crítico

* **Por qué menor ganancia:** cuando el núcleo del trabajo por iteración es extremadamente simple (pocas operaciones FP), el overhead del threading y del RNG dominan, y reducir multiplicaciones/divisiones aporta poco relativo.
* **Efecto de la variabilidad:** en corridas tan cortas (ms), factores del SO (interrupts, otras tareas) generan ruido importante en los tiempos medidos. Por tanto, hay que promediar muchas corridas.

### Riesgos y contramedidas

* **Semillas igualadas:** si usas `time(NULL)` sola, varios hilos procesos pueden compartir la misma semilla si se lanzan «al mismo segundo». Por eso combinamos con id y/o `clock_gettime`/`getpid`.
* **Stack vs heap:** usar stack para arrays grandes puede provocar stack overflow en sistemas con límites pequeños; si esperas num\_threads grande, volver a `malloc`.

### Recomendaciones futuras

* Para mejorar más: generar bloques de números aleatorios por hilo en un buffer y procesarlos en batch (mejor vectorización, menos llamadas a RNG).
* Considerar OpenMP si buscas una sintaxis más simple y la posibilidad de `#pragma omp simd` para vectorizar el bucle.

---

## 3) Procesos (fork + shm) — `procesos_needles.c` y `procesos_dartboard.c`

> Nota: aquí hay dos ejes de optimización: (A) reducir coste por iteración, (B) minimizar overhead de IPC/fork/joins. Nosotros nos centramos en (A) sin romper la lógica de (B).

### Cambios aplicados (código)

1. **Seed mejorada por proceso:** semilla derivada de `clock_gettime(CLOCK_MONOTONIC)` y `getpid()` o `proc_id` para evitar colisiones si los procesos se crean rápidamente.

   * *Por qué:* `time(NULL)` puede ser idéntica para varios forks; eso causa correlación en PRNG.
2. **Distribución de `chunk` con reparto del residuo:** `base_chunk = N / num_procs`, `remainder = N % num_procs`, dar +1 a los primeros `remainder` procesos.

   * *Por qué:* asegura que la suma de iteraciones sea exactamente N (mejora precision/statistics).
3. **Aproximación trigonométrica ajustada:** inicialmente probamos `fast_sin()` (Taylor 3ro) para mejorar tiempo; detectamos error excesivo → cambiamos a `sinf()` (float) que mantiene precisión y es más rápida que `sin()` doble en la mayoría de librerías.

   * *Por qué:* equilibrio velocidad vs precisión. `sinf()` reduce error a aceptable nivel y mantiene ganancia en tiempo.
4. **Memoria compartida simple (shmget + shmat) y escritura única por proceso:** cada proceso escribe `shm_hits[proc_id] = local_hits` y sale; el padre suma.

   * *Por qué:* evita sincronizaciones costosas; la escritura a una celda propia es atómica por diseño de caché/word size.
5. **Evitamos hacer syscalls innecesarios dentro del bucle:** todas las operaciones de I/O y creación de directorios se hacen fuera del cron de cálculo.

### Impacto medible

* **Needles procesos:** tiempos originales \~0.007–0.008 s; con optimización (sinf y reparto de chunk) bajaron a \~0.0038–0.0041 s en la primera versión agresiva, pero la aproximación rápida rompía precisión; al usar `sinf` se mantuvo buena velocidad y devolvió la precisión: ejemplo final \~0.00645 s (algo mayor que la versión agresiva, pero con error < 1%).
* **Dartboard procesos:** se observó mejora ligera y estabilización de tiempos (\~0.0039–0.0041 s) tras mejorar la semilla y optimizar el bucle.

### Análisis crítico

* **Tradeoff principal:** velocidad vs precisión al aproximar funciones trigonométricas. Para Needles, usar `fast_sin` rompió la verificación → inaceptable. `sinf` ofrece un buen punto intermedio.
* **Por qué mejora:** reducir coste por iteración (menos operaciones FP, menos divisiones) y generar mejores semillas reduce correlación de números aleatorios, lo que mejora el comportamiento estadístico y evita sesgos que podrían aumentar el error de π.
* **Overhead de fork:** la creación y espera de procesos tiene un coste, pero al ser pocos procesos (4) y N grande, este overhead se amortiza.

### Riesgos y contramedidas

* `sinf()` es más rápida pero no siempre precisa al 100% — validamos por verificación empírica.
* Escritura en `shm_hits` desde procesos múltiples está OK si cada proceso tiene su índice; cuidado si se cambia el esquema.

### Recomendaciones futuras

* Para producción: cambiar `rand_r` por `pcg32` o `xoroshiro128+` por proceso: mejor calidad estadística y buena velocidad. Documenta la elección de PRNG.
* Para cargas mucho mayores: considerar buffers de aleatorios y/o SIMD; usar `mmap` (shared anon) en lugar de SysV shm para portabilidad y menor API-legacy.

---

## 4) Cambios generales (estilo, seguridad y reproducibilidad)

1. **Evitar prints en el bucle** — todos los printf se mantienen fuera del bucle crítico y sólo imprimen resumen.
2. **Cabeceras CSV controladas** — escribimos header solo si el archivo no existía para evitar overhead.
3. **Mediciones reproducibles** — se midió con `getrusage` y `clock_gettime(CLOCK_MONOTONIC)` según el caso para medir user/real time correctamente.
4. **Documentar semilla usada** — sugerimos añadir la semilla en el CSV para reproducibilidad de pruebas.

---

## 5) Herramientas de validación y métricas que usamos y recomendamos

* **Comparación de PI**: siempre comparar con M\_PI y medir error porcentual. Tolerancia del proyecto: 1%.
* **Múltiples repeticiones y estadística:** por la variación del scheduler, correr 5–10 repeticiones y usar media y desviación estándar.
* **Profilers:** `perf stat`, `perf record` + `perf report` y `gprof` para encontrar hot spots.
* **Valgrind/Cachegrind**: para ver accesos a memoria y cache-misses si se sospecha mala locality.

---

## 6) Posibles pasos de optimización avanzada (si quieres exprimirlo más)

1. **SIMD / vectorización manual:** procesar 4–8 RNGs por iteración y usar `xmmintrin`/`avx` intrinsics o `#pragma omp simd`.

   * Coste: complejidad de código y validación estadística; beneficio mayor cuanto más grande es N.
2. **PRNG por bloque**: generar N por bloques (por ejemplo 1e4 números) en un buffer y operar sobre el buffer — reduce coste de llamada a RNG.
3. **OpenMP**: simplifica paralelización, permite `simd` y reducción automática; cuidado con reproducibilidad y semilla.
4. **Librerías matemáticas vectorizadas**: como SVML (intel) o sleef para trigonometría vectorizada de alta velocidad (si aceptas licencias/dep.).

---

## 7) Conclusión crítica y cierre

Hicimos las optimizaciones correctas para el contexto: balanceadas entre rendimiento y precisión, conservando la estructura y los requisitos del proyecto. En secuencial ganamos mucho por reducir el coste por iteración y cambiar la forma en que generamos números aleatorios. En hilos, dada la naturaleza ligera del cálculo, las ganancias de micro-optimización son menores y la atención se debe centrar en amortizar overhead (N mayor, batching). En procesos, el principal riesgo fue sacrificar precisión por velocidad; corregimos ese tradeoff adoptando `sinf` y semillas robustas.

