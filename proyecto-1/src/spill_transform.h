#ifndef SPILL_TRANSFORM_H
#define SPILL_TRANSFORM_H

#include <stddef.h>
#include "ir.h"
#include "asignacion_registros.h"

/* Nombre reservado usado para los slots abstractos de spill en la IR.
   El futuro codegen_ir debe bajar estos accesos a memoria de spilling. */
#define SPILL_IR_AREA "cdff_spill_area"

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

/* Construye el mapeo final %tN -> xN despues de insertar spills.
   Recalcula liveness/CFG; falla si aun hay nodos que requieren spill.
   Inicializar salida con ar_init y liberarla con ar_free. */
int spill_asignacion_final(const IRProgram *programa, int cantidad_colores,
                          int registro_base, AsignacionRegistros *salida);

#endif
