// Archivo: utils.h
// Proposito: Declaraciones de funciones de validacion de entrada, utilidades
//            de cadena y ayudas generales.
// Estandar:  C99

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>  /* size_t */

/* Retorna el minimo de dos valores */
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

/* Retorna el maximo de dos valores */
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

/* Tamano de un arreglo estatico */
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

int leer_linea(char *buf, int tam);
int leer_double_positivo(const char *prompt, double *resultado);
int leer_entero_rango(const char *prompt, int min, int max, int *resultado);
int leer_nombre(const char *prompt, char *buf, int tam);
int confirmar(const char *mensaje);

void trim(char *s);
void normalizar_decimal(char *s);
int es_numero(const char *s);
int es_cadena_valida_nombre(const char *s);
void obtener_fecha_hora_actual(char *fecha, char *hora);
int separar_campos(char *linea, char delimitador, char *campos[], int max_campos);
void imprimir_separador(char caracter, int ancho);

#endif /* UTILS_H */
