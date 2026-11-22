#ifndef LOOP_UNROLLING_H
#define LOOP_UNROLLING_H

#include <stddef.h>
#include "ir.h"

/* El factor indica cuantas iteraciones ejecuta cada vuelta del ciclo
   transformado. El limite de crecimiento evita expandir demasiado la IR. */
typedef struct {
    unsigned factor;                 /* 2, 4 u 8 (se admiten factores de 2 a 16). */
    size_t max_body_instructions;    /* 0: valor por defecto (32). */
    size_t max_extra_instructions;   /* 0: valor por defecto (256). */
} LoopUnrollConfig;

typedef struct {
    size_t loops_found;              /* Saltos hacia atras detectados. */
    size_t loops_unrolled;           /* Ciclos transformados. */
    size_t loops_skipped;            /* Ciclos conservados por seguridad o heuristica. */
    size_t instructions_before;
    size_t instructions_after;
} LoopUnrollStats;

/* Recibe la IR original y entrega otra IR independiente.
   La salida debe estar inicializada con ir_init() y ser distinta de input.
   Retorna 1 si se completo (incluso si no habia ciclos aptos), 0 si fallo.
   Ante error, output permanece intacto. No modifica input. */
int loop_unroll(const IRProgram *input, IRProgram *output,
                const LoopUnrollConfig *config, LoopUnrollStats *stats);

#endif
