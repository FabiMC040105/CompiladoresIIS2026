#ifndef CODEGEN_IR_H
#define CODEGEN_IR_H

#include "ir.h"
#include "codegen.h"
#include <stddef.h>

/*
 * Resuelve un simbolo del IR utilizando el nodo
 * original del AST para respetar los ambitos.
 *
 * Cuando nodo_origen es NULL, el adaptador solo
 * puede consultar simbolos globales.
 */
typedef int (*CodegenIRResolver)(
    void *contexto,
    const NodoAST *nodo_origen,
    const char *funcion,
    const char *nombre,
    CodegenSimbolo *salida
);


typedef struct {
    CodegenConfig original;
    CodegenIRResolver resolver_ir;
    void *contexto_ir;
} CodegenIRConfig;

typedef struct {
    size_t instrucciones_ir;
    size_t instrucciones_traducidas;
    size_t bundles_emitidos;
    size_t temporales;
} CodegenIREstadisticas;

/* Devuelve 1 si se pudo traducir todo; 0 si hay algo no soportado. */
int codegen_ir_generar_archivo(const IRProgram *programa,
                              const char *ruta_salida,
                              const CodegenIRConfig *config,
                              CodegenIREstadisticas *estadisticas);

#endif
