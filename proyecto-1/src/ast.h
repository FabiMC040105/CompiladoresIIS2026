#ifndef AST_H
#define AST_H

/* Tipos de nodos que vamos a usar en el arbol */
typedef enum
{
    AST_PROGRAMA,
    AST_PRINCIPAL,
    AST_BLOQUE,
    AST_DECLARACION,
    AST_ASIGNACION,
    AST_NUMERO,
    AST_BOOLEANO,
    AST_VARIABLE,
    AST_OPERACION,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FUNCION,
    AST_PARAMETRO,
    AST_PARAMETROS,
    AST_LLAMADA,
    AST_RETURN,
    AST_BREAK,
    AST_LISTA,
    AST_MATRIZ,
    AST_IMPORTACION
} TipoNodo;

/* Estructura de cada nodo del AST */
typedef struct NodoAST
{
    TipoNodo tipo;
    char *valor;
    struct NodoAST *hijo;
    struct NodoAST *siguiente;
} NodoAST;

/* Funciones para trabajar con el arbol */
NodoAST *ast_crear(TipoNodo tipo, const char *valor);
void ast_agregar_hijo(NodoAST *padre, NodoAST *hijo);
void ast_imprimir(NodoAST *nodo, int nivel);
void ast_liberar(NodoAST *nodo);

#endif
