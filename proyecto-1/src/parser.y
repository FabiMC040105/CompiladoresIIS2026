%{
#include <stdio.h>
#include <stdlib.h>

/*
 * Función que genera Flex.
 * Bison la llama para pedirle el siguiente token al lexer.
 */
int yylex(void);

/*
 * Número de línea mantenido por Flex gracias a:
 * %option yylineno
 */
extern int yylineno;

/*
 * Función de manejo de errores de Bison.
 */
void yyerror(const char *s);
%}

%define lr.type ielr
%define parse.error verbose

/* =========================================================
   TOKENS
   ========================================================= */

/* Palabras reservadas */
%token SI
%token SINO
%token SINOSI
%token MIENTRAS
%token PARA
%token DEVUELVA
%token DEFINA
%token ROMPA
%token PRINCIPAL

/* Tipos y estructuras */
%token LISTA
%token MATRIZ
%token VOF
%token ENTERO
%token VACIO

/* Operadores de más de un carácter */
%token POTENCIA
%token IGUALDAD
%token DIFERENTE
%token MENOR_IGUAL
%token MAYOR_IGUAL
%token AND
%token OR

/* Valores booleanos */
%token VERDADERO
%token FALSO

/* Identificadores y números */
%token NUMERO
%token IDENTIFICADOR

/* Importaciones */
%token TRAIGASE
%token EXTENSION_E

/* Otros tokens */
%token ERROR_LEXICO
%token FIN_LINEA

%%

/* =========================================================
   PROGRAMA
   ========================================================= */

Programa
    : Importaciones Globales Unidades
    ;

/* =========================================================
   IMPORTACIONES
   ========================================================= */

Importaciones
    : Importacion Importaciones
    |
    ;

Importacion
    : TRAIGASE '"' IDENTIFICADOR EXTENSION_E '"' FIN_LINEA
    ;

/* =========================================================
   DECLARACIONES GLOBALES
   ========================================================= */

Globales
    : Declaracion Globales
    |
    ;

/* =========================================================
   UNIDADES
   ========================================================= */

Unidades
    : DEFINA RestoDefinicion
    |
    ;

RestoDefinicion
    : ENTERO FirmaFuncion Unidades
    | VOF FirmaFuncion Unidades
    | VACIO RestoVacio
    ;

FirmaFuncion
    : IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA
    ;

RestoVacio
    : IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA Unidades
    | PRINCIPAL '(' ')' Bloque FIN_LINEA
    ;


/* =========================================================
   PARAMETROS
   ========================================================= */

Parametros
    : Parametro ParametrosResto
    |
    ;

ParametrosResto
    : ',' Parametro ParametrosResto
    |
    ;

Parametro
    : ENTERO IDENTIFICADOR
    | VOF IDENTIFICADOR
    | ENTERO LISTA IDENTIFICADOR '[' Dimension ']'
    | ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']'
    ;

/* =========================================================
   DIMENSIONES
   ========================================================= */

Dimension
    : NUMERO
    | IDENTIFICADOR
    ;

/* =========================================================
   BLOQUES
   ========================================================= */

Bloque
    : '{' Sentencias '}'
    ;

Sentencias
    : Sentencia Sentencias
    |
    ;

/* =========================================================
   SENTENCIAS
   ========================================================= */

Sentencia
    : Declaracion
    | Asignacion FIN_LINEA
    | Llamada FIN_LINEA
    | If
    | While
    | For
    | Return
    | Break
    ;

/* =========================================================
   DECLARACIONES
   ========================================================= */

Declaracion
    : ENTERO IDENTIFICADOR Inicializacion FIN_LINEA
    | VOF IDENTIFICADOR Inicializacion FIN_LINEA
    | ENTERO LISTA IDENTIFICADOR '[' Dimension ']' Inicializadorlista FIN_LINEA
    | ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']' Inicializadormatriz FIN_LINEA
    ;

/* =========================================================
   INICIALIZACIÓN
   ========================================================= */

Inicializacion
    : '=' Expresion
    |
    ;

Inicializadorlista
    : '=' '[' Elementos ']'
    |
    ;

Inicializadormatriz
    : '=' '[' Filas ']'
    |
    ;

/* =========================================================
   MATRICES
   ========================================================= */

Filas
    : Fila RestoFilas
    ;

RestoFilas
    : ',' Fila RestoFilas
    |
    ;

Fila
    : '[' Elementos ']'
    ;

Elementos
    : Expresion RestoElementos
    |
    ;

RestoElementos
    : ',' Expresion RestoElementos
    |
    ;

/* =========================================================
   ASIGNACIONES
   ========================================================= */

Asignacion
    : Destino '=' Expresion
    ;

Destino
    : Acceso
    ;

/* =========================================================
   ACCESOS
   ========================================================= */

Acceso
    : IDENTIFICADOR
    | IDENTIFICADOR '[' ExprArit ']'
    | IDENTIFICADOR '[' ExprArit ']' '[' ExprArit ']'
    ;

/* =========================================================
   IF
   ========================================================= */

If
    : SI '(' Expresion ')' Bloque Continuacionif
    ;

Continuacionif
    : FIN_LINEA
    | SINO Bloque FIN_LINEA
    | SINOSI '(' Expresion ')' Bloque Continuacionif
    ;

/* =========================================================
   WHILE
   ========================================================= */

While
    : MIENTRAS '(' Expresion ')' Bloque FIN_LINEA
    ;

/* =========================================================
   FOR
   ========================================================= */

For
    : PARA '(' InicioFor ',' Expresion ',' Asignacion ')' Bloque FIN_LINEA
    ;

InicioFor
    : Asignacion
    | ENTERO IDENTIFICADOR '=' Expresion
    ;

/* =========================================================
   RETURN
   ========================================================= */

Return
    : DEVUELVA Expresion FIN_LINEA
    ;

/* =========================================================
   BREAK
   ========================================================= */

Break
    : ROMPA FIN_LINEA
    ;

/* =========================================================
   LLAMADAS
   ========================================================= */

Llamada
    : IDENTIFICADOR '(' Argumentos ')'
    ;

Argumentos
    : Expresion RestoArgumentos
    |
    ;

RestoArgumentos
    : ',' Expresion RestoArgumentos
    |
    ;

/* =========================================================
   EXPRESIONES
   ========================================================= */

Expresion
    : Expresion OR ExprAnd
    | ExprAnd
    ;

ExprAnd
    : ExprAnd AND ExprComp
    | ExprComp
    ;

ExprComp
    : ExprArit Comparador ExprArit
    | ExprArit
    ;

Comparador
    : IGUALDAD
    | DIFERENTE
    | MENOR_IGUAL
    | MAYOR_IGUAL
    | '<'
    | '>'
    ;

ExprArit
    : ExprArit '+' Termino
    | ExprArit '-' Termino
    | Termino
    ;

Termino
    : Termino '*' Potencia
    | Termino '/' Potencia
    | Termino '%' Potencia
    | Potencia
    ;

Potencia
    : Unario POTENCIA Potencia
    | Unario
    ;

Unario
    : '-' Unario
    | Primario
    ;

Primario
    : NUMERO
    | VERDADERO
    | FALSO
    | Acceso
    | Llamada
    | '(' Expresion ')'
    ;

%%

/* =========================================================
   MANEJO DE ERRORES
   ========================================================= */

void yyerror(const char *s)
{
    fprintf(stderr,
            "Error sintactico en la linea %d: %s\n",
            yylineno,
            s);
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    if (yyparse() == 0)
    {
        printf("Programa sintacticamente correcto.\n");
        return 0;
    }

    printf("Programa sintacticamente incorrecto.\n");
    return 1;
}
