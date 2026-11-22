#ifndef REORDENAMIENTO_H
#define REORDENAMIENTO_H

#include "dependencias.h"

typedef struct {
    size_t bloques, regiones, regiones_optimizadas;
    size_t instrucciones;              /* Instrucciones cuyo indice cambia. */
    size_t esperas_antes, esperas_despues; /* Modelo IR, NO ciclos/NOP ISA. */
    size_t pico_local_antes, pico_local_despues;
    size_t rechazadas_presion;
} ReoEstadisticas;

/* Pase IR -> IR independiente del renombramiento y de Chaitin.
   Requiere IR semanticamente valida. Reconstruye CFG y dependencias.
   Conserva todos los cuadruplos, sus cadenas, source_line y contadores.
   1 = exito (tambien sin cambios); 0 = error, IR y stats sin cambios utiles.
   Descartar analisis anteriores: sus indices/numeracion quedan obsoletos.
   Recalcular liveness antes de asignar registros. */
int reordenar_programa(IRProgram *p, ReoEstadisticas *out);
void reo_imprimir(const ReoEstadisticas *s, FILE *out);

#endif
