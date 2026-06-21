# Sistema Integral de Gestión y Predicción de Contaminación del Aire en Zonas Urbanas
## Quito, Ecuador — Proyecto Académico (Lion) · S8 Ejercicio de Programación

---

## Tabla de contenidos

1. [Propósito](#propósito)
2. [Estructura del proyecto](#estructura-del-proyecto)
3. [Requisitos del sistema](#requisitos-del-sistema)
4. [Instrucciones de compilación](#instrucciones-de-compilación)
5. [Instrucciones de ejecución](#instrucciones-de-ejecución)
6. [Formato de archivos de entrada](#formato-de-archivos-de-entrada)
7. [Casos de prueba (escenarios A, B, C)](#casos-de-prueba)
8. [Algoritmo de predicción](#algoritmo-de-predicción)
9. [Límites OMS utilizados](#límites-oms-utilizados)
10. [Mapeo a la rúbrica](#mapeo-a-la-rúbrica)
11. [Decisiones de diseño](#decisiones-de-diseño)

---

## Propósito

Este sistema permite a operadores municipales:
- **Monitorear** en tiempo real (registro manual) los niveles de seis contaminantes del aire en múltiples zonas urbanas de Quito.
- **Analizar** promedios históricos de los últimos 30 días para comparar con límites OMS.
- **Predecir** los niveles del día siguiente mediante una suavización exponencial simple.
- **Alertar** automáticamente cuando los valores actuales o proyectados superan límites OMS, indicando severidad y recomendaciones de gestión.
- **Reportar** toda la información en un archivo de texto con fecha y hora de generación.

---

## Estructura del proyecto

```
aire_urbano/
├── main.c           → Punto de entrada: menú interactivo y coordinación
├── monitor.c/.h     → Lógica de zonas, mediciones, promedios, predicción y alertas
├── io.c/.h          → Lectura/escritura de archivos CSV (historial, reporte)
├── utils.c/.h       → Validación de entrada, manipulación de cadenas, utilidades
├── Makefile         → Reglas de compilación (all, clean, run, test)
├── datos_ejemplo.txt → 30 registros de ejemplo en 5 zonas de Quito
└── README.md        → Este archivo
```

---

## Requisitos del sistema

| Componente | Requisito |
|---|---|
| Sistema operativo | Linux (Ubuntu, Debian, Fedora, etc.) |
| Compilador | GCC 5.0+ (con soporte C99) |
| Librerías | Solo estándar: `stdio.h`, `stdlib.h`, `string.h`, `math.h`, `time.h` |
| Espacio en disco | < 1 MB |

---

## Instrucciones de compilación

### Compilación estándar

```bash
# 1. Clonar o copiar los archivos fuente en un directorio
cd aire_urbano/

# 2. Compilar con Make
make

# Salida esperada (abreviada):
# gcc -std=c99 -Wall -Wextra -Wpedantic -g -c -o main.o main.c
# gcc -std=c99 -Wall -Wextra -Wpedantic -g -c -o monitor.o monitor.c
# gcc -std=c99 -Wall -Wextra -Wpedantic -g -c -o io.o io.c
# gcc -std=c99 -Wall -Wextra -Wpedantic -g -c -o utils.o utils.c
# gcc -std=c99 -Wall -Wextra -Wpedantic -g -o aire_urbano main.o monitor.o io.o utils.o -lm
# Compilacion exitosa: aire_urbano
```

### Compilación manual (sin Make)

```bash
gcc -std=c99 -Wall -g -o aire_urbano main.c monitor.c io.c utils.c -lm
```

### Ejecutar pruebas unitarias

```bash
make test
# O directamente:
./aire_urbano --test
```

Ejecuta dos funciones de prueba definidas en `monitor.c`:

- **`test_prediccion`**: carga 3 registros de PM2.5 (10, 12, 14) en una zona de prueba y verifica que la predicción ponderada exponencial coincida con el valor esperado (`≈ 13.16`) con tolerancia `±0.01`.
- **`test_alertas`**: recorre 4 casos con PM2.5 para verificar la clasificación de severidad:
  - Caso A: PM2.5 = 14.0 → sin alerta
  - Caso B: PM2.5 = 16.0 (6.7 % sobre límite) → ADVERTENCIA
  - Caso C: PM2.5 = 19.0 (26.7 % sobre límite) → ALERTA
  - Caso D: PM2.5 = 28.0 (86.7 % sobre límite) → CRÍTICA

### Limpiar archivos generados

```bash
make clean
```

Elimina los archivos objeto (`.o`), el ejecutable `aire_urbano` y todos los archivos `reporte_*.txt`. El archivo `historial.txt` **no se elimina** (se preservan los datos del usuario). Para borrarlo también: `rm -f historial.txt`.

---

## Instrucciones de ejecución

### Modo interactivo (menú de consola)

```bash
make run
# o:
./aire_urbano
```

Al iniciar, el sistema:
1. Inicializa las 5 zonas de Quito por defecto (Centro Histórico, La Mariscal, Cumbayá, La Carolina, La Floresta).
2. Detecta y carga `datos_ejemplo.txt` automáticamente si existe en el directorio de trabajo.
3. Muestra el menú principal.

### Modo con archivo preexistente

```bash
./aire_urbano --cargar historial.txt
```

### Menú principal

```
============================================================
  SISTEMA INTEGRAL DE MONITOREO DE CALIDAD DEL AIRE
  Quito, Ecuador
============================================================
  [1] Mostrar zonas y datos actuales
  [2] Anadir zona
  [3] Registrar medicion actual
  [4] Cargar datos historicos desde archivo
  [5] Calcular promedios historicos (30 dias)
  [6] Predecir niveles 24h por zona
  [7] Generar reporte
  [8] Exportar/guardar historial
  [0] Salir
------------------------------------------------------------
```

**Opción 4 — Cargar historial:** solicita la ruta del archivo. Presionar Enter carga `historial.txt`; ingresar `e` carga `datos_ejemplo.txt`; cualquier otra ruta carga el archivo indicado.

**Opción 8 / Salir:** al guardar con opción 8 (o al salir con la opción 0), si `historial.txt` ya existe el sistema solicita confirmación antes de sobrescribirlo.

---

## Formato de archivos de entrada

### `historial.txt` / `datos_ejemplo.txt`

Cada línea no comentada sigue el formato CSV:

```
YYYY-MM-DD,HH:MM,Nombre de Zona,PM2.5,PM10,NO2,O3,SO2,CO
```

**Reglas:**
- Las líneas que comienzan con `#` son comentarios y se ignoran.
- Las líneas vacías se ignoran.
- `PM2.5`, `PM10`, `NO2`, `O3`, `SO2` en **µg/m³**.
- `CO` en **mg/m³**.
- Los valores deben ser **≥ 0**.
- El separador decimal puede ser `.` o `,` (se normaliza internamente).
- Si una línea tiene formato inválido, se reporta su número y se omite.

**Ejemplo:**

```
# Comentario
2026-05-25,08:00,Centro Historico,8.5,21.5,12.5,44.0,8.2,1.2
2026-05-25,09:00,La Mariscal,18.0,44.0,24.5,90.0,25.0,3.1
```

---

## Casos de prueba

Los `datos_ejemplo.txt` incluyen tres escenarios representativos.

---

### Caso A — Niveles dentro de límites (sin alertas)

**Zonas:** Centro Histórico y La Floresta  
**Descripción:** Todos los contaminantes por debajo de los límites OMS.

**Cómo reproducir:**

```bash
./aire_urbano
# Menú: opción 5 → seleccionar "La Floresta"
# o: opción 6 → seleccionar "La Floresta"
```

**Salida esperada (opción 6 — predicción):**

```
  Zona: La Floresta  (6 registros usados, alpha=0.8)
  Contam.     Pred.24h   LimOMS24h  Estado
  ----------------------------------------------------------
  PM2.5          6.02      15.0   OK
  PM10          16.21      45.0   OK
  NO2            9.56      25.0   OK
  O3            40.17     100.0   OK
  SO2            6.43      40.0   OK
  CO             0.88       4.0   OK

  Predicción dentro de límites OMS. Sin alertas.
```

---

### Caso B — Alertas moderadas (ADVERTENCIA / ALERTA)

**Zonas:** La Mariscal y La Carolina  
**Descripción:** PM2.5 y O3 superan límites 24h en ~20–30%.

**Cómo reproducir:**

```bash
./aire_urbano
# Menú: opción 6 → seleccionar "La Mariscal"
```

**Salida esperada:**

```
  Zona: La Mariscal  (6 registros usados, alpha=0.8)
  Contam.     Pred.24h   LimOMS24h  Estado
  ----------------------------------------------------------
  PM2.5         17.41      15.0   SUPERA LIMITE [!]
  PM10          43.52      45.0   OK
  NO2           23.43      25.0   OK
  O3            86.12     100.0   OK
  SO2           23.21      40.0   OK
  CO             2.93       4.0   OK

  [!] Alertas detectadas (1):
    [ALERTA] PM2.5: 17.41 ug/m3 → Límite 15.0
    Rec: Reducir trafico vehicular. Suspender quemas a cielo abierto. Informar a la poblacion y reforzar monitoreo.
```

---

### Caso C — Niveles críticos (CRÍTICA)

**Zona:** Cumbayá (episodio de contaminación hipotético)  
**Descripción:** PM2.5, O3 y CO superan límites >50% → nivel CRÍTICA.

**Cómo reproducir:**

```bash
./aire_urbano
# Menú: opción 6 → seleccionar "Cumbaya"
# También: opción 7 para generar reporte completo
```

**Salida esperada:**

```
  Zona: Cumbaya  (6 registros usados, alpha=0.8)
  Contam.     Pred.24h   LimOMS24h  Estado
  ----------------------------------------------------------
  PM2.5         44.38      15.0   SUPERA LIMITE [!]
  PM10          91.89      45.0   SUPERA LIMITE [!]
  NO2           45.51      25.0   SUPERA LIMITE [!]
  O3           122.73     100.0   SUPERA LIMITE [!]
  SO2           61.64      40.0   SUPERA LIMITE [!]
  CO             5.99       4.0   SUPERA LIMITE [!]

  [!] Alertas detectadas (6):
    [CRITICA] PM2.5: 44.38 ug/m3 → Límite 15.0
    Rec: Reducir trafico vehicular. Emitir comunicado publico...
    [CRITICA] PM10: 91.89 ug/m3 → Límite 45.0
    ...
```

---

## Algoritmo de predicción

### Método: Promedio Ponderado Exponencial (Suavización Exponencial Simple — SES)

**Parámetros:**
- `N` = mínimo(registros disponibles, 7) — ventana de días
- `α` = 0.8 — factor de decaimiento

**Fórmula:**

```
Sea r_0 el registro más reciente, r_1 el anterior, ..., r_{N-1} el más antiguo.

Peso del registro i:    w_i = α^i    (ejemplo: w_0=1.0, w_1=0.8, w_2=0.64, ...)

Predicción contaminante c:
  pred_c = Σ(r_i[c] * w_i) / Σ(w_i)    para i = 0..N-1
```

---

## Límites OMS utilizados

| Contaminante | Límite 24h | Límite Anual | Unidad |
|---|---|---|---|
| PM2.5 | 15 | 5 | µg/m³ |
| PM10 | 45 | 15 | µg/m³ |
| NO2 | 25 | 10 | µg/m³ |
| O3 | 100 | 60 | µg/m³ |
| SO2 | 40 | — | µg/m³ |
| CO | 4 | — | mg/m³ |

Fuente: *WHO Global Air Quality Guidelines 2021*.

**Niveles de severidad:**

| Exceso sobre límite | Nivel |
|---|---|
| 0 – 20 % | ADVERTENCIA |
| 20 – 50 % | ALERTA |
| > 50 % | CRÍTICA |

---

## Decisiones de diseño

| Decisión | Valor elegido | Justificación |
|---|---|---|
| Máximo de zonas | 10 | Suficiente para una ciudad; cabe en stack sin memoria dinámica |
| Máximo de registros por zona | 100 | Cubre varios meses de lecturas diarias; almacenado en arreglo estático |
| Días para promedio histórico | 30 | Mes calendario; estándar en monitoreo ambiental |
| Días para predicción | 7 | Semana; ventana óptima para SES sin ruido |
| Alpha de decaimiento | 0.8 | Balance entre sensibilidad reciente y estabilidad |
| Máximo intentos de entrada | 5 | Evita bucles infinitos ante entradas incorrectas |
| Separador CSV | coma `,` | Compatible con la mayoría de herramientas (Excel, pandas) |
| Encoding del archivo | UTF-8 | Soporta tildes y ñ en nombres de zonas |

---

## Cómo ejecutar los 3 casos de prueba

```bash
# 1. Compilar
make

# 2. Ejecutar pruebas unitarias
make test                          # Verifica predicción y alertas con datos controlados

# 3. Caso A — sin alertas (La Floresta)
./aire_urbano                      # datos_ejemplo.txt se carga automáticamente
# → Opción 6 → [5] La Floresta → ver predicción dentro de límites

# 4. Caso B — alertas moderadas (La Mariscal)
# → Opción 6 → [2] La Mariscal → ver ALERTA en PM2.5

# 5. Caso C — niveles críticos (Cumbayá)
# → Opción 6 → [3] Cumbaya → ver CRITICA en todos los contaminantes
# → Opción 7 → generar reporte PDF con las 3 zonas comparadas
```