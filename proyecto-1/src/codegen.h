#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include "ast.h"

/*
 * Forma minima que Fase 4 necesita conocer de cada simbolo.
 * Los valores empiezan en cero para conservar compatibilidad con los
 * resolvedores de las pruebas anteriores, donde todo era escalar.
 */
typedef enum {
    CODEGEN_FORMA_ESCALAR = 0,
    CODEGEN_FORMA_LISTA,
    CODEGEN_FORMA_MATRIZ
} CodegenForma;

typedef struct {
    const char *nombre;
    uint32_t direccion;
    const char *tipo;

    CodegenForma forma;
    unsigned cantidad_dimensiones;
    uint32_t dimensiones[2];
} CodegenSimbolo;

/* Resolvedor legado: suficiente para pruebas simples sin shadowing. */
typedef int (*CodegenResolverSimbolo)(void *contexto,
                                      const char *nombre,
                                      CodegenSimbolo *salida);

/*
 * Resolvedor recomendado para la integracion real con Fase 3.
 * Recibe el nodo identificador concreto del AST, lo que permite usar
 * la marca nodo->ambito que ya mantiene el parser/semantico y resolver
 * correctamente nombres repetidos en ambitos distintos.
 */
typedef int (*CodegenResolverSimboloNodo)(void *contexto,
                                          const NodoAST *nodo_identificador,
                                          const char *nombre,
                                          CodegenSimbolo *salida);

typedef struct {
    CodegenResolverSimbolo resolver_simbolo;
    CodegenResolverSimboloNodo resolver_simbolo_nodo;
    void *contexto_simbolos;
    unsigned nops_dependencia;
} CodegenConfig;

int codegen_generar_archivo(NodoAST *raiz,
                            const char *ruta_salida,
                            const CodegenConfig *config);

#endif
