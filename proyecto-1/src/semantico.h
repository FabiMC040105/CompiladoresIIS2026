
#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "ast.h"
#include "tabla_simbolos.h"

/* Permite obtener el ambito de una variable del AST */
typedef const Ambito *(*SemanticoResolverAmbito)(
    const NodoAST *nodo,
    void *contexto
);

/* Recorre el AST y devuelve la cantidad de errores */
int semantico_analizar(
    const NodoAST *raiz,
    const TablaSimbolos *tabla,
    SemanticoResolverAmbito resolver,
    void *contexto
);

/* Comprueba si una variable esta declarada */
int semantico_validar_variable(
    const NodoAST *variable,
    const TablaSimbolos *tabla,
    const Ambito *ambito
);

#endif
