// Archivo: io.h
// Proposito: Declaraciones de funciones para lectura y escritura de archivos
//            (historial, datos por lotes, reportes).
// Estandar:  C99

#ifndef IO_H
#define IO_H

#include "monitor.h"

// -----------------------------------------------------------------------------
//  CONSTANTES DE ARCHIVOS
// -----------------------------------------------------------------------------

#define ARCHIVO_HISTORIAL   "historial.txt"
#define PREFIJO_REPORTE     "reporte_"
#define MAX_RUTA_ARCHIVO    256

// -----------------------------------------------------------------------------
//  FUNCIONES DE LECTURA
// -----------------------------------------------------------------------------

int cargar_historial(Sistema *s, const char *ruta,
                     int *leidos, int *errores);

int cargar_datos_ejemplo(Sistema *s);

// -----------------------------------------------------------------------------
//  FUNCIONES DE ESCRITURA
// -----------------------------------------------------------------------------

int guardar_historial(const Sistema *s, int forzar);

int generar_reporte(const Sistema *s, char *ruta_salida);

// -----------------------------------------------------------------------------
//  FUNCIONES DE VALIDACION DE LINEAS
// -----------------------------------------------------------------------------

int parsear_linea_historial(char *linea, char *zona_nombre, Medicion *m);

#endif /* IO_H */
