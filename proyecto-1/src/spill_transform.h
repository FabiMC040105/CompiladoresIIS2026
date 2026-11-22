#ifndef SPILL_TRANSFORM_H
#define SPILL_TRANSFORM_H

#include <stddef.h>
#include "ir.h"

typedef struct {
    size_t rondas;
    size_t stores;
    size_t loads;
    size_t slots;
} ResultadoSpill;

/*
 * Resuelve la presion de registros de un programa IR.
 *
 * El proceso es:
 *   liveness
 *   -> grafo de interferencia
 *   -> coloreo
 *   -> seleccion de spill
 *   -> insercion de STORE/LOAD
 *   -> repetir hasta que el grafo sea coloreable.
 */
int spill_resolver_programa(
    IRProgram *programa,
    int cantidad_colores,
    int registro_base,
    ResultadoSpill *resultado
);

#endif
