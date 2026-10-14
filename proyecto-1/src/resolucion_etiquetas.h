#ifndef RESOLUCION_ETIQUETAS_H
#define RESOLUCION_ETIQUETAS_H

#include <stdio.h>

/*
 * Fase 5: lee ensamblador VLIW YA calendarizado, calcula las direcciones
 * de etiquetas y sustituye por offsets las referencias BRU y ADDI rd,x1.
 * No modifica los bundles ni inserta NOPs.
 * Retorna 1 si todo se resolvio correctamente; 0 si hubo algun error.
 */
int resolver_etiquetas_asm(const char *entrada, const char *salida, FILE *reporte);

#endif
