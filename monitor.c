// Archivo: monitor.c
// Proposito: Implementacion de la logica central: inicializacion del sistema,
//            gestion de zonas, calculo de promedios historicos, prediccion
//            ponderada exponencial y generacion de alertas.
// Estandar:  C99

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "monitor.h"
#include "utils.h"

// -----------------------------------------------------------------------------
//  DATOS GLOBALES CONSTANTES
// -----------------------------------------------------------------------------

/* Nombres de contaminantes en orden (IDX_PM25 ... IDX_CO) */
const char *NOMBRES_CONTAMINANTES[NUM_CONTAMINANTES] = {
    "PM2.5", "PM10", "NO2", "O3", "SO2", "CO"
};

/* Unidades de cada contaminante */
const char *UNIDADES_CONTAMINANTES[NUM_CONTAMINANTES] = {
    "ug/m3", "ug/m3", "ug/m3", "ug/m3", "ug/m3", "mg/m3"
};

/*
 * Limites OMS para media de 24 horas (o equivalente periodico mas restrictivo).
 * Fuentes: OMS Guias de Calidad del Aire 2021.
 */
const double LIMITE_OMS_24H[NUM_CONTAMINANTES] = {
    15.0,   /* PM2.5 */
    45.0,   /* PM10  */
    25.0,   /* NO2   */
    100.0,  /* O3    */
    40.0,   /* SO2   */
    4.0     /* CO    */
};

/*
 * Limites OMS para media anual.
 * -1.0 indica que la OMS no especifica limite anual para ese contaminante.
 */
const double LIMITE_OMS_ANUAL[NUM_CONTAMINANTES] = {
    5.0,    /* PM2.5 */
    15.0,   /* PM10  */
    10.0,   /* NO2   */
    60.0,   /* O3    */
    -1.0,   /* SO2   - no aplica */
    -1.0    /* CO    - no aplica */
};

/* Factor de decaimiento para prediccion exponencial. */
#define ALPHA_PREDICCION  0.8

/* Zonas predeterminadas de Quito */
static const char *ZONAS_DEFECTO[] = {
    "Centro Historico",
    "La Mariscal",
    "Cumbaya",
    "La Carolina",
    "La Floresta"
};
#define NUM_ZONAS_DEFECTO  5

// -----------------------------------------------------------------------------
//  INICIALIZACION
// -----------------------------------------------------------------------------

void inicializar_sistema(Sistema *s) {
    int i, j;

    if (!s) return;

    s->num_zonas = 0;
    memset(s->zonas, 0, sizeof(s->zonas));

    /* Anadir zonas predeterminadas */
    for (i = 0; i < NUM_ZONAS_DEFECTO; i++) {
        Zona *z = &s->zonas[i];
        strncpy(z->nombre, ZONAS_DEFECTO[i], MAX_NOMBRE - 1);
        z->nombre[MAX_NOMBRE - 1] = '\0';
        z->num_registros = 0;
        z->tiene_medicion_actual = 0;
        for (j = 0; j < NUM_CONTAMINANTES; j++) {
            z->ultima_medicion[j] = 0.0;
        }
    }
    s->num_zonas = NUM_ZONAS_DEFECTO;
}

// -----------------------------------------------------------------------------
//  GESTION DE ZONAS
// -----------------------------------------------------------------------------

int agregar_zona(Sistema *s, const char *nombre) {
    Zona *z;
    int j;

    if (!s || !nombre) return -1;
    if (s->num_zonas >= MAX_ZONAS) {
        fprintf(stderr, "[ERROR] Capacidad maxima de zonas (%d) alcanzada.\n",
                MAX_ZONAS);
        return -1;
    }

    z = &s->zonas[s->num_zonas];
    memset(z, 0, sizeof(Zona));
    strncpy(z->nombre, nombre, MAX_NOMBRE - 1);
    z->nombre[MAX_NOMBRE - 1] = '\0';
    z->num_registros = 0;
    z->tiene_medicion_actual = 0;
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        z->ultima_medicion[j] = 0.0;
    }

    s->num_zonas++;
    return 0;
}

// -----------------------------------------------------------------------------
//  REGISTRO DE MEDICIONES
// -----------------------------------------------------------------------------

int registrar_medicion(Zona *z, const Medicion *m) {
    int j;

    if (!z || !m) return -1;
    if (z->num_registros >= MAX_REGISTROS) {
        fprintf(stderr,
                "[AVISO] Historial lleno en zona '%s'. Registro descartado.\n",
                z->nombre);
        return -1;
    }

    /* Copiar la medicion al historial */
    z->historial[z->num_registros] = *m;
    z->num_registros++;

    /* Actualizar medicion actual */
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        z->ultima_medicion[j] = m->valores[j];
    }
    z->tiene_medicion_actual = 1;

    return 0;
}

// -----------------------------------------------------------------------------
//  CALCULO DE PROMEDIO HISTORICO
// -----------------------------------------------------------------------------

int calcular_promedio_historico(const Zona *z,
                                double promedios[NUM_CONTAMINANTES]) {
    int i, j;
    int inicio, count;
    double suma[NUM_CONTAMINANTES];

    if (!z || !promedios) return 0;

    /* Inicializar acumuladores */
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        suma[j] = 0.0;
        promedios[j] = 0.0;
    }

    if (z->num_registros == 0) return 0;

    /* Tomar los ultimos DIAS_PROMEDIO registros */
    count = MIN(z->num_registros, DIAS_PROMEDIO);
    inicio = z->num_registros - count;  /* indice del primer registro a incluir */

    for (i = inicio; i < z->num_registros; i++) {
        for (j = 0; j < NUM_CONTAMINANTES; j++) {
            suma[j] += z->historial[i].valores[j];
        }
    }

    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        promedios[j] = suma[j] / count;
    }

    return count;
}

// -----------------------------------------------------------------------------
//  PREDICCION PONDERADA EXPONENCIAL 24H
// -----------------------------------------------------------------------------

int predecir_24h(const Zona *z, double prediccion[NUM_CONTAMINANTES]) {
    int i, j;
    int count, idx;
    double peso, suma_pesos;
    double suma[NUM_CONTAMINANTES];

    if (!z || !prediccion) return 0;

    /* Inicializar */
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        suma[j] = 0.0;
        prediccion[j] = 0.0;
    }

    if (z->num_registros == 0) return 0;

    /* Usar los ultimos DIAS_PREDICCION registros */
    count  = MIN(z->num_registros, DIAS_PREDICCION);
    suma_pesos = 0.0;

    /* i=0 -> registro mas reciente (indice historial: num_registros-1) */
    for (i = 0; i < count; i++) {
        idx  = z->num_registros - 1 - i;   /* indice en el arreglo historial */
        peso = pow(ALPHA_PREDICCION, (double)i);
        suma_pesos += peso;

        for (j = 0; j < NUM_CONTAMINANTES; j++) {
            suma[j] += z->historial[idx].valores[j] * peso;
        }
    }

    /* Normalizar */
    if (suma_pesos > 0.0) {
        for (j = 0; j < NUM_CONTAMINANTES; j++) {
            prediccion[j] = suma[j] / suma_pesos;
        }
    }

    return count;
}

// -----------------------------------------------------------------------------
//  GENERACION DE ALERTAS
// -----------------------------------------------------------------------------

static Severidad calcular_severidad(double valor, double limite) {
    double exceso_pct;

    if (limite <= 0.0) return SEV_ADVERTENCIA;
    exceso_pct = (valor - limite) / limite * 100.0;

    if (exceso_pct > 50.0) return SEV_CRITICA;
    if (exceso_pct > 20.0) return SEV_ALERTA;
    return SEV_ADVERTENCIA;
}

int generar_alertas(const Zona *z,
                    const double prediccion[NUM_CONTAMINANTES],
                    Alerta alertas[], int max_alertas) {
    int j, n = 0;
    double val, lim;

    if (!z || !prediccion || !alertas || max_alertas <= 0) return 0;

    for (j = 0; j < NUM_CONTAMINANTES && n < max_alertas; j++) {
        lim = LIMITE_OMS_24H[j];

        /* -- Evaluacion de valor actual -- */
        if (z->tiene_medicion_actual) {
            val = z->ultima_medicion[j];
            if (val > lim) {
                strncpy(alertas[n].zona, z->nombre, MAX_NOMBRE - 1);
                alertas[n].zona[MAX_NOMBRE - 1] = '\0';
                alertas[n].contaminante  = j;
                alertas[n].valor         = val;
                alertas[n].limite        = lim;
                alertas[n].severidad     = calcular_severidad(val, lim);
                alertas[n].es_prediccion = 0;
                n++;
                if (n >= max_alertas) break;
            }
        }

        /* -- Evaluacion de prediccion 24h -- */
        val = prediccion[j];
        if (val > lim) {
            strncpy(alertas[n].zona, z->nombre, MAX_NOMBRE - 1);
            alertas[n].zona[MAX_NOMBRE - 1] = '\0';
            alertas[n].contaminante  = j;
            alertas[n].valor         = val;
            alertas[n].limite        = lim;
            alertas[n].severidad     = calcular_severidad(val, lim);
            alertas[n].es_prediccion = 1;
            n++;
        }
    }

    return n;
}

const char *severidad_a_texto(Severidad sev) {
    switch (sev) {
        case SEV_ADVERTENCIA: return "ADVERTENCIA";
        case SEV_ALERTA:      return "ALERTA";
        case SEV_CRITICA:     return "CRITICA";
        default:              return "DESCONOCIDA";
    }
}

void recomendacion_alerta(const Alerta *a, char *buf, int tam) {
    const char *base = "";

    if (!a || !buf || tam <= 0) return;

    /* Recomendaciones base por contaminante */
    switch (a->contaminante) {
        case IDX_PM25:
        case IDX_PM10:
            base = "Reducir trafico vehicular. Suspender quemas a cielo abierto.";
            break;
        case IDX_NO2:
            base = "Restringir circulacion de vehiculos diesel. "
                   "Aumentar ventilacion en espacios cerrados.";
            break;
        case IDX_O3:
            base = "Suspender actividades fisicas al aire libre en horas pico. "
                   "Reducir uso de solventes y emisiones VOC.";
            break;
        case IDX_SO2:
            base = "Revisar emisiones industriales. "
                   "Limitar actividades en zonas de influencia de fuentes fijas.";
            break;
        case IDX_CO:
            base = "Evitar encendido de motores en espacios cerrados. "
                   "Mejorar ventilacion en areas urbanas densas.";
            break;
        default:
            base = "Revisar fuentes de emision locales.";
    }

    /* Anadir recomendacion adicional segun severidad */
    if (a->severidad == SEV_CRITICA) {
        snprintf(buf, (size_t)tam,
                 "%s Emitir comunicado publico y evaluar medidas de emergencia.",
                 base);
    } else if (a->severidad == SEV_ALERTA) {
        snprintf(buf, (size_t)tam,
                 "%s Informar a la poblacion y reforzar monitoreo.", base);
    } else {
        snprintf(buf, (size_t)tam, "%s Mantener monitoreo continuo.", base);
    }
}

// -----------------------------------------------------------------------------
//  VISUALIZACION EN CONSOLA
// -----------------------------------------------------------------------------

void mostrar_zona(const Zona *z) {
    int j;

    if (!z) return;

    printf("\n  Zona: %s\n", z->nombre);
    printf("  Registros historicos: %d\n", z->num_registros);

    if (z->tiene_medicion_actual) {
        printf("  Ultima medicion:\n");
        for (j = 0; j < NUM_CONTAMINANTES; j++) {
            printf("    %-8s: %8.2f %s",
                   NOMBRES_CONTAMINANTES[j],
                   z->ultima_medicion[j],
                   UNIDADES_CONTAMINANTES[j]);
            if (z->ultima_medicion[j] > LIMITE_OMS_24H[j]) {
                printf(" [!] SUPERA LIMITE OMS (%.1f %s)",
                       LIMITE_OMS_24H[j],
                       UNIDADES_CONTAMINANTES[j]);
            }
            printf("\n");
        }
    } else {
        printf("  Sin medicion actual registrada.\n");
    }
}

void mostrar_sistema(const Sistema *s) {
    int i;

    if (!s) return;

    printf("\n");
    imprimir_separador('=', 60);
    printf("\n  SISTEMA DE MONITOREO - ZONAS DE QUITO\n");
    printf("  Total de zonas: %d / %d\n", s->num_zonas, MAX_ZONAS);
    imprimir_separador('=', 60);

    if (s->num_zonas == 0) {
        printf("  No hay zonas registradas.\n");
        return;
    }

    for (i = 0; i < s->num_zonas; i++) {
        printf("\n  [%d] ", i + 1);
        mostrar_zona(&s->zonas[i]);
        imprimir_separador('-', 60);
    }
}

// -----------------------------------------------------------------------------
//  PRUEBAS UNITARIAS
// -----------------------------------------------------------------------------

int test_prediccion(void) {
    Zona z;
    double pred[NUM_CONTAMINANTES];
    int fallos = 0;
    double esperado, tolerancia;
    int n;

    memset(&z, 0, sizeof(Zona));
    strncpy(z.nombre, "TEST", MAX_NOMBRE - 1);

    /* Registro 1 (mas antiguo) */
    z.historial[0].valores[IDX_PM25] = 10.0;
    /* Registro 2 */
    z.historial[1].valores[IDX_PM25] = 12.0;
    /* Registro 3 (mas reciente) */
    z.historial[2].valores[IDX_PM25] = 14.0;
    z.num_registros = 3;

    n = predecir_24h(&z, pred);

    if (n != 3) {
        printf("[TEST PREDICCION] FALLO: se esperaban 3 registros, se obtuvieron %d\n", n);
        fallos++;
    }

    esperado   = (14.0 * 1.0 + 12.0 * 0.8 + 10.0 * 0.64) / (1.0 + 0.8 + 0.64);
    tolerancia = 0.01;

    if (fabs(pred[IDX_PM25] - esperado) > tolerancia) {
        printf("[TEST PREDICCION] FALLO PM2.5: esperado=%.4f obtenido=%.4f\n",
               esperado, pred[IDX_PM25]);
        fallos++;
    } else {
        printf("[TEST PREDICCION] PASS PM2.5: prediccion=%.4f (esperado=%.4f)\n",
               pred[IDX_PM25], esperado);
    }

    return fallos;
}

int test_alertas(void) {
    Zona z;
    Alerta alertas[20];
    double pred[NUM_CONTAMINANTES];
    int n_alertas, fallos = 0;

    memset(&z, 0, sizeof(Zona));
    memset(pred, 0, sizeof(pred));
    strncpy(z.nombre, "TEST", MAX_NOMBRE - 1);

    /* -- Caso A: sin alerta -- */
    z.ultima_medicion[IDX_PM25] = 14.0;
    z.tiene_medicion_actual = 1;
    pred[IDX_PM25] = 14.0;

    n_alertas = generar_alertas(&z, pred, alertas, 20);
    if (n_alertas != 0) {
        printf("[TEST ALERTAS] Caso A FALLO: esperadas 0 alertas, obtenidas %d\n",
               n_alertas);
        fallos++;
    } else {
        printf("[TEST ALERTAS] Caso A PASS: sin alertas para PM2.5=14.0\n");
    }

    /* -- Caso B: ADVERTENCIA -- */
    z.ultima_medicion[IDX_PM25] = 16.0;   /* 6.7% sobre limite */
    pred[IDX_PM25] = 16.0;

    n_alertas = generar_alertas(&z, pred, alertas, 20);
    if (n_alertas < 1 || alertas[0].severidad != SEV_ADVERTENCIA) {
        printf("[TEST ALERTAS] Caso B FALLO: se esperaba ADVERTENCIA, obtenidas %d alertas\n", n_alertas);
        fallos++;
    } else {
        printf("[TEST ALERTAS] Caso B PASS: ADVERTENCIA para PM2.5=16.0\n");
    }

    /* -- Caso C: ALERTA -- */
    z.ultima_medicion[IDX_PM25] = 19.0;   /* 26.7% sobre limite */
    pred[IDX_PM25] = 19.0;

    n_alertas = generar_alertas(&z, pred, alertas, 20);
    if (n_alertas < 1 || alertas[0].severidad != SEV_ALERTA) {
        printf("[TEST ALERTAS] Caso C FALLO: se esperaba ALERTA\n");
        fallos++;
    } else {
        printf("[TEST ALERTAS] Caso C PASS: ALERTA para PM2.5=19.0\n");
    }

    /* -- Caso D: CRITICA -- */
    z.ultima_medicion[IDX_PM25] = 28.0;   /* 86.7% sobre limite */
    pred[IDX_PM25] = 28.0;

    n_alertas = generar_alertas(&z, pred, alertas, 20);
    if (n_alertas < 1 || alertas[0].severidad != SEV_CRITICA) {
        printf("[TEST ALERTAS] Caso D FALLO: se esperaba CRITICA\n");
        fallos++;
    } else {
        printf("[TEST ALERTAS] Caso D PASS: CRITICA para PM2.5=28.0\n");
    }

    return fallos;
}
