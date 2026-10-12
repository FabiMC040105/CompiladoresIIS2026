#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include "ast.h"

typedef enum {
    CODEGEN_FORMA_ESCALAR = 0,
    CODEGEN_FORMA_LISTA,
    CODEGEN_FORMA_MATRIZ
} CodegenForma;

typedef enum {
    CODEGEN_CAT_VARIABLE = 0,
    CODEGEN_CAT_PARAMETRO,
    CODEGEN_CAT_FUNCION
} CodegenCategoria;

typedef enum {
    CODEGEN_UBICACION_GLOBAL = 0,
    CODEGEN_UBICACION_LOCAL,
    CODEGEN_UBICACION_PARAMETRO,
    CODEGEN_UBICACION_FUNCION
} CodegenUbicacion;

/*
 * Vista de un simbolo que necesita el backend.
 * direccion es firmada porque los locales usan offsets negativos respecto a FP.
 */
typedef struct {
    const char *nombre;
    long direccion;
    const char *tipo;

    CodegenForma forma;
    unsigned cantidad_dimensiones;
    uint32_t dimensiones[2];

    CodegenCategoria categoria;
    CodegenUbicacion ubicacion;
    uint32_t tamano_bytes;

    /* Solo tienen significado para SIM_FUNCION. */
    uint32_t tamano_parametros;
    uint32_t tamano_locales;
    uint32_t tamano_marco;

    /* Parametros lista/matriz contienen una direccion de 4 bytes. */
    int parametro_por_referencia;
} CodegenSimbolo;

/* Compatibilidad con pruebas antiguas sin informacion de ambito. */
typedef int (*CodegenResolverSimbolo)(void *contexto,
                                      const char *nombre,
                                      CodegenSimbolo *salida);

/* Resolvedor recomendado para la integracion con la Fase 3. */
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
