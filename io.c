// Archivo: io.c
// Proposito: Lectura y escritura de archivos de historial, datos de ejemplo
//            y generacion de reportes en texto plano.
// Estandar:  C99

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "io.h"
#include "monitor.h"
#include "utils.h"

/* Numero maximo de campos por linea del CSV de historial */
#define MAX_CAMPOS_CSV  (3 + NUM_CONTAMINANTES)   /* fecha,hora,zona + 6 contam. = 9 */

/* Maximo de alertas por zona al generar reporte */
#define MAX_ALERTAS_ZONA  20

// -----------------------------------------------------------------------------
//  PARSING DE LINEA
// -----------------------------------------------------------------------------

int parsear_linea_historial(char *linea, char *zona_nombre, Medicion *m) {
    char  copia[512];
    char *campos[MAX_CAMPOS_CSV + 2];   /* margen extra */
    int   n_campos, j;
    char  tmp[32];
    double val;
    char  *endptr;

    if (!linea || !zona_nombre || !m) return 0;

    strncpy(copia, linea, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';
    trim(copia);

    if (copia[0] == '\0' || copia[0] == '#') return 0;

    n_campos = separar_campos(copia, ',', campos, MAX_CAMPOS_CSV + 2);

    if (n_campos < (int)(3 + NUM_CONTAMINANTES)) {
        return 0;   /* Faltan campos */
    }

    /* Campo 0: fecha */
    trim(campos[0]);
    if (strlen(campos[0]) != 10 || campos[0][4] != '-' || campos[0][7] != '-') {
        return 0;
    }
    strncpy(m->fecha, campos[0], sizeof(m->fecha) - 1);
    m->fecha[sizeof(m->fecha) - 1] = '\0';

    /* Campo 1: hora */
    trim(campos[1]);
    if (strlen(campos[1]) != 5 || campos[1][2] != ':') {
        return 0;
    }
    strncpy(m->hora, campos[1], sizeof(m->hora) - 1);
    m->hora[sizeof(m->hora) - 1] = '\0';

    /* Campo 2: nombre de zona */
    trim(campos[2]);
    if (strlen(campos[2]) < 1) return 0;
    strncpy(zona_nombre, campos[2], MAX_NOMBRE - 1);
    zona_nombre[MAX_NOMBRE - 1] = '\0';

    /* Campos 3..8: valores de contaminantes */
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        strncpy(tmp, campos[3 + j], sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        trim(tmp);
        normalizar_decimal(tmp);

        val = strtod(tmp, &endptr);
        if (*endptr != '\0' || val < 0.0) {
            return 0;   /* Valor invalido */
        }
        m->valores[j] = val;
    }

    return 1;
}

// -----------------------------------------------------------------------------
//  CARGA DE HISTORIAL
// -----------------------------------------------------------------------------

static Zona *buscar_o_crear_zona(Sistema *s, const char *nombre) {
    int i;

    for (i = 0; i < s->num_zonas; i++) {
        if (strcmp(s->zonas[i].nombre, nombre) == 0) {
            return &s->zonas[i];
        }
    }

    if (agregar_zona(s, nombre) != 0) return NULL;
    return &s->zonas[s->num_zonas - 1];
}

int cargar_historial(Sistema *s, const char *ruta,
                     int *leidos, int *errores) {
    FILE  *f;
    char   linea[512];
    char   zona_nombre[MAX_NOMBRE];
    Medicion m;
    int    n_linea = 0;
    Zona  *z;

    if (!s || !ruta) return -1;

    *leidos  = 0;
    *errores = 0;

    f = fopen(ruta, "r");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir '%s' para lectura.\n", ruta);
        return -1;
    }

    while (fgets(linea, sizeof(linea), f)) {
        n_linea++;

        /* Descartar exceso de linea si fue truncada */
        {
            size_t len = strlen(linea);
            if (len > 0 && linea[len - 1] != '\n') {
                int c;
                while ((c = fgetc(f)) != '\n' && c != EOF)
                    ;
            }
        }

        trim(linea);

        if (linea[0] == '\0' || linea[0] == '#') continue;

        memset(&m, 0, sizeof(m));
        zona_nombre[0] = '\0';

        if (!parsear_linea_historial(linea, zona_nombre, &m)) {
            fprintf(stderr,
                    "[AVISO] Linea %d: formato invalido, omitida.\n", n_linea);
            (*errores)++;
            continue;
        }

        z = buscar_o_crear_zona(s, zona_nombre);
        if (!z) {
            fprintf(stderr,
                    "[AVISO] Linea %d: no se pudo crear zona '%s'. Sistema lleno?\n",
                    n_linea, zona_nombre);
            (*errores)++;
            continue;
        }

        if (registrar_medicion(z, &m) == 0) {
            (*leidos)++;
        } else {
            fprintf(stderr,
                    "[AVISO] Linea %d: historial de zona '%s' lleno, registro omitido.\n",
                    n_linea, zona_nombre);
            (*errores)++;
        }
    }

    fclose(f);
    return 0;
}

int cargar_datos_ejemplo(Sistema *s) {
    int leidos, errores;
    int ret;

    ret = cargar_historial(s, "datos_ejemplo.txt", &leidos, &errores);
    if (ret == 0) {
        printf("[INFO] datos_ejemplo.txt: %d registros cargados, %d errores.\n",
               leidos, errores);
    }
    return ret;
}

// -----------------------------------------------------------------------------
//  GUARDADO DE HISTORIAL
// -----------------------------------------------------------------------------

int guardar_historial(const Sistema *s, int forzar) {
    FILE  *f;
    int    i, k, j, total = 0;

    if (!s) return -1;

    if (!forzar) {
        f = fopen(ARCHIVO_HISTORIAL, "r");
        if (f) {
            fclose(f);
            if (!confirmar("El archivo 'historial.txt' ya existe. Desea sobrescribirlo?")) {
                printf("[INFO] Guardado cancelado por el usuario.\n");
                return -2;
            }
        }
    }

    f = fopen(ARCHIVO_HISTORIAL, "w");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo abrir '%s' para escritura.\n",
                ARCHIVO_HISTORIAL);
        return -1;
    }

    /* Encabezado del archivo */
    fprintf(f, "# Sistema de Monitoreo de Calidad del Aire - Quito\n");
    fprintf(f, "# Formato: YYYY-MM-DD,HH:MM,Zona,PM2.5,PM10,NO2,O3,SO2,CO\n");
    fprintf(f, "# Generado automaticamente. No editar manualmente.\n");

    for (i = 0; i < s->num_zonas; i++) {
        const Zona *z = &s->zonas[i];
        for (k = 0; k < z->num_registros; k++) {
            const Medicion *m = &z->historial[k];
            fprintf(f, "%s,%s,%s",
                    m->fecha, m->hora, z->nombre);
            for (j = 0; j < NUM_CONTAMINANTES; j++) {
                fprintf(f, ",%.2f", m->valores[j]);
            }
            fprintf(f, "\n");
            total++;
        }
    }

    fclose(f);
    printf("[INFO] %d registros guardados en '%s'.\n", total, ARCHIVO_HISTORIAL);
    return total;
}

// -----------------------------------------------------------------------------
//  GENERACION DE REPORTE
// -----------------------------------------------------------------------------

int generar_reporte(const Sistema *s, char *ruta_salida) {
    FILE    *f;
    char     fecha[11], hora[6];
    char     nombre_archivo[MAX_RUTA_ARCHIVO];
    int      i, j, k;
    double   promedios[NUM_CONTAMINANTES];
    double   prediccion[NUM_CONTAMINANTES];
    Alerta   alertas[MAX_ALERTAS_ZONA];
    int      n_alertas, n_prom, n_pred;
    char     buf_rec[512];
    int      total_alertas_global = 0;

    if (!s) return -1;

    obtener_fecha_hora_actual(fecha, hora);

    /* Construir nombre del archivo: reporte_YYYYMMDD_HHMM.txt */
    snprintf(nombre_archivo, sizeof(nombre_archivo),
             "reporte_%s_%s.txt",
             fecha,     /* "YYYY-MM-DD" */
             hora);     /* "HH:MM" */

    /* Formato final: reporte_YYYYMMDD_HHMM.txt */
    {
        char tmp[MAX_RUTA_ARCHIVO];
        char fecha_sin_guion[9];   /* YYYYMMDD */
        char hora_sin_dos_puntos[5]; /* HHMM */

        k = 0;
        for (i = 0; fecha[i]; i++)
            if (fecha[i] != '-') fecha_sin_guion[k++] = fecha[i];
        fecha_sin_guion[k] = '\0';

        k = 0;
        for (i = 0; hora[i]; i++)
            if (hora[i] != ':') hora_sin_dos_puntos[k++] = hora[i];
        hora_sin_dos_puntos[k] = '\0';

        snprintf(tmp, sizeof(tmp), "reporte_%s_%s.txt",
                 fecha_sin_guion, hora_sin_dos_puntos);
        strncpy(nombre_archivo, tmp, MAX_RUTA_ARCHIVO - 1);
        nombre_archivo[MAX_RUTA_ARCHIVO - 1] = '\0';
    }

    if (ruta_salida)
        strncpy(ruta_salida, nombre_archivo, MAX_RUTA_ARCHIVO - 1);

    f = fopen(nombre_archivo, "w");
    if (!f) {
        fprintf(stderr, "[ERROR] No se pudo crear el reporte '%s'.\n",
                nombre_archivo);
        return -1;
    }

    for (k = 0; k < 70; k++) fprintf(f, "=");
    fprintf(f, "\n");
    fprintf(f,
            "  REPORTE DE CALIDAD DEL AIRE - SISTEMA INTEGRAL DE MONITOREO\n");
    fprintf(f, "  Municipio de Quito, Ecuador\n");
    fprintf(f, "  Fecha: %s  Hora: %s\n", fecha, hora);
    fprintf(f, "  Zonas monitoreadas: %d\n", s->num_zonas);
    for (k = 0; k < 70; k++) fprintf(f, "=");
    fprintf(f, "\n\n");

    fprintf(f, "  Limites OMS de referencia (24h):\n");
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        fprintf(f, "    %-8s : %.1f %s\n",
                NOMBRES_CONTAMINANTES[j],
                LIMITE_OMS_24H[j],
                UNIDADES_CONTAMINANTES[j]);
    }
    fprintf(f, "\n");

    for (i = 0; i < s->num_zonas; i++) {
        const Zona *z = &s->zonas[i];

        for (k = 0; k < 70; k++) fprintf(f, "-");
        fprintf(f, "\n");
        fprintf(f, "  ZONA: %s\n", z->nombre);
        fprintf(f, "  Registros historicos: %d\n\n", z->num_registros);

        fprintf(f, "  %-10s %-12s %-12s %-12s %-10s\n",
                "Contam.", "Prom.30d", "Val.Actual", "Pred.24h", "LimOMS24h");
        for (k = 0; k < 66; k++) fprintf(f, "-");
        fprintf(f, "\n");

        n_prom = calcular_promedio_historico(z, promedios);
        n_pred = predecir_24h(z, prediccion);

        for (j = 0; j < NUM_CONTAMINANTES; j++) {
            char marca_prom[4]  = "   ";
            char marca_act[4]   = "   ";
            char marca_pred[4]  = "   ";

            if (n_prom > 0 && promedios[j]         > LIMITE_OMS_24H[j]) strcpy(marca_prom, "[!]");
            if (z->tiene_medicion_actual &&
                z->ultima_medicion[j]               > LIMITE_OMS_24H[j]) strcpy(marca_act,  "[!]");
            if (n_pred > 0 && prediccion[j]         > LIMITE_OMS_24H[j]) strcpy(marca_pred, "[!]");

            fprintf(f, "  %-10s", NOMBRES_CONTAMINANTES[j]);

            if (n_prom > 0)
                fprintf(f, " %-9.2f%s", promedios[j], marca_prom);
            else
                fprintf(f, " %-12s", "N/D");

            if (z->tiene_medicion_actual)
                fprintf(f, " %-9.2f%s", z->ultima_medicion[j], marca_act);
            else
                fprintf(f, " %-12s", "N/D");

            if (n_pred > 0)
                fprintf(f, " %-9.2f%s", prediccion[j], marca_pred);
            else
                fprintf(f, " %-12s", "N/D");

            fprintf(f, " %-10.1f\n", LIMITE_OMS_24H[j]);
        }

        n_alertas = generar_alertas(z, prediccion, alertas, MAX_ALERTAS_ZONA);
        total_alertas_global += n_alertas;

        if (n_alertas > 0) {
            fprintf(f, "\n  *** ALERTAS (%d) ***\n", n_alertas);
            for (k = 0; k < n_alertas; k++) {
                recomendacion_alerta(&alertas[k], buf_rec, sizeof(buf_rec));
                fprintf(f,
                        "  [%s] %s %s - Valor: %.2f %s "
                        "(Limite OMS: %.1f) | %s\n"
                        "  Recomendacion: %s\n",
                        severidad_a_texto(alertas[k].severidad),
                        NOMBRES_CONTAMINANTES[alertas[k].contaminante],
                        alertas[k].es_prediccion ? "(PRED)" : "(ACTUAL)",
                        alertas[k].valor,
                        UNIDADES_CONTAMINANTES[alertas[k].contaminante],
                        alertas[k].limite,
                        z->nombre,
                        buf_rec);
            }
        } else {
            fprintf(f, "\n  Sin alertas activas para esta zona.\n");
        }

        fprintf(f, "\n");
    }

    for (k = 0; k < 70; k++) fprintf(f, "=");
    fprintf(f, "\n");
    fprintf(f, "  RESUMEN GLOBAL\n");
    fprintf(f, "  Total de alertas generadas: %d\n", total_alertas_global);
    fprintf(f, "  [!] = Supera limite OMS 24h\n");
    fprintf(f, "\n  Fin del reporte. Generado el %s a las %s.\n", fecha, hora);
    for (k = 0; k < 70; k++) fprintf(f, "=");
    fprintf(f, "\n");

    fclose(f);
    printf("[INFO] Reporte generado: '%s'\n", nombre_archivo);
    return 0;
}
