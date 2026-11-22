#ifndef LOOP_UNROLLING_H
#define LOOP_UNROLLING_H

#include <stddef.h>
#include "ir.h"

/* Configuracion que recibe mi optimizador
   factor indica cuantas copias del cuerpo intento hacer por vuelta
   Los otros dos valores evitan que el codigo crezca demasiado */
typedef struct {
    unsigned factor;                 /* 2, 4 u 8 (se admiten factores de 2 a 16). */
    size_t max_body_instructions;    /* 0: valor por defecto (32). */
    size_t max_extra_instructions;   /* 0: valor por defecto (256). */
} LoopUnrollConfig;

typedef struct {
    size_t loops_found;              /* Ciclos que encontre en la IR */
    size_t loops_unrolled;           /* Ciclos que si pude desenrollar */
    size_t loops_skipped;            /* Ciclos que deje como estaban */
    size_t instructions_before;
    size_t instructions_after;
} LoopUnrollStats;

/* Funcion que usa el compilador para aplicar Loop Unrolling
   input es la IR original y output guarda la version transformada
   output debe inicializarse con ir_init y no puede ser el mismo input
   Devuelve 1 si termino bien, incluso cuando no desenrollo nada
   Si ocurre un error devuelve 0 y deja output sin cambios */
int loop_unroll(const IRProgram *input, IRProgram *output,
                const LoopUnrollConfig *config, LoopUnrollStats *stats);

#endif
