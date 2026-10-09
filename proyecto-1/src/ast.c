#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Crea un nodo nuevo y guarda su valor */
NodoAST *ast_crear(TipoNodo tipo, const char *valor)
{
    NodoAST *nodo = malloc(sizeof(NodoAST));

    if (nodo == NULL) {
        fprintf(stderr, "Error al crear un nodo del AST.\n");
        exit(EXIT_FAILURE);
    }

    nodo->tipo = tipo;
    nodo->valor = NULL;
    nodo->hijo = NULL;
    nodo->siguiente = NULL;

    /* Guarda el valor del nodo si tiene uno */
    if (valor != NULL) {
        nodo->valor = malloc(strlen(valor) + 1);

        if (nodo->valor == NULL) {
            fprintf(stderr, "Error al guardar el valor del nodo.\n");
            free(nodo);
            exit(EXIT_FAILURE);
        }

        strcpy(nodo->valor, valor);
    }

    return nodo;
}

/* Agrega un hijo al final de la lista de hijos */
void ast_agregar_hijo(NodoAST *padre, NodoAST *hijo)
{
    NodoAST *actual;

    if (padre == NULL || hijo == NULL) {
        return;
    }

    /* Si no tiene hijos, el nuevo nodo queda como primero */
    if (padre->hijo == NULL) {
        padre->hijo = hijo;
        return;
    }

    /* Busca el ultimo hijo para agregar el nuevo */
    actual = padre->hijo;

    while (actual->siguiente != NULL) {
        actual = actual->siguiente;
    }

    actual->siguiente = hijo;
}

/* Devuelve el nombre del tipo de nodo */
static const char *ast_nombre_tipo(TipoNodo tipo)
{
    switch (tipo) {
        case AST_PROGRAMA: return "PROGRAMA";
        case AST_PRINCIPAL: return "PRINCIPAL";
        case AST_BLOQUE: return "BLOQUE";
        case AST_DECLARACION: return "DECLARACION";
        case AST_ASIGNACION: return "ASIGNACION";
        case AST_NUMERO: return "NUMERO";
        case AST_BOOLEANO: return "BOOLEANO";
        case AST_VARIABLE: return "VARIABLE";
        case AST_OPERACION: return "OPERACION";
        case AST_IF: return "IF";
        case AST_WHILE: return "WHILE";
        case AST_FOR: return "FOR";
        case AST_FUNCION: return "FUNCION";
        case AST_PARAMETRO: return "PARAMETRO";
        case AST_LLAMADA: return "LLAMADA";
        case AST_RETURN: return "RETURN";
        case AST_BREAK: return "BREAK";
        case AST_LISTA: return "LISTA";
        case AST_MATRIZ: return "MATRIZ";
        case AST_IMPORTACION: return "IMPORTACION";
        default: return "DESCONOCIDO";
    }
}

/* Imprime un nodo y sus ramas */
static void ast_imprimir_ramas(NodoAST *nodo, const char *prefijo, int ultimo)
{
    if (nodo == NULL) {
        return;
    }

    /* Imprime la rama y el nombre del nodo */
    printf("%s", prefijo);
    printf("%s", ultimo ? "└── " : "├── ");

    printf("%s", ast_nombre_tipo(nodo->tipo));

    if (nodo->valor != NULL) {
        printf(": %s", nodo->valor);
    }

    printf("\n");

    /* Prepara los espacios que van a usar los hijos */
    char nuevo_prefijo[1024];

    snprintf(nuevo_prefijo, sizeof(nuevo_prefijo), "%s%s",
             prefijo, ultimo ? "    " : "│   ");

    /* Imprime los hijos del nodo actual */
    NodoAST *hijo = nodo->hijo;

    while (hijo != NULL) {
        int es_ultimo = (hijo->siguiente == NULL);

        ast_imprimir_ramas(hijo, nuevo_prefijo, es_ultimo);

        hijo = hijo->siguiente;
    }
}

/* Imprime la raiz y todos sus hijos */
void ast_imprimir(NodoAST *nodo, int nivel)
{
    (void)nivel;

    if (nodo == NULL) {
        return;
    }

    /* Imprime la raiz sin una rama */
    printf("%s", ast_nombre_tipo(nodo->tipo));

    if (nodo->valor != NULL) {
        printf(": %s", nodo->valor);
    }

    printf("\n");

    /* Imprime los hijos de la raiz con sus ramas */
    NodoAST *hijo = nodo->hijo;

    while (hijo != NULL) {
        int es_ultimo = (hijo->siguiente == NULL);

        ast_imprimir_ramas(hijo, "", es_ultimo);

        hijo = hijo->siguiente;
    }
}

/* Libera la memoria utilizada por el arbol */
void ast_liberar(NodoAST *nodo)
{
    while (nodo != NULL) {
        NodoAST *siguiente = nodo->siguiente;

        /* Primero libera los hijos de este nodo */
        ast_liberar(nodo->hijo);

        /* Libera el valor y el nodo */
        free(nodo->valor);
        free(nodo);

        nodo = siguiente;
    }
}
