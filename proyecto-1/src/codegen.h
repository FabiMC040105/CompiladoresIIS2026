#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include "ast.h"

typedef struct {
    const char *nombre;
    uint32_t direccion;
    const char *tipo;
} CodegenSimbolo;

typedef int (*CodegenResolverSimbolo)(void *contexto,
                                      const char *nombre,
                                      CodegenSimbolo *salida);

typedef struct {
    CodegenResolverSimbolo resolver_simbolo;
    void *contexto_simbolos;
    unsigned nops_dependencia;
} CodegenConfig;

int codegen_generar_archivo(NodoAST *raiz,
                            const char *ruta_salida,
                            const CodegenConfig *config);

#endif
