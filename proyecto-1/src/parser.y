%{
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *s);
%}

/* Declaración de tokens */

%token SI SINO SINOSI MIENTRAS PARA
%token DEVUELVA DEFINA ROMPA PRINCIPAL
%token LISTA MATRIZ VOF ENTERO VACIO

%token POTENCIA IGUALDAD DIFERENTE
%token MENOR_IGUAL MAYOR_IGUAL
%token AND OR VERDADERO FALSO

%token NUMERO IDENTIFICADOR
%token TRAIGASE EXTENSION_E FIN_LINEA
%token ERROR_LEXICO

/* Símbolo inicial */

%start Programa

%%

/* Producciones de la GLC */

Programa:
    %empty
    ;

%%

/* Manejo de errores */

void yyerror(const char *s) {
    fprintf(stderr, "Error sintactico: %s\n", s);
}

/* Funcion principal */

int main(void) {
    return yyparse();
}