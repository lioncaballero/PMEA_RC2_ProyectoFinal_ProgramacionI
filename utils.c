// Archivo: utils.c
// Proposito: Implementacion de utilidades de validacion de entrada, manipulacion
//            de cadenas, formateo de consola y obtencion de fecha/hora.
// Estandar:  C99

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "utils.h"
#include "monitor.h"  /* Para MAX_INTENTOS */

// -----------------------------------------------------------------------------
//  ENTRADA SEGURA
// -----------------------------------------------------------------------------

int leer_linea(char *buf, int tam) {
    int c;

    if (!buf || tam <= 0) return 0;

    if (!fgets(buf, tam, stdin)) {
        buf[0] = '\0';
        return 0;
    }

    /* Eliminar '\n' */
    {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        } else if (len == (size_t)(tam - 1)) {
            while ((c = getchar()) != '\n' && c != EOF)
                ;
        }
    }

    return 1;
}

int leer_double_positivo(const char *prompt, double *resultado) {
    char buf[64];
    char *endptr;
    double val;
    int intentos;

    for (intentos = 0; intentos < MAX_INTENTOS; intentos++) {
        printf("%s", prompt);
        if (!leer_linea(buf, sizeof(buf))) {
            printf("[ERROR] No se pudo leer la entrada.\n");
            continue;
        }

        trim(buf);
        normalizar_decimal(buf);

        if (buf[0] == '\0') {
            printf("[ERROR] Entrada vacia. Intente de nuevo.\n");
            continue;
        }

        if (!es_numero(buf)) {
            printf("[ERROR] '%s' no es un numero valido. Use solo digitos y punto decimal.\n",
                   buf);
            continue;
        }

        val = strtod(buf, &endptr);

        if (*endptr != '\0') {
            printf("[ERROR] Numero invalido. Intente de nuevo.\n");
            continue;
        }

        if (val < 0.0) {
            printf("[ERROR] No se permiten valores negativos para mediciones ambientales.\n");
            continue;
        }

        *resultado = val;
        return 1;
    }

    printf("[AVISO] Maximo de intentos (%d) alcanzado. Volviendo al menu.\n",
           MAX_INTENTOS);
    return 0;
}

int leer_entero_rango(const char *prompt, int min, int max, int *resultado) {
    char buf[32];
    char *endptr;
    long val;
    int intentos;

    for (intentos = 0; intentos < MAX_INTENTOS; intentos++) {
        printf("%s", prompt);
        if (!leer_linea(buf, sizeof(buf))) {
            printf("[ERROR] No se pudo leer la entrada.\n");
            continue;
        }

        trim(buf);

        if (buf[0] == '\0') {
            printf("[ERROR] Entrada vacia. Intente de nuevo.\n");
            continue;
        }

        {
            int k = 0;
            if (buf[0] == '-' || buf[0] == '+') k = 1;
            int valido = 1;
            if (buf[k] == '\0') valido = 0;
            while (buf[k] && valido) {
                if (!isdigit((unsigned char)buf[k])) valido = 0;
                k++;
            }
            if (!valido) {
                printf("[ERROR] '%s' no es un entero valido.\n", buf);
                continue;
            }
        }

        val = strtol(buf, &endptr, 10);

        if (*endptr != '\0') {
            printf("[ERROR] Entero invalido. Intente de nuevo.\n");
            continue;
        }

        if (val < (long)min || val > (long)max) {
            printf("[ERROR] Valor fuera de rango [%d, %d]. Intente de nuevo.\n",
                   min, max);
            continue;
        }

        *resultado = (int)val;
        return 1;
    }

    printf("[AVISO] Maximo de intentos (%d) alcanzado. Volviendo al menu.\n",
           MAX_INTENTOS);
    return 0;
}

int leer_nombre(const char *prompt, char *buf, int tam) {
    int intentos;
    int len;

    for (intentos = 0; intentos < MAX_INTENTOS; intentos++) {
        printf("%s", prompt);
        if (!leer_linea(buf, tam)) {
            printf("[ERROR] No se pudo leer la entrada.\n");
            continue;
        }

        trim(buf);
        len = (int)strlen(buf);

        if (len < 2) {
            printf("[ERROR] El nombre es demasiado corto (minimo 2 caracteres).\n");
            continue;
        }

        if (len >= tam) {
            printf("[AVISO] Nombre truncado a %d caracteres.\n", tam - 1);
            buf[tam - 1] = '\0';
        }

        if (es_numero(buf)) {
            printf("[ERROR] El nombre de zona no puede ser un numero.\n");
            continue;
        }

        if (!es_cadena_valida_nombre(buf)) {
            printf("[ERROR] El nombre solo puede contener letras, espacios y guiones.\n");
            continue;
        }

        return 1;
    }

    printf("[AVISO] Maximo de intentos (%d) alcanzado. Volviendo al menu.\n",
           MAX_INTENTOS);
    return 0;
}

int confirmar(const char *mensaje) {
    char buf[8];

    printf("%s (s/n): ", mensaje);
    if (!leer_linea(buf, sizeof(buf))) return 0;
    trim(buf);
    return (buf[0] == 's' || buf[0] == 'S' ||
            buf[0] == 'y' || buf[0] == 'Y');
}

// -----------------------------------------------------------------------------
//  MANIPULACION DE CADENAS
// -----------------------------------------------------------------------------

void trim(char *s) {
    char *inicio, *fin;
    size_t len;

    if (!s) return;
    len = strlen(s);
    if (len == 0) return;

    fin = s + len - 1;
    while (fin >= s && (isspace((unsigned char)*fin) || iscntrl((unsigned char)*fin))) {
        *fin = '\0';
        fin--;
    }

    inicio = s;
    while (*inicio && (isspace((unsigned char)*inicio) || iscntrl((unsigned char)*inicio))) {
        inicio++;
    }

    if (inicio != s) {
        memmove(s, inicio, strlen(inicio) + 1);
    }
}

void normalizar_decimal(char *s) {
    if (!s) return;
    while (*s) {
        if (*s == ',') *s = '.';
        s++;
    }
}

int es_numero(const char *s) {
    int tiene_digito = 0;
    int tiene_punto  = 0;

    if (!s || *s == '\0') return 0;

    if (*s == '+' || *s == '-') s++;

    while (*s) {
        if (isdigit((unsigned char)*s)) {
            tiene_digito = 1;
        } else if (*s == '.' || *s == ',') {
            if (tiene_punto) return 0;
            tiene_punto = 1;
        } else {
            return 0;
        }
        s++;
    }

    return tiene_digito;
}

int es_cadena_valida_nombre(const char *s) {
    unsigned char c;

    if (!s) return 0;
    while (*s) {
        c = (unsigned char)*s;
        if (isalpha(c) || isspace(c) || c == '-' || c >= 0x80) {
            /* valido */
        } else {
            return 0;
        }
        s++;
    }
    return 1;
}

// -----------------------------------------------------------------------------
//  FECHA Y HORA
// -----------------------------------------------------------------------------

void obtener_fecha_hora_actual(char *fecha, char *hora) {
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    if (fecha)
        strftime(fecha, 11, "%Y-%m-%d", tm_info);
    if (hora)
        strftime(hora, 6, "%H:%M", tm_info);
}

// -----------------------------------------------------------------------------
//  PARSING CSV
// -----------------------------------------------------------------------------

int separar_campos(char *linea, char delimitador,
                   char *campos[], int max_campos) {
    int n = 0;
    char *p;

    if (!linea || !campos || max_campos <= 0) return 0;

    campos[n++] = linea;
    p = linea;

    while (*p && n < max_campos) {
        if (*p == delimitador) {
            *p = '\0';
            campos[n++] = p + 1;
        }
        p++;
    }

    return n;
}

// -----------------------------------------------------------------------------
//  FORMATEO
// -----------------------------------------------------------------------------

void imprimir_separador(char caracter, int ancho) {
    int i;
    for (i = 0; i < ancho; i++) putchar(caracter);
    putchar('\n');
}
