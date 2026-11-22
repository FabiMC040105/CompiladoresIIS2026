#ifndef RENOMBRAMIENTO_H
#define RENOMBRAMIENTO_H

#include "dag.h"

typedef struct {
    size_t raw, war, waw, memoria, barreras;
} RenConteo;

typedef struct {
    size_t bloques, definiciones, usos, instrucciones;
    RenConteo antes, despues;
} RenEstadisticas;

/* Pase local de IR -> IR. Requiere IR semanticamente valida, con temporales
   %tN privados y declaraciones escalares de la IR actual. Construye su propio
   CFG actualizado. Conserva cantidad, orden, opcodes y etiquetas. Retorna 1
   en exito (incluso sin cambios); 0 en error, sin modificar p. El pase no
   necesita liveness global ni modifica codegen. Tras un cambio, descartar
   DAG/DepGrafo anteriores: sus cadenas prestadas dejan de ser validas.
   El CFG anterior conserva rangos pero conviene reconstruirlo. */
int renombrar_programa(IRProgram *p, RenEstadisticas *out);
void ren_imprimir(const RenEstadisticas *s, FILE *out);

#endif
