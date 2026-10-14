%option noyywrap
%option yylineno
%option noinput nounput
%x COMENTARIO 
/* 
1. noyywrap: termina el análisis al llegar al final del archivo.

2. yylineno: mantiene en yylineno el número de línea actual.

3. %x comentario: crea un estado exclusivo llamado COMENTARIO. Mientras esté activo, Flex solamente 
ejecutará las reglas que tengan <COMENTARIO> , permite la deteccion de comentarios bloque. El de linea
se reconoce utilizando la ER.

4. noinput nounput: evita la genracion de funciones de flex que no son utilizadas el lexer. 

*/
%{
/*
Inclusion de las bibliotecas, las estructuras y declaraciones (.h) que se van a utilizar.

1.parser.tab.h-> da los tokens definidos del Bison permitiendo la comunicacion entre ellos
2.ast.h-> permite la creacion de nodos del ast que se asocian con numeros, vof y identificadores.
3.tabla_simbolos.h-> da los tipos y las declaraciones necesarias para la integracion  con la tabla
  de simbolos y el analisis semantico.

*/
#include <stdio.h>
#include "parser.tab.h"
#include "ast.h"
#include "tabla_simbolos.h"
extern int opcion_verbose;

/*Es un macro que permite mostrar los tokens reconocidos en el analisis lexico cuando se usa ./cdff con -v*/
#define LEX_LOG(...) \
    do { \
        if (opcion_verbose) \
            printf(__VA_ARGS__); \
    } while (0)

/* 
Avisos para registrar el contexto de bloques e identificadores.
No modifican la secuencia de tokens de Flex. 

Estas funciones se definen en parser.y

*/
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
        /* palabras reservadas del lenguaje */

       /* reconocimiento de operaciones iterativas */ 
"si"                  { parser_indicar_bloque("si", yylineno); LEX_LOG("SI: %s\n", yytext); return SI; }
"sino"                { parser_indicar_bloque("sino", yylineno); LEX_LOG("SINO: %s\n", yytext); return SINO; }
"sinosi"              { parser_indicar_bloque("sinosi", yylineno); LEX_LOG("SINOSI: %s\n", yytext); return SINOSI; }
"mientras"            { parser_indicar_bloque("mientras", yylineno); LEX_LOG("MIENTRAS: %s\n", yytext); return MIENTRAS; }
"para"                { parser_abrir_ambito_para_lexico(yylineno); LEX_LOG("PARA: %s\n", yytext); return PARA; }
"devuelva"            { LEX_LOG("DEVUELVA: %s\n", yytext); return DEVUELVA; }
"defina"              { parser_avisar_defina(); LEX_LOG("DEFINA: %s\n", yytext); return DEFINA; }
"rompa"               { LEX_LOG("ROMPA: %s\n", yytext); return ROMPA; }
"principal"           { parser_marcar_principal(yylineno); LEX_LOG("PRINCIPAL: %s\n", yytext); return PRINCIPAL; }

        /* reconocimiento de tipos de arrays */
"lista"               { LEX_LOG("LISTA: %s\n", yytext); return LISTA; }
"matriz"              { LEX_LOG("MATRIZ: %s\n", yytext); return MATRIZ; }

    /* Reconocimiento de tipos */
"vof"                 { parser_avisar_tipo(TIPO_VOF); LEX_LOG("VOF: %s\n", yytext); return VOF; }
"entero"              { parser_avisar_tipo(TIPO_ENTERO); LEX_LOG("ENTERO: %s\n", yytext); return ENTERO; }
"vacio"               { parser_avisar_tipo(TIPO_VACIO); LEX_LOG("VACIO: %s\n", yytext); return VACIO; }

        /* Reconocimiento de operadores aritmeticos */
"+"                   { LEX_LOG("SUMA: %s\n", yytext); return '+'; }
"-"                   { LEX_LOG("RESTA: %s\n", yytext); return '-'; }
"*"                   { LEX_LOG("MULTIPLICACION: %s\n", yytext); return '*'; }
"/"                   { LEX_LOG("DIVISION: %s\n", yytext); return '/'; }
"**"                  { LEX_LOG("POTENCIA: %s\n", yytext); return POTENCIA; }
"%"                   { LEX_LOG("MODULO: %s\n", yytext); return '%'; }
"="                   { LEX_LOG("ASIGNACION: %s\n", yytext); return '='; }


        /* Reconocimiento de comparadores */
"=="                  { LEX_LOG("IGUALDAD: %s\n", yytext); return IGUALDAD; }
"!="                  { LEX_LOG("DIFERENTE: %s\n", yytext); return DIFERENTE; }
"<="                  { LEX_LOG("MENOR_IGUAL: %s\n", yytext); return MENOR_IGUAL; }
">="                  { LEX_LOG("MAYOR_IGUAL: %s\n", yytext); return MAYOR_IGUAL; }
"<"                   { LEX_LOG("MENOR: %s\n", yytext); return '<'; }
">"                   { LEX_LOG("MAYOR: %s\n", yytext); return '>'; }

        /* Reconocimiento de comparadores logicos */
"$$"                  { LEX_LOG("AND: %s\n", yytext); return AND; }
"--"                  { LEX_LOG("OR: %s\n", yytext); return OR; }

        /* Reconocimiento de valores vof */
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

        /* reconocimiento de delimitadores y ambitos lexicos */
"{"     { parser_abrir_ambito_lexico(yylineno); LEX_LOG("LLAVE_IZQ: %s\n", yytext); return '{'; }
"}"     { parser_cerrar_ambito_lexico(); LEX_LOG("LLAVE_DER: %s\n", yytext); return '}'; }
"("     { LEX_LOG("PARENTESIS_IZQ: %s\n", yytext); return '('; }
")"     { LEX_LOG("PARENTESIS_DER: %s\n", yytext); return ')'; }
"["     { LEX_LOG("CORCHETE_IZQ: %s\n", yytext); return '['; }
"]"     { LEX_LOG("CORCHETE_DER: %s\n", yytext); return ']'; }
","     { LEX_LOG("COMA: %s\n", yytext); return ','; }
\"      { LEX_LOG("COMILLA: %s\n", yytext); return '"'; }

    /* Permite detectar numeros que comienzan con 0s a la izquierda y retorna error lexico. */
0[0-9]+ {
    printf(
        "NUMERO ENTERO NO RECONOCIDO en línea %d: %s\n",
        yylineno,
        yytext
    );
    return ERROR_LEXICO;
}

    /* ER para numeros validos */
0|[1-9][0-9]* {
    LEX_LOG("NUMERO: %s\n", yytext);
/* Guardamos el numero para usarlo en el AST */
    yylval = ast_crear(AST_NUMERO, yytext);
    return NUMERO;
}

    /* ER para reconocimiento de identificadores */
[a-z][a-zA-Z_0-9]* {
    LEX_LOG("IDENTIFICADOR: %s\n", yytext);
/* Guardamos el nombre de la variable */
    yylval = ast_crear(AST_VARIABLE, yytext);
    parser_marcar_identificador(yylval);
    return IDENTIFICADOR;
}

    /* ER para la importacion de documentos */
"#traigase"           { LEX_LOG("TRAIGASE: %s\n", yytext); return TRAIGASE; }
".e"                  { LEX_LOG("EXTENSION_E: %s\n", yytext); return EXTENSION_E; }

    /*
    ER para reconocimiento de comentario de linea (^) y bloque (^^) tambien ignora 
    los saltos de linea y tambien da error lexico si se detecta un comentario de 
    bloque sin terminar
    */

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

    /* Fin de linea */
":)" { LEX_LOG("FIN_LINEA: %s\n", yytext); return FIN_LINEA; }

    /* Permite consumir espacios, tabulacion (TAB) y los saltos de linea */
[ \t\r\n]+  ;

    /* da error lexico en cualquier caracter que no se encuentre en ER previas */
. {
    printf("ERROR LEXICO en linea %d: %s\n", yylineno, yytext);
    return ERROR_LEXICO;
}

%%
