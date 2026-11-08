/* LL(1) del IDE: tokenizador sin efectos secundarios y parser predictivo.
 * NO contiene ni llama a Bison, yyparse, cdff ni análisis semántico. */
#ifndef CDFF_LL1_PREDICT_H
#define CDFF_LL1_PREDICT_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "ll1_table.h"

typedef struct {
    uint64_t expected;        /* predicción FIRST de los símbolos pendientes */
    size_t position;          /* posición en bytes del token inesperado */
    int found;                /* terminal real o -1: error léxico */
    int consumed;             /* tokens completos consumidos antes del resultado */
    int ok;                   /* 1: prefijo gramatical válido, 0: error */
    size_t last_token_end;    /* fin del último token consumido por la pila */
    size_t token_end;         /* fin del token conflictivo, si existe */
} LL1Result;

/* Tokenizador compartido por el predictor y la tabla auxiliar del IDE.
 * start/end son offsets en bytes; EOF devuelve T_EOF, error -1. */
int ll1_next_token(const char *source, size_t length, size_t *offset, size_t *start);

/* @param length: fin del prefijo, incluso si la entrada total continúa.
 * El '$' NO se consume en un prefijo: se devuelve como token esperado.
 * trace NULL en producción; distinto de NULL produce traza de la pila. */
int ll1_analyze(const char *source, size_t length, LL1Result *out, FILE *trace);
/* La entrada es completa si $ pertenece a PRIMERO(pila), incluidas ε. */
static inline int ll1_is_complete(const LL1Result *r) {
    return r->ok && (r->expected & (UINT64_C(1) << T_EOF)) != 0;
}
/* Verifica que un token pueda aparecer AHORA (no como posibilidad lejana). */
static inline int ll1_expects(const LL1Result *r, int token) {
    return r->ok && token >= 0 && token < T_COUNT &&
           ((r->expected >> token) & UINT64_C(1)) != 0;
}
#endif
