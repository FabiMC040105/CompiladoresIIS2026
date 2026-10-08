%option noyywrap
%option yylineno
%x COMENTARIO 
/* 
1. noyywrap: termina el análisis al llegar al final del archivo.

2. yylineno: mantiene en yylineno el número de línea actual.

3. x comentario: crea un estado exclusivo llamado COMENTARIO. Mientras esté activo, Flex solamente 
ejecutará las reglas que tengan <COMENTARIO> 
*/
%{
#include <stdio.h>
#include "parser.tab.h"
#include "ast.h"
%}

%%

"si"                  { printf("SI: %s\n", yytext); return SI; }
"sino"                { printf("SINO: %s\n", yytext); return SINO; }
"sinosi"              { printf("SINOSI: %s\n", yytext); return SINOSI; }
"mientras"            { printf("MIENTRAS: %s\n", yytext); return MIENTRAS; }
"para"                { printf("PARA: %s\n", yytext); return PARA; }
"devuelva"            { printf("DEVUELVA: %s\n", yytext); return DEVUELVA; }
"defina"              { printf("DEFINA: %s\n", yytext); return DEFINA; }
"rompa"               { printf("ROMPA: %s\n", yytext); return ROMPA; }
"principal"           { printf("PRINCIPAL: %s\n", yytext); return PRINCIPAL; }

"lista"               { printf("LISTA: %s\n", yytext); return LISTA; }
"matriz"              { printf("MATRIZ: %s\n", yytext); return MATRIZ; }

"vof"                 { printf("VOF: %s\n", yytext); return VOF; }
"entero"              { printf("ENTERO: %s\n", yytext); return ENTERO; }
"vacio"               { printf("VACIO: %s\n", yytext); return VACIO; }

"+"                   { printf("SUMA: %s\n", yytext); return '+'; }
"-"                   { printf("RESTA: %s\n", yytext); return '-'; }
"*"                   { printf("MULTIPLICACION: %s\n", yytext); return '*'; }
"/"                   { printf("DIVISION: %s\n", yytext); return '/'; }
"**"                  { printf("POTENCIA: %s\n", yytext); return POTENCIA; }
"%"                   { printf("MODULO: %s\n", yytext); return '%'; }
"="                   { printf("ASIGNACION: %s\n", yytext); return '='; }

"=="                  { printf("IGUALDAD: %s\n", yytext); return IGUALDAD; }
"!="                  { printf("DIFERENTE: %s\n", yytext); return DIFERENTE; }
"<="                  { printf("MENOR_IGUAL: %s\n", yytext); return MENOR_IGUAL; }
">="                  { printf("MAYOR_IGUAL: %s\n", yytext); return MAYOR_IGUAL; }
"<"                   { printf("MENOR: %s\n", yytext); return '<'; }
">"                   { printf("MAYOR: %s\n", yytext); return '>'; }

"$$"                  { printf("AND: %s\n", yytext); return AND; }
"--"                  { printf("OR: %s\n", yytext); return OR; }

"verdadero" {
    printf("VERDADERO: %s\n", yytext);
    yylval = ast_crear(AST_BOOLEANO, yytext);
    return VERDADERO;
}

"falso" {
    printf("FALSO: %s\n", yytext);
    yylval = ast_crear(AST_BOOLEANO, yytext);
    return FALSO;
}

"{"                   { printf("LLAVE_IZQ: %s\n", yytext); return '{'; }
"}"                   { printf("LLAVE_DER: %s\n", yytext); return '}'; }
"("                   { printf("PARENTESIS_IZQ: %s\n", yytext); return '('; }
")"                   { printf("PARENTESIS_DER: %s\n", yytext); return ')'; }
"["                   { printf("CORCHETE_IZQ: %s\n", yytext); return '['; }
"]"                   { printf("CORCHETE_DER: %s\n", yytext); return ']'; }
","                   { printf("COMA: %s\n", yytext); return ','; }
\"                    { printf("COMILLA: %s\n", yytext); return '"'; }

0[0-9]+ {
    printf(
        "NUMERO ENTERO NO RECONOCIDO en línea %d: %s\n",
        yylineno,
        yytext
    );
    return ERROR_LEXICO;
}

0|[1-9][0-9]* {
    printf("NUMERO: %s\n", yytext);
/* Guardamos el numero para usarlo en el AST */
    yylval = ast_crear(AST_NUMERO, yytext);
    return NUMERO;
}

[a-z][a-zA-Z_0-9]* {
    printf("IDENTIFICADOR: %s\n", yytext);
/* Guardamos el nombre de la variable */
    yylval = ast_crear(AST_VARIABLE, yytext);
    return IDENTIFICADOR;
}

"#traigase"           { printf("TRAIGASE: %s\n", yytext); return TRAIGASE; }
".e"                  { printf("EXTENSION_E: %s\n", yytext); return EXTENSION_E; }

"^^" {
    printf("INICIO_COMENTARIO_BLOQUE\n");
    BEGIN(COMENTARIO);
}

<COMENTARIO>"^^" {
    printf("FIN_COMENTARIO_BLOQUE\n");
    BEGIN(INITIAL);
}
<COMENTARIO>.  ;
<COMENTARIO>\n {
    /* Ignorar saltos de línea dentro del comentario */
}

<COMENTARIO><<EOF>> {
    printf(
        "ERROR en linea %d: comentario de bloque sin cerrar\n",
        yylineno
    );
    return ERROR_LEXICO;
}

\^([^^\n][^\n]*)? { printf("COMENTARIO_LINEA: %s\n", yytext); }

":)" { printf("FIN_LINEA: %s\n", yytext); return FIN_LINEA; }

[ \t\r\n]+  ;

. {
    printf("ERROR LEXICO en linea %d: %s\n", yylineno, yytext);
    return ERROR_LEXICO;
}

%%
