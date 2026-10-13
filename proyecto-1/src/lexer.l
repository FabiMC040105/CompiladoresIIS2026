%option noyywrap
%option yylineno
%option noinput nounput
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
#include "tabla_simbolos.h"
extern int opcion_verbose;

#define LEX_LOG(...) \
    do { \
        if (opcion_verbose) \
            printf(__VA_ARGS__); \
    } while (0)
/* Avisos para registrar el contexto de bloques e identificadores.
   No modifican la secuencia de tokens de Flex. */
void parser_abrir_ambito_lexico(int linea);
void parser_abrir_ambito_para_lexico(int linea);
void parser_indicar_bloque(const char *tipo, int linea);
void parser_cerrar_ambito_lexico(void);
void parser_marcar_identificador(NodoAST *nodo);
void parser_avisar_defina(void);
void parser_avisar_tipo(TipoDato tipo);
void parser_marcar_principal(int linea);
%}

%%

"si"                  { parser_indicar_bloque("si", yylineno); LEX_LOG("SI: %s\n", yytext); return SI; }
"sino"                { parser_indicar_bloque("sino", yylineno); LEX_LOG("SINO: %s\n", yytext); return SINO; }
"sinosi"              { parser_indicar_bloque("sinosi", yylineno); LEX_LOG("SINOSI: %s\n", yytext); return SINOSI; }
"mientras"            { parser_indicar_bloque("mientras", yylineno); LEX_LOG("MIENTRAS: %s\n", yytext); return MIENTRAS; }
"para"                { parser_abrir_ambito_para_lexico(yylineno); LEX_LOG("PARA: %s\n", yytext); return PARA; }
"devuelva"            { LEX_LOG("DEVUELVA: %s\n", yytext); return DEVUELVA; }
"defina"              { parser_avisar_defina(); LEX_LOG("DEFINA: %s\n", yytext); return DEFINA; }
"rompa"               { LEX_LOG("ROMPA: %s\n", yytext); return ROMPA; }
"principal"           { parser_marcar_principal(yylineno); LEX_LOG("PRINCIPAL: %s\n", yytext); return PRINCIPAL; }

"lista"               { LEX_LOG("LISTA: %s\n", yytext); return LISTA; }
"matriz"              { LEX_LOG("MATRIZ: %s\n", yytext); return MATRIZ; }

"vof"                 { parser_avisar_tipo(TIPO_VOF); LEX_LOG("VOF: %s\n", yytext); return VOF; }
"entero"              { parser_avisar_tipo(TIPO_ENTERO); LEX_LOG("ENTERO: %s\n", yytext); return ENTERO; }
"vacio"               { parser_avisar_tipo(TIPO_VACIO); LEX_LOG("VACIO: %s\n", yytext); return VACIO; }

"+"                   { LEX_LOG("SUMA: %s\n", yytext); return '+'; }
"-"                   { LEX_LOG("RESTA: %s\n", yytext); return '-'; }
"*"                   { LEX_LOG("MULTIPLICACION: %s\n", yytext); return '*'; }
"/"                   { LEX_LOG("DIVISION: %s\n", yytext); return '/'; }
"**"                  { LEX_LOG("POTENCIA: %s\n", yytext); return POTENCIA; }
"%"                   { LEX_LOG("MODULO: %s\n", yytext); return '%'; }
"="                   { LEX_LOG("ASIGNACION: %s\n", yytext); return '='; }

"=="                  { LEX_LOG("IGUALDAD: %s\n", yytext); return IGUALDAD; }
"!="                  { LEX_LOG("DIFERENTE: %s\n", yytext); return DIFERENTE; }
"<="                  { LEX_LOG("MENOR_IGUAL: %s\n", yytext); return MENOR_IGUAL; }
">="                  { LEX_LOG("MAYOR_IGUAL: %s\n", yytext); return MAYOR_IGUAL; }
"<"                   { LEX_LOG("MENOR: %s\n", yytext); return '<'; }
">"                   { LEX_LOG("MAYOR: %s\n", yytext); return '>'; }

"$$"                  { LEX_LOG("AND: %s\n", yytext); return AND; }
"--"                  { LEX_LOG("OR: %s\n", yytext); return OR; }

"verdadero" {
    LEX_LOG("VERDADERO: %s\n", yytext);
    yylval = ast_crear(AST_BOOLEANO, yytext);
    return VERDADERO;
}

"falso" {
    LEX_LOG("FALSO: %s\n", yytext);
    yylval = ast_crear(AST_BOOLEANO, yytext);
    return FALSO;
}

"{"                   { parser_abrir_ambito_lexico(yylineno); LEX_LOG("LLAVE_IZQ: %s\n", yytext); return '{'; }
"}"                   { parser_cerrar_ambito_lexico(); LEX_LOG("LLAVE_DER: %s\n", yytext); return '}'; }
"("                   { LEX_LOG("PARENTESIS_IZQ: %s\n", yytext); return '('; }
")"                   { LEX_LOG("PARENTESIS_DER: %s\n", yytext); return ')'; }
"["                   { LEX_LOG("CORCHETE_IZQ: %s\n", yytext); return '['; }
"]"                   { LEX_LOG("CORCHETE_DER: %s\n", yytext); return ']'; }
","                   { LEX_LOG("COMA: %s\n", yytext); return ','; }
\"                    { LEX_LOG("COMILLA: %s\n", yytext); return '"'; }

0[0-9]+ {
    printf(
        "NUMERO ENTERO NO RECONOCIDO en línea %d: %s\n",
        yylineno,
        yytext
    );
    return ERROR_LEXICO;
}

0|[1-9][0-9]* {
    LEX_LOG("NUMERO: %s\n", yytext);
/* Guardamos el numero para usarlo en el AST */
    yylval = ast_crear(AST_NUMERO, yytext);
    return NUMERO;
}

[a-z][a-zA-Z_0-9]* {
    LEX_LOG("IDENTIFICADOR: %s\n", yytext);
/* Guardamos el nombre de la variable */
    yylval = ast_crear(AST_VARIABLE, yytext);
    parser_marcar_identificador(yylval);
    return IDENTIFICADOR;
}

"#traigase"           { LEX_LOG("TRAIGASE: %s\n", yytext); return TRAIGASE; }
".e"                  { LEX_LOG("EXTENSION_E: %s\n", yytext); return EXTENSION_E; }

"^^" {
    LEX_LOG("INICIO_COMENTARIO_BLOQUE\n");
    BEGIN(COMENTARIO);
}

<COMENTARIO>"^^" {
    LEX_LOG("FIN_COMENTARIO_BLOQUE\n");
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

\^([^^\n][^\n]*)? { LEX_LOG("COMENTARIO_LINEA: %s\n", yytext); }

":)" { LEX_LOG("FIN_LINEA: %s\n", yytext); return FIN_LINEA; }

[ \t\r\n]+  ;

. {
    printf("ERROR LEXICO en linea %d: %s\n", yylineno, yytext);
    return ERROR_LEXICO;
}

%%
