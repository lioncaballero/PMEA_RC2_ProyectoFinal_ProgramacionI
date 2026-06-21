// Archivo: monitor.h
// Proposito: Definiciones de estructuras, constantes y declaraciones de funciones
//            para el Sistema Integral de Gestion y Prediccion de Contaminacion del Aire.
// Autor:     Sistema generado para proyecto academico (Lion) - Quito, Ecuador
// Estandar:  C99

#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>

// -----------------------------------------------------------------------------
//  CONSTANTES GENERALES
// -----------------------------------------------------------------------------

#define MAX_ZONAS          10       /* Maximo de zonas permitidas */
#define MAX_REGISTROS      100     /* Registros historicos por zona */
#define MAX_NOMBRE         100      /* Longitud maxima de nombre de zona */
#define NUM_CONTAMINANTES  6        /* PM2.5, PM10, NO2, O3, SO2, CO */
#define DIAS_PROMEDIO      30       /* Ventana de dias para promedio historico */
#define DIAS_PREDICCION    7        /* Ultimos N dias para prediccion ponderada */
#define MAX_INTENTOS       5        /* Max. intentos de entrada antes de volver al menu */

/* Indices de contaminantes - usados para acceder a arreglos internos */
#define IDX_PM25  0
#define IDX_PM10  1
#define IDX_NO2   2
#define IDX_O3    3
#define IDX_SO2   4
#define IDX_CO    5

/* Nombres de contaminantes (para mostrar e imprimir) */
extern const char *NOMBRES_CONTAMINANTES[NUM_CONTAMINANTES];

/* Unidades por contaminante */
extern const char *UNIDADES_CONTAMINANTES[NUM_CONTAMINANTES];

// -----------------------------------------------------------------------------
//  LIMITES OMS (valores exactos requeridos por el enunciado)
// -----------------------------------------------------------------------------

/* Limites OMS para media de 24 horas (o equivalente aplicable) */
extern const double LIMITE_OMS_24H[NUM_CONTAMINANTES];

/* Limites OMS para media anual (donde aplica; -1 si no corresponde) */
extern const double LIMITE_OMS_ANUAL[NUM_CONTAMINANTES];

// -----------------------------------------------------------------------------
//  ESTRUCTURA: MEDICION
//  Representa una lectura puntual de todos los contaminantes en una zona.
// -----------------------------------------------------------------------------

typedef struct {
    char   fecha[11];                      /* "YYYY-MM-DD" */
    char   hora[6];                        /* "HH:MM" */
    double valores[NUM_CONTAMINANTES];     /* Valores medidos */
} Medicion;

// -----------------------------------------------------------------------------
//  ESTRUCTURA: ZONA
//  Agrupa la identidad de una zona y su historial de mediciones.
// -----------------------------------------------------------------------------

typedef struct {
    char     nombre[MAX_NOMBRE];              /* Nombre de la zona */
    Medicion historial[MAX_REGISTROS];        /* Arreglo estatico de mediciones */
    int      num_registros;                   /* Cantidad de registros cargados */
    double   ultima_medicion[NUM_CONTAMINANTES]; /* Ultima medicion registrada */
    int      tiene_medicion_actual;           /* 1 si hay medicion actual, 0 si no */
} Zona;

// -----------------------------------------------------------------------------
//  ESTRUCTURA: SISTEMA
//  Contenedor principal que agrupa todas las zonas.
// -----------------------------------------------------------------------------

typedef struct {
    Zona zonas[MAX_ZONAS];
    int  num_zonas;
} Sistema;

// -----------------------------------------------------------------------------
//  ESTRUCTURA: ALERTA
//  Describe una alerta generada por exceso de un limite OMS.
// -----------------------------------------------------------------------------

typedef enum {
    SEV_ADVERTENCIA = 0,   /* 0-20% sobre el limite */
    SEV_ALERTA      = 1,   /* 20-50% sobre el limite */
    SEV_CRITICA     = 2    /* >50% sobre el limite */
} Severidad;

typedef struct {
    char      zona[MAX_NOMBRE];
    int       contaminante;    /* Indice en NOMBRES_CONTAMINANTES */
    double    valor;           /* Valor que disparo la alerta */
    double    limite;          /* Limite OMS correspondiente */
    Severidad severidad;
    int       es_prediccion;   /* 1 = viene de prediccion, 0 = valor actual */
} Alerta;

// -----------------------------------------------------------------------------
//  DECLARACIONES DE FUNCIONES - monitor.c
// -----------------------------------------------------------------------------

void inicializar_sistema(Sistema *s);
int agregar_zona(Sistema *s, const char *nombre);
int registrar_medicion(Zona *z, const Medicion *m);
int calcular_promedio_historico(const Zona *z, double promedios[NUM_CONTAMINANTES]);
int predecir_24h(const Zona *z, double prediccion[NUM_CONTAMINANTES]);
int generar_alertas(const Zona *z, const double prediccion[NUM_CONTAMINANTES], Alerta alertas[], int max_alertas);
const char *severidad_a_texto(Severidad sev);
void recomendacion_alerta(const Alerta *a, char *buf, int tam);
void mostrar_zona(const Zona *z);
void mostrar_sistema(const Sistema *s);

// -----------------------------------------------------------------------------
//  FUNCIONES DE PRUEBA (invocadas por make test)
// -----------------------------------------------------------------------------

int test_prediccion(void);
int test_alertas(void);

#endif /* MONITOR_H */
