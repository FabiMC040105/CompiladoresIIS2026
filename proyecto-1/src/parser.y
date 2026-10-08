%code requires {
#include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* Funcion que obtiene los tokens del lexer */
int yylex(void);

/* Numero de linea que mantiene Flex */
extern int yylineno;

/* Funcion para reportar errores */
void yyerror(const char *s);

/* Aqui guardamos la raiz del arbol */
NodoAST *raiz = NULL;

/* Une dos listas de nodos */
static NodoAST *ast_unir(NodoAST *primero, NodoAST *segundo)
{
    NodoAST *actual;

    if (primero == NULL)
        return segundo;

    actual = primero;

    while (actual->siguiente != NULL)
        actual = actual->siguiente;

    actual->siguiente = segundo;
    return primero;
}

/* Cambia el valor de un nodo */
static void ast_cambiar_valor(NodoAST *nodo, const char *valor)
{
    char *nuevo;

    if (nodo == NULL)
        return;

    nuevo = malloc(strlen(valor) + 1);

    if (nuevo == NULL)
    {
        fprintf(stderr, "Error al guardar el valor del nodo.\n");
        exit(EXIT_FAILURE);
    }

    strcpy(nuevo, valor);
    free(nodo->valor);
    nodo->valor = nuevo;
}

/* Crea una operacion y agrega sus operandos */
static NodoAST *ast_operacion(const char *operador,
                             NodoAST *izquierdo,
                             NodoAST *derecho)
{
    NodoAST *nodo = ast_crear(AST_OPERACION, operador);

    ast_agregar_hijo(nodo, izquierdo);
    ast_agregar_hijo(nodo, derecho);

    return nodo;
}
%}

%define lr.type ielr
%define parse.error verbose
%define api.value.type {NodoAST *}

/* Palabras reservadas */
%token SI SINO SINOSI MIENTRAS PARA DEVUELVA DEFINA ROMPA PRINCIPAL

/* Tipos y estructuras */
%token LISTA MATRIZ VOF ENTERO VACIO

/* Operadores */
%token POTENCIA IGUALDAD DIFERENTE MENOR_IGUAL MAYOR_IGUAL AND OR

/* Valores booleanos */
%token VERDADERO FALSO

/* Identificadores y numeros */
%token NUMERO IDENTIFICADOR

/* Importaciones */
%token TRAIGASE EXTENSION_E

/* Otros tokens */
%token ERROR_LEXICO FIN_LINEA

%%

/* Programa completo */
Programa
    : Importaciones Globales Unidades
      {
          raiz = ast_crear(AST_PROGRAMA, "programa");
          ast_agregar_hijo(raiz, $1);
          ast_agregar_hijo(raiz, $2);
          ast_agregar_hijo(raiz, $3);
          $$ = raiz;
      }
    ;

/* Importaciones */
Importaciones
    : Importacion Importaciones
      {
          $$ = ast_unir($1, $2);
      }
    |
      {
          $$ = NULL;
      }
    ;

Importacion
    : TRAIGASE '"' IDENTIFICADOR EXTENSION_E '"' FIN_LINEA
      {
          $$ = ast_crear(AST_IMPORTACION, $3->valor);
          ast_liberar($3);
      }
    ;

/* Declaraciones globales */
Globales
    : Declaracion Globales
      {
          $$ = ast_unir($1, $2);
      }
    |
      {
          $$ = NULL;
      }
    ;

/* Funciones y principal */
Unidades
    : DEFINA RestoDefinicion
      {
          $$ = $2;
      }
    |
      {
          $$ = NULL;
      }
    ;

RestoDefinicion
    : ENTERO FirmaFuncion Unidades
      {
          ast_cambiar_valor($2, "entero");
          $$ = ast_unir($2, $3);
      }
    | VOF FirmaFuncion Unidades
      {
          ast_cambiar_valor($2, "vof");
          $$ = ast_unir($2, $3);
      }
    | VACIO RestoVacio
      {
          $$ = $2;
      }
    ;

FirmaFuncion
    : IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA
      {
          $$ = ast_crear(AST_FUNCION, "funcion");
          ast_agregar_hijo($$, $1);
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
      }
    ;

RestoVacio
    : IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA Unidades
      {
          NodoAST *funcion = ast_crear(AST_FUNCION, "vacio");

          ast_agregar_hijo(funcion, $1);
          ast_agregar_hijo(funcion, $3);
          ast_agregar_hijo(funcion, $5);

          $$ = ast_unir(funcion, $7);
      }
    | PRINCIPAL '(' ')' Bloque FIN_LINEA
      {
          $$ = ast_crear(AST_PRINCIPAL, "principal");
          ast_agregar_hijo($$, $4);
      }
    ;

/* Parametros */
Parametros
    : Parametro ParametrosResto
      {
          $$ = ast_unir($1, $2);
      }
    |
      {
          $$ = NULL;
      }
    ;

ParametrosResto
    : ',' Parametro ParametrosResto
      {
          $$ = ast_unir($2, $3);
      }
    |
      {
          $$ = NULL;
      }
    ;

Parametro
    : ENTERO IDENTIFICADOR
      {
          $$ = ast_crear(AST_PARAMETRO, "entero");
          ast_agregar_hijo($$, $2);
      }
    | VOF IDENTIFICADOR
      {
          $$ = ast_crear(AST_PARAMETRO, "vof");
          ast_agregar_hijo($$, $2);
      }
    | ENTERO LISTA IDENTIFICADOR '[' Dimension ']'
      {
          $$ = ast_crear(AST_PARAMETRO, "entero lista");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
      }
    | ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']'
      {
          $$ = ast_crear(AST_PARAMETRO, "entero matriz");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $8);
      }
    ;

/* Dimensiones de listas y matrices */
Dimension
    : NUMERO
      {
          $$ = $1;
      }
    | IDENTIFICADOR
      {
          $$ = $1;
      }
    ;

/* Bloques de codigo */
Bloque
    : '{' Sentencias '}'
      {
          $$ = ast_crear(AST_BLOQUE, "bloque");
          ast_agregar_hijo($$, $2);
      }
    ;

Sentencias
    : Sentencia Sentencias
      {
          $$ = ast_unir($1, $2);
      }
    |
      {
          $$ = NULL;
      }
    ;

/* Sentencias del lenguaje */
Sentencia
    : Declaracion
      {
          $$ = $1;
      }
    | Asignacion FIN_LINEA
      {
          $$ = $1;
      }
    | Llamada FIN_LINEA
      {
          $$ = $1;
      }
    | If
      {
          $$ = $1;
      }
    | While
      {
          $$ = $1;
      }
    | For
      {
          $$ = $1;
      }
    | Return
      {
          $$ = $1;
      }
    | Break
      {
          $$ = $1;
      }
    ;

/* Declaraciones de variables */
Declaracion
    : ENTERO IDENTIFICADOR Inicializacion FIN_LINEA
      {
          $$ = ast_crear(AST_DECLARACION, "entero");
          ast_agregar_hijo($$, $2);
          ast_agregar_hijo($$, $3);
      }
    | VOF IDENTIFICADOR Inicializacion FIN_LINEA
      {
          $$ = ast_crear(AST_DECLARACION, "vof");
          ast_agregar_hijo($$, $2);
          ast_agregar_hijo($$, $3);
      }
    | ENTERO LISTA IDENTIFICADOR '[' Dimension ']' Inicializadorlista FIN_LINEA
      {
          $$ = ast_crear(AST_LISTA, "declaracion entero");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $7);
      }
    | ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']' Inicializadormatriz FIN_LINEA
      {
          $$ = ast_crear(AST_MATRIZ, "declaracion entero");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $8);
          ast_agregar_hijo($$, $10);
      }
    ;

/* Valor inicial de una variable */
Inicializacion
    : '=' Expresion
      {
          $$ = $2;
      }
    |
      {
          $$ = NULL;
      }
    ;

Inicializadorlista
    : '=' '[' Elementos ']'
      {
          $$ = ast_crear(AST_LISTA, "inicializacion");
          ast_agregar_hijo($$, $3);
      }
    |
      {
          $$ = NULL;
      }
    ;

Inicializadormatriz
    : '=' '[' Filas ']'
      {
          $$ = ast_crear(AST_MATRIZ, "inicializacion");
          ast_agregar_hijo($$, $3);
      }
    |
      {
          $$ = NULL;
      }
    ;

/* Filas y elementos de listas y matrices */
Filas
    : Fila RestoFilas
      {
          $$ = ast_unir($1, $2);
      }
    ;

RestoFilas
    : ',' Fila RestoFilas
      {
          $$ = ast_unir($2, $3);
      }
    |
      {
          $$ = NULL;
      }
    ;

Fila
    : '[' Elementos ']'
      {
          $$ = ast_crear(AST_LISTA, "fila");
          ast_agregar_hijo($$, $2);
      }
    ;

Elementos
    : Expresion RestoElementos
      {
          $$ = ast_unir($1, $2);
      }
    |
      {
          $$ = NULL;
      }
    ;

RestoElementos
    : ',' Expresion RestoElementos
      {
          $$ = ast_unir($2, $3);
      }
    |
      {
          $$ = NULL;
      }
    ;

/* Asignaciones */
Asignacion
    : Destino '=' Expresion
      {
          $$ = ast_crear(AST_ASIGNACION, "=");
          ast_agregar_hijo($$, $1);
          ast_agregar_hijo($$, $3);
      }
    ;

Destino
    : Acceso
      {
          $$ = $1;
      }
    ;

/* Acceso a variables, listas y matrices */
Acceso
    : IDENTIFICADOR
      {
          $$ = $1;
      }
    | IDENTIFICADOR '[' ExprArit ']'
      {
          $$ = ast_crear(AST_OPERACION, "acceso_lista");
          ast_agregar_hijo($$, $1);
          ast_agregar_hijo($$, $3);
      }
    | IDENTIFICADOR '[' ExprArit ']' '[' ExprArit ']'
      {
          $$ = ast_crear(AST_OPERACION, "acceso_matriz");
          ast_agregar_hijo($$, $1);
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $6);
      }
    ;

/* Condicionales */
If
    : SI '(' Expresion ')' Bloque Continuacionif
      {
          $$ = ast_crear(AST_IF, "si");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $6);
      }
    ;

Continuacionif
    : FIN_LINEA
      {
          $$ = NULL;
      }
    | SINO Bloque FIN_LINEA
      {
          $$ = ast_crear(AST_IF, "sino");
          ast_agregar_hijo($$, $2);
      }
    | SINOSI '(' Expresion ')' Bloque Continuacionif
      {
          $$ = ast_crear(AST_IF, "sinosi");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $6);
      }
    ;

/* Ciclo mientras */
While
    : MIENTRAS '(' Expresion ')' Bloque FIN_LINEA
      {
          $$ = ast_crear(AST_WHILE, "mientras");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
      }
    ;

/* Ciclo para */
For
    : PARA '(' InicioFor ',' Expresion ',' Asignacion ')' Bloque FIN_LINEA
      {
          $$ = ast_crear(AST_FOR, "para");
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $7);
          ast_agregar_hijo($$, $9);
      }
    ;

InicioFor
    : Asignacion
      {
          $$ = $1;
      }
    | ENTERO IDENTIFICADOR '=' Expresion
      {
          $$ = ast_crear(AST_DECLARACION, "entero");
          ast_agregar_hijo($$, $2);
          ast_agregar_hijo($$, $4);
      }
    ;

/* Devolver un valor */
Return
    : DEVUELVA Expresion FIN_LINEA
      {
          $$ = ast_crear(AST_RETURN, "devuelva");
          ast_agregar_hijo($$, $2);
      }
    ;

/* Romper un ciclo */
Break
    : ROMPA FIN_LINEA
      {
          $$ = ast_crear(AST_BREAK, "rompa");
      }
    ;

/* Llamadas a funciones */
Llamada
    : IDENTIFICADOR '(' Argumentos ')'
      {
          $$ = ast_crear(AST_LLAMADA, $1->valor);
          ast_liberar($1);
          ast_agregar_hijo($$, $3);
      }
    ;

Argumentos
    : Expresion RestoArgumentos
      {
          $$ = ast_unir($1, $2);
      }
    |
      {
          $$ = NULL;
      }
    ;

RestoArgumentos
    : ',' Expresion RestoArgumentos
      {
          $$ = ast_unir($2, $3);
      }
    |
      {
          $$ = NULL;
      }
    ;

/* Expresiones logicas */
Expresion
    : Expresion OR ExprAnd
      {
          $$ = ast_operacion("OR", $1, $3);
      }
    | ExprAnd
      {
          $$ = $1;
      }
    ;

ExprAnd
    : ExprAnd AND ExprComp
      {
          $$ = ast_operacion("AND", $1, $3);
      }
    | ExprComp
      {
          $$ = $1;
      }
    ;

/* Comparaciones */
ExprComp
    : ExprArit Comparador ExprArit
      {
          $$ = ast_operacion($2->valor, $1, $3);
          ast_liberar($2);
      }
    | ExprArit
      {
          $$ = $1;
      }
    ;

Comparador
    : IGUALDAD
      {
          $$ = ast_crear(AST_OPERACION, "==");
      }
    | DIFERENTE
      {
          $$ = ast_crear(AST_OPERACION, "!=");
      }
    | MENOR_IGUAL
      {
          $$ = ast_crear(AST_OPERACION, "<=");
      }
    | MAYOR_IGUAL
      {
          $$ = ast_crear(AST_OPERACION, ">=");
      }
    | '<'
      {
          $$ = ast_crear(AST_OPERACION, "<");
      }
    | '>'
      {
          $$ = ast_crear(AST_OPERACION, ">");
      }
    ;

/* Operaciones aritmeticas */
ExprArit
    : ExprArit '+' Termino
      {
          $$ = ast_operacion("+", $1, $3);
      }
    | ExprArit '-' Termino
      {
          $$ = ast_operacion("-", $1, $3);
      }
    | Termino
      {
          $$ = $1;
      }
    ;

Termino
    : Termino '*' Potencia
      {
          $$ = ast_operacion("*", $1, $3);
      }
    | Termino '/' Potencia
      {
          $$ = ast_operacion("/", $1, $3);
      }
    | Termino '%' Potencia
      {
          $$ = ast_operacion("%", $1, $3);
      }
    | Potencia
      {
          $$ = $1;
      }
    ;

Potencia
    : Unario POTENCIA Potencia
      {
          $$ = ast_operacion("**", $1, $3);
      }
    | Unario
      {
          $$ = $1;
      }
    ;

Unario
    : '-' Unario
      {
          $$ = ast_operacion("unario -", $2, NULL);
      }
    | Primario
      {
          $$ = $1;
      }
    ;

/* Valores, variables, llamadas y expresiones entre parentesis */
Primario
    : NUMERO
      {
          $$ = $1;
      }
    | VERDADERO
      {
          $$ = $1;
      }
    | FALSO
      {
          $$ = $1;
      }
    | Acceso
      {
          $$ = $1;
      }
    | Llamada
      {
          $$ = $1;
      }
    | '(' Expresion ')'
      {
          $$ = $2;
      }
    ;

%%

/* Manejo de errores */
void yyerror(const char *s)
{
    fprintf(stderr,
            "Error sintactico en la linea %d: %s\n",
            yylineno,
            s);
}

/* Analiza el programa e imprime el arbol */
int main(void)
{
    if (yyparse() == 0)
    {
        printf("\nPrograma sintacticamente correcto.\n");

        if (raiz != NULL)
        {
            printf("\nArbol de sintaxis abstracta:\n");
            ast_imprimir(raiz, 0);
            ast_liberar(raiz);
        }

        return 0;
    }

    printf("Programa sintacticamente incorrecto.\n");

    if (raiz != NULL)
        ast_liberar(raiz);

    return 1;
}
