#ifndef CODEGEN_TABLA_H
#define CODEGEN_TABLA_H

#include "ast.h"
#include "codegen.h"
#include "tabla_simbolos.h"

typedef Ambito *(*CodegenObtenerAmbitoNodo)(const NodoAST *nodo);

typedef struct {
    const TablaSimbolos *tabla;
    CodegenObtenerAmbitoNodo obtener_ambito_nodo;
} CodegenTablaContexto;

int codegen_resolver_desde_tabla(void *contexto,
                                 const NodoAST *nodo,
                                 const char *nombre,
                                 CodegenSimbolo *salida);

/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/



/*
 * Adaptador de simbolos para el backend IR.
 *
 * Si recibimos un nodo del AST, buscamos usando
 * su ambito. Si no hay nodo, mantenemos una
 * busqueda restringida al ambito global.
 */
int codegen_resolver_ir_global(
    void *contexto,
    const NodoAST *nodo_origen,
    const char *funcion,
    const char *nombre,
    CodegenSimbolo *salida
);



/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/
#endif
