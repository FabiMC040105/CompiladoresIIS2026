#include "optimizaciones_ir.h"
#include <string.h>

/* Clonacion profunda para evitar dejar el programa parcialmente cambiado si
   alguna fase falla (por ejemplo, al agotar memoria o rondas de spilling). */
static int opt_ir_clonar(const IRProgram *origen, IRProgram *destino)
{
    ir_init(destino);
    destino->next_temp = origen->next_temp;
    destino->next_label = origen->next_label;
    for (size_t i = 0; i < origen->count; ++i) {
        const IRInstruction *v = &origen->instructions[i];
        if (!ir_emit_ex(destino, v->op, v->arg1, v->arg2,
                        v->result, v->extra, v->source_line)) {
            ir_free(destino);
            return 0;
        }
    }
    return 1;
}

int opt_ir_aplicar(IRProgram *programa, unsigned fases, int colores,
                  int registro_base, OptIREstadisticas *estadisticas)
{
    IRProgram trabajo;
    OptIREstadisticas s = {0};

    if (!programa || !estadisticas ||
        (programa->count && !programa->instructions) ||
        (fases & ~OPT_IR_TODAS) ||
        ((fases & OPT_IR_SPILL) && (colores <= 0 || registro_base < 0)))
        return 0;
    s.instrucciones_antes = programa->count;
    if (!fases) {
        s.instrucciones_despues = programa->count;
        *estadisticas = s;
        return 1;
    }
    if (!opt_ir_clonar(programa, &trabajo)) return 0;

    /* DCE elimina computos muertos antes de analizar dependencias y presion. */
    if (fases & OPT_IR_DCE) {
        if (!eliminar_codigo_muerto(&trabajo, &s.dce)) goto fallo;
        s.fases_ejecutadas |= OPT_IR_DCE;
    }
    /* Renombramiento elimina antidependencias locales conservadoramente. */
    if (fases & OPT_IR_RENOMBRAR) {
        if (!renombrar_programa(&trabajo, &s.renombramiento)) goto fallo;
        s.fases_ejecutadas |= OPT_IR_RENOMBRAR;
    }
    /* Reordenamiento usa las dependencias YA actualizadas por renombramiento. */
    if (fases & OPT_IR_REORDENAR) {
        if (!reordenar_programa(&trabajo, &s.reordenamiento)) goto fallo;
        s.fases_ejecutadas |= OPT_IR_REORDENAR;
    }
    /* Ultimo: spilling reconstruye el CFG/liveness sobre la IR definitiva. */
    if (fases & OPT_IR_SPILL) {
        if (!spill_resolver_programa(&trabajo, colores, registro_base,
                                    &s.spilling)) goto fallo;
        s.fases_ejecutadas |= OPT_IR_SPILL;
    }

    s.instrucciones_despues = trabajo.count;
    ir_free(programa);
    *programa = trabajo;
    *estadisticas = s;
    return 1;
fallo:
    ir_free(&trabajo);
    return 0;
}

void opt_ir_imprimir(const OptIREstadisticas *s, FILE *salida)
{
    if (!s || !salida) return;
    fprintf(salida, "[OPT IR] instrucciones: %zu -> %zu\n",
            s->instrucciones_antes, s->instrucciones_despues);
    if (s->fases_ejecutadas & OPT_IR_DCE)
        fprintf(salida, "[OPT IR] DCE: eliminadas=%zu pasadas=%zu\n",
                s->dce.eliminadas, s->dce.pasadas);
    if (s->fases_ejecutadas & OPT_IR_RENOMBRAR)
        fprintf(salida, "[OPT IR] Renombramiento: definiciones=%zu usos=%zu\n",
                s->renombramiento.definiciones, s->renombramiento.usos);
    if (s->fases_ejecutadas & OPT_IR_REORDENAR)
        fprintf(salida, "[OPT IR] Reordenamiento: regiones=%zu instrucciones=%zu\n",
                s->reordenamiento.regiones_optimizadas,
                s->reordenamiento.instrucciones);
    if (s->fases_ejecutadas & OPT_IR_SPILL)
        fprintf(salida, "[SPILL IR] rondas=%zu STORE=%zu LOAD=%zu slots=%zu\n",
                s->spilling.rondas, s->spilling.stores,
                s->spilling.loads, s->spilling.slots);
}

int opt_ir_asignacion_para_codegen_ir(const IRProgram *programa, int colores,
                                     int registro_base, AsignacionRegistros *salida)
{
    return spill_asignacion_final(programa, colores, registro_base, salida);
}
