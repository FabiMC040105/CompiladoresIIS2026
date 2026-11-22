#ifndef ASIGNACION_REGISTROS_H
#define ASIGNACION_REGISTROS_H

#include <stddef.h>
#include <stdio.h>

#include "grafo_interferencia.h"

typedef struct {
    char *temporal;

    /* Registro fisico xN. Vale -1 si el temporal fue enviado a spill. */
    int registro;

    /* Indica si este temporal debe almacenarse en memoria. */
    int es_spill;

    /*
     * Indice logico del slot de spill.
     * Todavia no representa un offset concreto de memoria.
     */
    size_t slot_spill;
} AsignacionTemporal;

typedef struct {
    AsignacionTemporal *entradas;
    size_t cantidad;
    size_t cantidad_spills;
} AsignacionRegistros;

void ar_init(AsignacionRegistros *asignacion);
void ar_free(AsignacionRegistros *asignacion);

int ar_construir(
    const GrafoInterferencia *grafo,
    int registro_base,
    AsignacionRegistros *asignacion
);

const AsignacionTemporal *ar_buscar(
    const AsignacionRegistros *asignacion,
    const char *temporal
);

void ar_imprimir(
    const AsignacionRegistros *asignacion,
    FILE *salida
);

#endif
