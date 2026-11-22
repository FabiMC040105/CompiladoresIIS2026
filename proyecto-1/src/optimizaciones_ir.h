#ifndef OPTIMIZACIONES_IR_H
#define OPTIMIZACIONES_IR_H

#include "ir.h"
#include "dead_code_elimination.h"
#include "renombramiento.h"
#include "reordenamiento.h"
#include "spill_transform.h"

/* Opciones independientes; OPT_IR_TODAS ejecuta las cuatro fases. */
enum {
    OPT_IR_DCE         = 1u << 0,
    OPT_IR_RENOMBRAR   = 1u << 1,
    OPT_IR_REORDENAR   = 1u << 2,
    OPT_IR_SPILL       = 1u << 3,
    OPT_IR_TODAS       = OPT_IR_DCE | OPT_IR_RENOMBRAR |
                         OPT_IR_REORDENAR | OPT_IR_SPILL
};

typedef struct {
    unsigned fases_ejecutadas;
    ResultadoDCE dce;
    RenEstadisticas renombramiento;
    ReoEstadisticas reordenamiento;
    ResultadoSpill spilling;
    size_t instrucciones_antes;
    size_t instrucciones_despues;
} OptIREstadisticas;

/* API para el futuro codegen_ir:
   - Recibe IRProgram construido por ir_generate, NO el AST.
   - Conserva orden: DCE -> renombramiento -> reordenamiento -> spilling.
   - Cada fase reconstruye CFG, DAG o liveness; nunca reutilizar resultados
     de analisis sobre una IR que fue modificada.
   - En error no modifica el IR de entrada (aplica sobre una copia).
   - No asigna registros fisicos en el IR ni emite codigo: el futuro
     codegen_ir debe consumir el IR resultante y bajar los slots de spill.
   - Los pseudo-accesos cdff_spill_area[slot] requieren reservar espacio
     propio al bajar a ASM; NO son un arreglo declarado por el usuario.
 */
int opt_ir_aplicar(IRProgram *programa, unsigned fases, int colores,
                  int registro_base, OptIREstadisticas *estadisticas);
void opt_ir_imprimir(const OptIREstadisticas *estadisticas, FILE *salida);

/* Opcional para codegen_ir: asignacion fisica final de los temporales.
   Llamar DESPUES de opt_ir_aplicar y ANTES de volver a mutar el programa.
   Inicializar y liberar la asignacion con ar_init/ar_free. */
int opt_ir_asignacion_para_codegen_ir(const IRProgram *programa, int colores,
                                     int registro_base, AsignacionRegistros *salida);

#endif
