// Archivo: main.c
// Proposito: Punto de entrada del programa. Gestiona el menu principal
//            interactivo y coordina las llamadas a los modulos monitor, io y utils.
// Estandar:  C99
//
// Uso:
//   ./aire_urbano              -> modo interactivo
//   ./aire_urbano --test       -> ejecutar pruebas unitarias y salir
//   ./aire_urbano --cargar <archivo> -> carga historial y abre menu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "monitor.h"
#include "io.h"
#include "utils.h"

// -----------------------------------------------------------------------------
//  OPCIONES DE MENU
// -----------------------------------------------------------------------------

#define OPT_MOSTRAR_ZONAS        1
#define OPT_AGREGAR_ZONA         2
#define OPT_REGISTRAR_MEDICION   3
#define OPT_CARGAR_HISTORIAL     4
#define OPT_PROMEDIOS_HISTORICOS 5
#define OPT_PREDECIR_24H         6
#define OPT_GENERAR_REPORTE      7
#define OPT_GUARDAR_HISTORIAL    8
#define OPT_SALIR                0

/* Maximo de alertas mostradas en pantalla por sesion */
#define MAX_ALERTAS_MENU  30

// -----------------------------------------------------------------------------
//  FUNCIONES AUXILIARES DEL MENU
// -----------------------------------------------------------------------------

/**
 * mostrar_menu - Imprime el menu principal.
 */
static void mostrar_menu(void) {
    printf("\n");
    imprimir_separador('=', 60);
    printf("  SISTEMA INTEGRAL DE MONITOREO DE CALIDAD DEL AIRE\n");
    printf("  Quito, Ecuador\n");
    imprimir_separador('=', 60);
    printf("  [%d] Mostrar zonas y datos actuales\n",   OPT_MOSTRAR_ZONAS);
    printf("  [%d] Anadir zona\n",                      OPT_AGREGAR_ZONA);
    printf("  [%d] Registrar medicion actual\n",        OPT_REGISTRAR_MEDICION);
    printf("  [%d] Cargar datos historicos desde archivo\n", OPT_CARGAR_HISTORIAL);
    printf("  [%d] Calcular promedios historicos (30 dias)\n", OPT_PROMEDIOS_HISTORICOS);
    printf("  [%d] Predecir niveles 24h por zona\n",    OPT_PREDECIR_24H);
    printf("  [%d] Generar reporte\n",                  OPT_GENERAR_REPORTE);
    printf("  [%d] Exportar/guardar historial\n",       OPT_GUARDAR_HISTORIAL);
    printf("  [%d] Salir\n",                            OPT_SALIR);
    imprimir_separador('-', 60);
}

/**
 * seleccionar_zona - Muestra la lista de zonas y permite seleccionar una.
 * @return Indice de la zona seleccionada (0-based), o -1 si se cancela.
 */
static int seleccionar_zona(const Sistema *s) {
    int i, opcion;

    if (s->num_zonas == 0) {
        printf("[INFO] No hay zonas registradas.\n");
        return -1;
    }

    printf("\n  Zonas disponibles:\n");
    for (i = 0; i < s->num_zonas; i++) {
        printf("    [%d] %s\n", i + 1, s->zonas[i].nombre);
    }

    if (!leer_entero_rango("  Seleccione zona (numero): ", 1, s->num_zonas, &opcion)) {
        return -1;
    }
    return opcion - 1;
}

// -----------------------------------------------------------------------------
//  MANEJADORES DE OPCIONES
// -----------------------------------------------------------------------------

/**
 * manejar_agregar_zona - Solicita nombre y anade una nueva zona.
 */
static void manejar_agregar_zona(Sistema *s) {
    char nombre[MAX_NOMBRE];
    int i;

    printf("\n  --- Anadir nueva zona ---\n");

    if (!leer_nombre("  Nombre de la nueva zona: ", nombre, MAX_NOMBRE)) {
        printf("[AVISO] Operacion cancelada.\n");
        return;
    }

    /* Verificar duplicados */
    for (i = 0; i < s->num_zonas; i++) {
        if (strcmp(s->zonas[i].nombre, nombre) == 0) {
            printf("[AVISO] Ya existe una zona con el nombre '%s'.\n", nombre);
            return;
        }
    }

    if (agregar_zona(s, nombre) == 0) {
        printf("[OK] Zona '%s' anadida (total: %d zonas).\n",
               nombre, s->num_zonas);
    } else {
        printf("[ERROR] No se pudo anadir la zona.\n");
    }
}

/**
 * manejar_registrar_medicion - Solicita zona, fecha/hora y valores de cada
 *   contaminante y registra la medicion.
 */
static void manejar_registrar_medicion(Sistema *s) {
    int    idx_zona, j;
    Medicion m;
    char   fecha_actual[11], hora_actual[6];
    char   buf_fecha[16], buf_hora[10];
    int    usar_fecha_actual;

    printf("\n  --- Registrar medicion ---\n");

    idx_zona = seleccionar_zona(s);
    if (idx_zona < 0) return;

    /* Fecha y hora */
    obtener_fecha_hora_actual(fecha_actual, hora_actual);
    printf("  Fecha/hora actual del sistema: %s %s\n", fecha_actual, hora_actual);
    usar_fecha_actual = confirmar("  Usar fecha y hora actual?");

    if (usar_fecha_actual) {
        strncpy(m.fecha, fecha_actual, sizeof(m.fecha) - 1);
        m.fecha[sizeof(m.fecha) - 1] = '\0';
        strncpy(m.hora, hora_actual, sizeof(m.hora) - 1);
        m.hora[sizeof(m.hora) - 1] = '\0';
    } else {
        int intentos;
        int valido;

        /* Fecha manual */
        valido = 0;
        for (intentos = 0; intentos < MAX_INTENTOS && !valido; intentos++) {
            printf("  Fecha (YYYY-MM-DD): ");
            if (!leer_linea(buf_fecha, sizeof(buf_fecha))) continue;
            trim(buf_fecha);
            if (strlen(buf_fecha) == 10 &&
                buf_fecha[4] == '-' && buf_fecha[7] == '-') {
                valido = 1;
                strncpy(m.fecha, buf_fecha, sizeof(m.fecha) - 1);
                m.fecha[sizeof(m.fecha) - 1] = '\0';
            } else {
                printf("[ERROR] Formato de fecha invalido. Use YYYY-MM-DD.\n");
            }
        }
        if (!valido) { printf("[AVISO] Operacion cancelada.\n"); return; }

        /* Hora manual */
        valido = 0;
        for (intentos = 0; intentos < MAX_INTENTOS && !valido; intentos++) {
            printf("  Hora (HH:MM): ");
            if (!leer_linea(buf_hora, sizeof(buf_hora))) continue;
            trim(buf_hora);
            if (strlen(buf_hora) == 5 && buf_hora[2] == ':') {
                valido = 1;
                strncpy(m.hora, buf_hora, sizeof(m.hora) - 1);
                m.hora[sizeof(m.hora) - 1] = '\0';
            } else {
                printf("[ERROR] Formato de hora invalido. Use HH:MM.\n");
            }
        }
        if (!valido) { printf("[AVISO] Operacion cancelada.\n"); return; }
    }

    /* Ingresar valores de contaminantes */
    printf("\n  Ingrese valores de contaminantes (>= 0):\n");
    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        char prompt[80];
        snprintf(prompt, sizeof(prompt), "    %s (%s): ",
                 NOMBRES_CONTAMINANTES[j], UNIDADES_CONTAMINANTES[j]);
        if (!leer_double_positivo(prompt, &m.valores[j])) {
            printf("[AVISO] Registro cancelado.\n");
            return;
        }
    }

    /* Registrar */
    if (registrar_medicion(&s->zonas[idx_zona], &m) == 0) {
        printf("[OK] Medicion registrada en zona '%s' (%s %s).\n",
               s->zonas[idx_zona].nombre, m.fecha, m.hora);

        /* Alertas imediatas */
        {
            Alerta alertas[MAX_ALERTAS_MENU];
            double pred[NUM_CONTAMINANTES];
            char   buf_rec[512];
            int    na, k;

            predecir_24h(&s->zonas[idx_zona], pred);
            na = generar_alertas(&s->zonas[idx_zona], pred,
                                 alertas, MAX_ALERTAS_MENU);
            if (na > 0) {
                printf("\n  [!] Se generaron %d alertas:\n", na);
                for (k = 0; k < na; k++) {
                    recomendacion_alerta(&alertas[k], buf_rec, sizeof(buf_rec));
                    printf("    [%s] %s %s: %.2f %s (Limite: %.1f)\n"
                           "    Rec: %s\n",
                           severidad_a_texto(alertas[k].severidad),
                           NOMBRES_CONTAMINANTES[alertas[k].contaminante],
                           alertas[k].es_prediccion ? "(pred)" : "(actual)",
                           alertas[k].valor,
                           UNIDADES_CONTAMINANTES[alertas[k].contaminante],
                           alertas[k].limite,
                           buf_rec);
                }
            } else {
                printf("  [OK] Todos los valores dentro de limites OMS.\n");
            }
        }
    } else {
        printf("[ERROR] No se pudo registrar la medicion.\n");
    }
}

/**
 * manejar_cargar_historial - Pide ruta de archivo y carga el historial.
 */
static void manejar_cargar_historial(Sistema *s) {
    char ruta[MAX_RUTA_ARCHIVO];
    int leidos, errores;

    printf("\n  --- Cargar historial desde archivo ---\n");
    printf("  (Presione Enter para usar '%s' o 'e' para datos_ejemplo.txt)\n",
           ARCHIVO_HISTORIAL);
    printf("  Ruta del archivo: ");

    if (!leer_linea(ruta, sizeof(ruta))) {
        printf("[AVISO] Operacion cancelada.\n");
        return;
    }
    trim(ruta);

    if (ruta[0] == '\0') {
        strncpy(ruta, ARCHIVO_HISTORIAL, sizeof(ruta) - 1);
    } else if (ruta[0] == 'e' || ruta[0] == 'E') {
        strncpy(ruta, "datos_ejemplo.txt", sizeof(ruta) - 1);
    }

    if (cargar_historial(s, ruta, &leidos, &errores) == 0) {
        printf("[OK] Archivo '%s': %d registros cargados, %d errores.\n",
               ruta, leidos, errores);
    } else {
        printf("[ERROR] No se pudo cargar el archivo '%s'.\n", ruta);
    }
}

/**
 * manejar_promedios_historicos - Muestra promedios de los ultimos 30 dias
 *   para la zona seleccionada y alerta si superan limites OMS.
 */
static void manejar_promedios_historicos(const Sistema *s) {
    int    idx_zona, j, n;
    double promedios[NUM_CONTAMINANTES];

    printf("\n  --- Promedios historicos (ultimos %d dias) ---\n", DIAS_PROMEDIO);

    idx_zona = seleccionar_zona(s);
    if (idx_zona < 0) return;

    n = calcular_promedio_historico(&s->zonas[idx_zona], promedios);

    if (n == 0) {
        printf("  Sin registros historicos para la zona '%s'.\n",
               s->zonas[idx_zona].nombre);
        return;
    }

    printf("\n  Zona: %s  (%d registros usados)\n",
           s->zonas[idx_zona].nombre, n);
    imprimir_separador('-', 60);
    printf("  %-8s  %10s  %10s  %s\n",
           "Contam.", "Promedio", "LimOMS24h", "Estado");
    imprimir_separador('-', 60);

    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        const char *estado =
            promedios[j] > LIMITE_OMS_24H[j] ? "SUPERA LIMITE [!]" : "OK";
        printf("  %-8s  %10.2f  %10.1f  %s\n",
               NOMBRES_CONTAMINANTES[j],
               promedios[j],
               LIMITE_OMS_24H[j],
               estado);
    }
}

/**
 * manejar_prediccion - Muestra la prediccion 24h para la zona seleccionada.
 */
static void manejar_prediccion(const Sistema *s) {
    int    idx_zona, j, n;
    double pred[NUM_CONTAMINANTES];
    Alerta alertas[MAX_ALERTAS_MENU];
    int    na;
    char   buf_rec[512];

    printf("\n  --- Prediccion de niveles 24h ---\n");

    idx_zona = seleccionar_zona(s);
    if (idx_zona < 0) return;

    n = predecir_24h(&s->zonas[idx_zona], pred);

    if (n == 0) {
        printf("  Sin registros para calcular prediccion en zona '%s'.\n",
               s->zonas[idx_zona].nombre);
        return;
    }

    printf("\n  Zona: %s  (%d registros usados, alpha=%.1f)\n",
           s->zonas[idx_zona].nombre, n, 0.8);
    printf("  Formula: Promedio ponderado exponencial (SES).\n");
    imprimir_separador('-', 60);
    printf("  %-8s  %10s  %10s  %s\n",
           "Contam.", "Pred.24h", "LimOMS24h", "Estado");
    imprimir_separador('-', 60);

    for (j = 0; j < NUM_CONTAMINANTES; j++) {
        const char *estado =
            pred[j] > LIMITE_OMS_24H[j] ? "SUPERA LIMITE [!]" : "OK";
        printf("  %-8s  %10.2f  %10.1f  %s\n",
               NOMBRES_CONTAMINANTES[j],
               pred[j],
               LIMITE_OMS_24H[j],
               estado);
    }

    /* Mostrar alertas generadas */
    na = generar_alertas(&s->zonas[idx_zona], pred,
                         alertas, MAX_ALERTAS_MENU);
    if (na > 0) {
        printf("\n  [!] Alertas detectadas (%d):\n", na);
        int k;
        for (k = 0; k < na; k++) {
            recomendacion_alerta(&alertas[k], buf_rec, sizeof(buf_rec));
            printf("    [%s] %s: %.2f %s -> Limite %.1f\n"
                   "    Rec: %s\n",
                   severidad_a_texto(alertas[k].severidad),
                   NOMBRES_CONTAMINANTES[alertas[k].contaminante],
                   alertas[k].valor,
                   UNIDADES_CONTAMINANTES[alertas[k].contaminante],
                   alertas[k].limite,
                   buf_rec);
        }
    } else {
        printf("\n  Prediccion dentro de limites OMS. Sin alertas.\n");
    }
}

/**
 * manejar_generar_reporte - Genera el reporte y muestra el nombre del archivo.
 */
static void manejar_generar_reporte(const Sistema *s) {
    char ruta[MAX_RUTA_ARCHIVO];

    printf("\n  --- Generar reporte ---\n");
    if (generar_reporte(s, ruta) == 0) {
        printf("[OK] Reporte disponible: %s\n", ruta);
    } else {
        printf("[ERROR] No se pudo generar el reporte.\n");
    }
}

// -----------------------------------------------------------------------------
//  MODO TEST
// -----------------------------------------------------------------------------

/**
 * ejecutar_tests - Ejecuta todas las pruebas unitarias y retorna el total
 *   de fallos.
 */
static int ejecutar_tests(void) {
    int fallos = 0;

    printf("\n");
    imprimir_separador('=', 60);
    printf("  PRUEBAS UNITARIAS\n");
    imprimir_separador('=', 60);

    printf("\n  [1] Test de prediccion ponderada:\n");
    fallos += test_prediccion();

    printf("\n  [2] Test de deteccion de alertas:\n");
    fallos += test_alertas();

    printf("\n");
    imprimir_separador('=', 60);
    if (fallos == 0) {
        printf("  RESULTADO: TODAS LAS PRUEBAS PASARON (0 fallos).\n");
    } else {
        printf("  RESULTADO: %d FALLO(S) DETECTADO(S).\n", fallos);
    }
    imprimir_separador('=', 60);
    printf("\n");

    return fallos;
}

// -----------------------------------------------------------------------------
//  MAIN
// -----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    Sistema s;
    int opcion;
    int leidos, errores;

    /* -- Modo test: ./aire_urbano --test -- */
    if (argc >= 2 && strcmp(argv[1], "--test") == 0) {
        int fallos = ejecutar_tests();
        return fallos > 0 ? 1 : 0;
    }

    /* -- Inicializar sistema con zonas de Quito -- */
    inicializar_sistema(&s);

    printf("\n");
    imprimir_separador('=', 60);
    printf("  Sistema de Monitoreo de Calidad del Aire - Quito\n");
    printf("  Version 1.0 | Iniciando...\n");
    imprimir_separador('=', 60);
    printf("  Zonas cargadas por defecto: %d\n\n", s.num_zonas);

    /* -- Carga opcional desde argumento: ./aire_urbano --cargar <archivo> -- */
    if (argc >= 3 && strcmp(argv[1], "--cargar") == 0) {
        printf("[INFO] Cargando historial desde '%s'...\n", argv[2]);
        if (cargar_historial(&s, argv[2], &leidos, &errores) == 0) {
            printf("[OK] %d registros cargados, %d errores.\n",
                   leidos, errores);
        } else {
            printf("[AVISO] No se pudo cargar '%s'. Continuando sin datos.\n",
                   argv[2]);
        }
    }

    /* -- Intentar cargar datos_ejemplo.txt automaticamente -- */
    {
        FILE *f = fopen("datos_ejemplo.txt", "r");
        if (f) {
            fclose(f);
            printf("[INFO] Detectado 'datos_ejemplo.txt'. Cargando...\n");
            cargar_historial(&s, "datos_ejemplo.txt", &leidos, &errores);
            printf("[OK] datos_ejemplo.txt: %d registros, %d errores.\n",
                   leidos, errores);
        }
    }

    /* -- Bucle principal del menu -- */
    do {
        mostrar_menu();

        if (!leer_entero_rango("  Seleccione opcion: ",
                               OPT_SALIR, OPT_GUARDAR_HISTORIAL,
                               &opcion)) {
            printf("[AVISO] Entrada invalida. Regresando al menu.\n");
            continue;
        }

        switch (opcion) {
            case OPT_MOSTRAR_ZONAS:
                mostrar_sistema(&s);
                break;

            case OPT_AGREGAR_ZONA:
                manejar_agregar_zona(&s);
                break;

            case OPT_REGISTRAR_MEDICION:
                manejar_registrar_medicion(&s);
                break;

            case OPT_CARGAR_HISTORIAL:
                manejar_cargar_historial(&s);
                break;

            case OPT_PROMEDIOS_HISTORICOS:
                manejar_promedios_historicos(&s);
                break;

            case OPT_PREDECIR_24H:
                manejar_prediccion(&s);
                break;

            case OPT_GENERAR_REPORTE:
                manejar_generar_reporte(&s);
                break;

            case OPT_GUARDAR_HISTORIAL:
                guardar_historial(&s, 0);
                break;

            case OPT_SALIR:
                if (confirmar("  Desea guardar el historial antes de salir?")) {
                    guardar_historial(&s, 0);
                }
                printf("\n  Hasta luego! Sistema cerrado.\n\n");
                break;

            default:
                printf("[ERROR] Opcion no reconocida.\n");
                break;
        }

    } while (opcion != OPT_SALIR);

    return 0;
}
