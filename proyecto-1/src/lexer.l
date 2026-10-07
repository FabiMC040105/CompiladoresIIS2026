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
%}

%%

"si"                  { printf("SI: %s\n", yytext); }
"sino"                { printf("SINO: %s\n", yytext); }
"sinosi"              { printf("SINOSI: %s\n", yytext); }
"mientras"            { printf("MIENTRAS: %s\n", yytext); }
"para"                { printf("PARA: %s\n", yytext); }
"devuelva"            { printf("DEVUELVA: %s\n", yytext); }
"defina"              { printf("DEFINA: %s\n", yytext); }
"rompa"               { printf("ROMPA: %s\n", yytext); }
"principal"           { printf("PRINCIPAL: %s\n", yytext); }

"lista"               { printf("LISTA: %s\n", yytext); }
"matriz"              { printf("MATRIZ: %s\n", yytext); }

"vof"                 { printf("VOF: %s\n", yytext); }
"entero"              { printf("ENTERO: %s\n", yytext); }
"vacio"               { printf("VACIO: %s\n", yytext); }

"+"                   { printf("SUMA: %s\n", yytext); }
"-"                   { printf("RESTA: %s\n", yytext); }
"*"                   { printf("MULTIPLICACION: %s\n", yytext); }
"/"                   { printf("DIVISION: %s\n", yytext); }
"**"                  { printf("POTENCIA: %s\n", yytext); }
"%"                   { printf("MODULO: %s\n", yytext); }
"="                   { printf("ASIGNACION: %s\n", yytext); }

"=="                  { printf("IGUALDAD: %s\n", yytext); }
"!="                  { printf("DIFERENTE: %s\n", yytext); }
"<="                  { printf("MENOR_IGUAL: %s\n", yytext); }
">="                  { printf("MAYOR_IGUAL: %s\n", yytext); }
"<"                   { printf("MENOR: %s\n", yytext); }
">"                   { printf("MAYOR: %s\n", yytext); }

"$$"                  { printf("AND: %s\n", yytext); }
"--"                  { printf("OR: %s\n", yytext); }

"verdadero"           { printf("VERDADERO: %s\n", yytext); }
"falso"               { printf("FALSO: %s\n", yytext); }

"{"                   { printf("LLAVE_IZQ: %s\n", yytext); }
"}"                   { printf("LLAVE_DER: %s\n", yytext); }
"("                   { printf("PARENTESIS_IZQ: %s\n", yytext); }
")"                   { printf("PARENTESIS_DER: %s\n", yytext); }
"["                   { printf("CORCHETE_IZQ: %s\n", yytext); }
"]"                   { printf("CORCHETE_DER: %s\n", yytext); }
","                   { printf("COMA: %s\n", yytext); }
"“"                   { printf("COMILLA_IZQ: %s\n", yytext); }
"”"                   { printf("COMILLA_DER: %s\n", yytext); }

0[0-9]+ {
    printf(
        "NUMERO ENTERO NO RECONOCIDO en línea %d: %s\n",
        yylineno,
        yytext
    );
}

0|[1-9][0-9]* {printf("NUMERO: %s\n", yytext);}

[a-z][a-zA-Z_0-9]* { printf("IDENTIFICADOR: %s\n", yytext);}


"^^" {
    printf("INICIO_COMENTARIO_BLOQUE\n");
    BEGIN(COMENTARIO);
}

<COMENTARIO>"^^" {
    printf("FIN_COMENTARIO_BLOQUE\n");
    BEGIN(INITIAL);
}

<COMENTARIO>\n {
    /* Ignorar saltos de línea dentro del comentario */
}

<COMENTARIO><<EOF>> {
    printf(
        "ERROR en línea %d: comentario de bloque sin cerrar\n",
        yylineno
    );
    yyterminate();
}

\^([^^\n][^\n]*)? { printf("COMENTARIO_LINEA: %s\n", yytext); }

":)" {printf("FIN_LINEA: %s\n", yytext);}

[ \t\r\n]+  ;

.           { printf("NO RECONOCIDO: %s\n", yytext); }

%%

int main(void)
{
    yylex();
    return 0;
}
