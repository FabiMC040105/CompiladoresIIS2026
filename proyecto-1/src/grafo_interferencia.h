#ifndef GRAFO_INTERFERENCIA_H
#define GRAFO_INTERFERENCIA_H

#include <stddef.h>
#include <stdio.h>
#include "ir.h"
#include "liveness.h"

typedef struct {
    char *nombre;
    int color;       /* 0..K-1; -1 si no fue coloreado */
    int spill;       /* 1 si no pudo recibir color */
} NodoInterferencia;

typedef struct {
    NodoInterferencia *nodos;
    size_t cantidad;
    size_t capacidad;
    unsigned char *ady; /* matriz cantidad x cantidad */
} GrafoInterferencia;

void gi_init(GrafoInterferencia *grafo);
void gi_free(GrafoInterferencia *grafo);

/* Construye el grafo a partir de IN/OUT por instruccion.
   Solo crea nodos para temporales virtuales %tN, porque las variables
   fuente permanecen en memoria en el backend actual. */
int gi_construir(const IRProgram *programa,
                 const DatosInstruccion *liveness,
                 GrafoInterferencia *grafo);

/* Coloreo tipo simplify/select. Cada color representa un registro fisico.
   Los nodos que no pueden colorearse quedan marcados con spill=1. */
int gi_colorear(GrafoInterferencia *grafo, int cantidad_colores);

size_t gi_contar_spills(const GrafoInterferencia *grafo);

/* Muestra los nodos y aristas del grafo de interferencia. */
void gi_imprimir_grafo(const GrafoInterferencia *grafo,
                       FILE *salida);

/* Exporta el grafo en formato Graphviz DOT.
   Si ya fue coloreado, incluye el color/register o SPILL en cada nodo. */
int gi_exportar_dot(const GrafoInterferencia *grafo,
                    int registro_base,
                    const char *ruta);
void gi_imprimir(const GrafoInterferencia *grafo,
                 int registro_base,
                 FILE *salida);

#endif
