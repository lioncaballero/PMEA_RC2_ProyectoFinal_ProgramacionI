# Archivo: Makefile
# Proposito: Compilacion del Sistema Integral de Monitoreo de Calidad del Aire.
# Uso:
#   make          -> compilar (alias de 'all')
#   make all      -> compilar ejecutable principal
#   make clean    -> eliminar archivos objeto y ejecutables
#   make run      -> compilar y ejecutar en modo interactivo
#   make test     -> compilar y ejecutar pruebas unitarias

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -g
LDFLAGS = -lm

TARGET  = aire_urbano
SRCS    = main.c monitor.c io.c utils.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean run test

## all: compilar el ejecutable principal
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo Compilacion exitosa: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o:    main.c    monitor.h io.h utils.h
monitor.o: monitor.c monitor.h utils.h
io.o:      io.c      io.h monitor.h utils.h
utils.o:   utils.c   utils.h monitor.h

run: all
	./$(TARGET)

test: all
	@echo ""
	@echo "======================================================"
	@echo "  Ejecutando pruebas unitarias..."
	@echo "======================================================"
	./$(TARGET) --test
	@if [ $$? -eq 0 ]; then \
		echo "  OK Todas las pruebas pasaron."; \
	else \
		echo "  ERROR Algunas pruebas fallaron. Revise la salida anterior."; \
	fi

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f reporte_*.txt
	@echo Limpieza completada

# Mantener historial.txt al limpiar (datos del usuario)
# Para eliminar tambien el historial: rm -f historial.txt
