#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>

#define SCHEDULER_REGISTROS 16
#define SCHEDULER_SEPARACION_RAW 3u

typedef struct {
    unsigned char lee[SCHEDULER_REGISTROS];
    unsigned char escribe[SCHEDULER_REGISTROS];

    /*
     * ALU/LOAD con destino x1 no escriben el register file: redirigen el PC
     * cuando llegan a WB. El codegen inserta un bundle NOP posterior para
     * impedir efectos laterales de un bundle joven que coincida en EX.
     */
    unsigned char redirecciona_pc;
} SchedulerInfo;

typedef struct {
    size_t disponible_desde[SCHEDULER_REGISTROS];
} Scheduler;

void scheduler_inicializar(Scheduler *scheduler);

/*
 * Extrae registros fuente/destino de una instruccion textual del ISA.
 * Devuelve 1 si la instruccion es reconocida y 0 si no lo es.
 */
int scheduler_analizar(const char *slot,
                       const char *instruccion,
                       SchedulerInfo *info);

/*
 * Calcula cuantos bundles vacios deben insertarse ANTES de la instruccion
 * para que todos sus operandos ya hayan llegado a WB.
 */
unsigned scheduler_nops_necesarios(const Scheduler *scheduler,
                                    const SchedulerInfo *info,
                                    size_t bundle_actual);

/* Registra las escrituras que producira una instruccion ya calendarizada. */
void scheduler_registrar_escrituras(Scheduler *scheduler,
                                     const SchedulerInfo *info,
                                     size_t bundle_instruccion);

#endif
