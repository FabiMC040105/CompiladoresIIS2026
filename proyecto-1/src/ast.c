#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Numero de linea actual, mantenido por Flex */
extern int yylineno;
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
nodo->linea = yylineno;
nodo->hijo = NULL;
nodo->siguiente = NULL;

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

if (padre->hijo == NULL) {
    padre->linea = hijo->linea;
    padre->hijo = hijo;
    return;
}

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
case AST_ACCESO_LISTA: return "ACCESO_LISTA";
case AST_ACCESO_MATRIZ: return "ACCESO_MATRIZ";
case AST_IF: return "IF";
case AST_WHILE: return "WHILE";
case AST_FOR: return "FOR";
case AST_FUNCION: return "FUNCION";
case AST_PARAMETRO: return "PARAMETRO";
case AST_PARAMETROS: return "PARAMETROS";
case AST_LLAMADA: return "LLAMADA";
case AST_RETURN: return "RETURN";
case AST_BREAK: return "BREAK";
case AST_DECL_LISTA: return "DECL_LISTA";
case AST_DECL_MATRIZ: return "DECL_MATRIZ";
case AST_LISTA: return "LISTA";
case AST_MATRIZ: return "MATRIZ";
case AST_IMPORTACION: return "IMPORTACION";
default: return "DESCONOCIDO";
}
}

/* Escribe texto escapando caracteres especiales de Graphviz */
static void ast_escribir_texto_dot(FILE *archivo, const char *texto)
{
if (texto == NULL) {
return;
}

while (*texto != '\0') {
    switch (*texto) {
        case '"':
            fputc('\\', archivo);
            fputc('"', archivo);
            break;

        case '\\':
            fputc('\\', archivo);
            fputc('\\', archivo);
            break;

        case '\n':
            fputs("\\n", archivo);
            break;

        case '\r':
            fputs("\\r", archivo);
            break;

        default:
            fputc(*texto, archivo);
            break;
    }

    texto++;
}

}

/* Escribe un nodo y sus relaciones en el archivo DOT */
static void ast_exportar_nodo(
FILE *archivo,
NodoAST *nodo,
int padre,
int *contador)
{
if (nodo == NULL) {
return;
}

/* Asigna un identificador unico a cada nodo */
int id = (*contador)++;

/* Escribe el nodo */
fprintf(archivo, "    nodo%d [label=\"", id);

ast_escribir_texto_dot(
    archivo,
    ast_nombre_tipo(nodo->tipo)
);

if (nodo->valor != NULL) {
    fputs(": ", archivo);
    ast_escribir_texto_dot(archivo, nodo->valor);
}

fprintf(archivo, "\\n(linea %d)", nodo->linea);
fputs("\"];\n", archivo);

/* Conecta el nodo con su padre */
if (padre >= 0) {
    fprintf(
        archivo,
        "    nodo%d -> nodo%d;\n",
        padre,
        id
    );
}

/* Exporta todos los hijos del nodo */
NodoAST *hijo = nodo->hijo;

while (hijo != NULL) {
    ast_exportar_nodo(
        archivo,
        hijo,
        id,
        contador
    );

    hijo = hijo->siguiente;
}

}

/* Genera el archivo arbol.dot con todo el AST */
static int ast_generar_dot(NodoAST *raiz)
{
FILE *archivo = fopen("arbol.dot", "w");

if (archivo == NULL) {
    perror("Error al crear arbol.dot");
    return 0;
}

/* Configuracion visual del arbol */
fputs("digraph AST {\n", archivo);
fputs("    graph [rankdir=TB, ordering=out, "
      "nodesep=0.45, ranksep=0.65, splines=line];\n",
      archivo);
fputs("    node [shape=box, style=rounded, "
      "fontname=\"Consolas\", fontsize=11];\n",
      archivo);
fputs("    edge [arrowhead=none];\n\n", archivo);

int contador = 0;

ast_exportar_nodo(
    archivo,
    raiz,
    -1,
    &contador
);

fputs("}\n", archivo);

if (fclose(archivo) != 0) {
    perror("Error al guardar arbol.dot");
    return 0;
}

return 1;

}

/* Exporta el AST cuando el programa llama a esta funcion */
void ast_imprimir(NodoAST *nodo, int nivel)
{
(void)nivel;

if (nodo == NULL) {
    return;
}

if (ast_generar_dot(nodo)) {
    printf("AST exportado correctamente a arbol.dot\n");
    printf("Para generar la imagen, ejecuta:\n");
    printf("dot -Tpng arbol.dot -o arbol.png\n");
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
