/* ARCHIVO GENERADO: NO EDITAR MANUALMENTE.
 * python3 ide/tools/generate_ll1.py; gramática fija de parser.y.
 * M[A,a] contiene número de producción, -1 equivale a error.
 * Ver ide/DEFENSA_LL1.md para FIRST/FOLLOW y correspondencias. */
#ifndef CDFF_LL1_TABLE_H
#define CDFF_LL1_TABLE_H
#include <stdint.h>
enum { T_EOF, T_TRAIGASE, T_QUOTE, T_EXT_E, T_DEFINA, T_ENTERO, T_VOF, T_VACIO, T_PRINCIPAL, T_LISTA, T_MATRIZ, T_SI, T_SINO, T_SINOSI, T_MIENTRAS, T_PARA, T_DEVUELVA, T_ROMPA, T_ID, T_NUM, T_TRUE, T_FALSE, T_LP, T_RP, T_LB, T_RB, T_LC, T_RC, T_COMMA, T_FIN, T_ASSIGN, T_OR, T_AND, T_COMP, T_PLUS, T_MINUS, T_MUL, T_DIV, T_MOD, T_POW, T_COUNT };
enum { N_PROGRAM, N_IMPORTS, N_IMPORT, N_GLOBALS, N_UNITS, N_DEF_TAIL, N_SIGNATURE, N_VACIO_TAIL, N_PARAMS, N_PARAMS_MORE, N_PARAM, N_PARAM_ENTERO, N_DIM, N_BLOCK, N_STMTS, N_STMT, N_DECL, N_DECL_ENTERO, N_INIT, N_INIT_LIST, N_INIT_MATRIX, N_ROWS, N_ROWS_MORE, N_ROW, N_ELEMENTS, N_ELEMENTS_MORE, N_ID_STMT, N_ACCESS_TAIL, N_ACCESS_SECOND, N_IF, N_IF_MORE, N_WHILE, N_FOR, N_FOR_INIT, N_ASSIGNMENT, N_RETURN, N_BREAK, N_ARGS, N_ARGS_MORE, N_EXPR, N_OR_TAIL, N_AND_EXPR, N_AND_TAIL, N_COMP_EXPR, N_COMP_TAIL, N_ARIT, N_ARIT_TAIL, N_TERM, N_TERM_TAIL, N_POWER, N_POWER_TAIL, N_UNARY, N_PRIMARY, N_PRIMARY_SUFFIX, N_COUNT };
typedef struct { uint8_t length; int16_t rhs[14]; } LL1Production;
/* Identificador de terminal/NT: 0..T_COUNT-1 terminal; T_COUNT+N_* no terminal. */
static const LL1Production LL1_PRODUCTIONS[105] = {
    {3, {(T_COUNT+N_IMPORTS), (T_COUNT+N_GLOBALS), (T_COUNT+N_UNITS)}}, /* PROGRAM -> IMPORTS GLOBALS UNITS */
    {2, {(T_COUNT+N_IMPORT), (T_COUNT+N_IMPORTS)}}, /* IMPORTS -> IMPORT IMPORTS */
    {0, {}}, /* IMPORTS -> ε */
    {6, {T_TRAIGASE, T_QUOTE, T_ID, T_EXT_E, T_QUOTE, T_FIN}}, /* IMPORT -> TRAIGASE QUOTE ID EXT_E QUOTE FIN */
    {2, {(T_COUNT+N_DECL), (T_COUNT+N_GLOBALS)}}, /* GLOBALS -> DECL GLOBALS */
    {0, {}}, /* GLOBALS -> ε */
    {2, {T_DEFINA, (T_COUNT+N_DEF_TAIL)}}, /* UNITS -> DEFINA DEF_TAIL */
    {0, {}}, /* UNITS -> ε */
    {3, {T_ENTERO, (T_COUNT+N_SIGNATURE), (T_COUNT+N_UNITS)}}, /* DEF_TAIL -> ENTERO SIGNATURE UNITS */
    {3, {T_VOF, (T_COUNT+N_SIGNATURE), (T_COUNT+N_UNITS)}}, /* DEF_TAIL -> VOF SIGNATURE UNITS */
    {2, {T_VACIO, (T_COUNT+N_VACIO_TAIL)}}, /* DEF_TAIL -> VACIO VACIO_TAIL */
    {6, {T_ID, T_LP, (T_COUNT+N_PARAMS), T_RP, (T_COUNT+N_BLOCK), T_FIN}}, /* SIGNATURE -> ID LP PARAMS RP BLOCK FIN */
    {7, {T_ID, T_LP, (T_COUNT+N_PARAMS), T_RP, (T_COUNT+N_BLOCK), T_FIN, (T_COUNT+N_UNITS)}}, /* VACIO_TAIL -> ID LP PARAMS RP BLOCK FIN UNITS */
    {5, {T_PRINCIPAL, T_LP, T_RP, (T_COUNT+N_BLOCK), T_FIN}}, /* VACIO_TAIL -> PRINCIPAL LP RP BLOCK FIN */
    {2, {(T_COUNT+N_PARAM), (T_COUNT+N_PARAMS_MORE)}}, /* PARAMS -> PARAM PARAMS_MORE */
    {0, {}}, /* PARAMS -> ε */
    {3, {T_COMMA, (T_COUNT+N_PARAM), (T_COUNT+N_PARAMS_MORE)}}, /* PARAMS_MORE -> COMMA PARAM PARAMS_MORE */
    {0, {}}, /* PARAMS_MORE -> ε */
    {2, {T_ENTERO, (T_COUNT+N_PARAM_ENTERO)}}, /* PARAM -> ENTERO PARAM_ENTERO */
    {2, {T_VOF, T_ID}}, /* PARAM -> VOF ID */
    {1, {T_ID}}, /* PARAM_ENTERO -> ID */
    {5, {T_LISTA, T_ID, T_LB, (T_COUNT+N_DIM), T_RB}}, /* PARAM_ENTERO -> LISTA ID LB DIM RB */
    {8, {T_MATRIZ, T_ID, T_LB, (T_COUNT+N_DIM), T_RB, T_LB, (T_COUNT+N_DIM), T_RB}}, /* PARAM_ENTERO -> MATRIZ ID LB DIM RB LB DIM RB */
    {1, {T_NUM}}, /* DIM -> NUM */
    {1, {T_ID}}, /* DIM -> ID */
    {3, {T_LC, (T_COUNT+N_STMTS), T_RC}}, /* BLOCK -> LC STMTS RC */
    {2, {(T_COUNT+N_STMT), (T_COUNT+N_STMTS)}}, /* STMTS -> STMT STMTS */
    {0, {}}, /* STMTS -> ε */
    {1, {(T_COUNT+N_DECL)}}, /* STMT -> DECL */
    {2, {T_ID, (T_COUNT+N_ID_STMT)}}, /* STMT -> ID ID_STMT */
    {1, {(T_COUNT+N_IF)}}, /* STMT -> IF */
    {1, {(T_COUNT+N_WHILE)}}, /* STMT -> WHILE */
    {1, {(T_COUNT+N_FOR)}}, /* STMT -> FOR */
    {1, {(T_COUNT+N_RETURN)}}, /* STMT -> RETURN */
    {1, {(T_COUNT+N_BREAK)}}, /* STMT -> BREAK */
    {2, {T_ENTERO, (T_COUNT+N_DECL_ENTERO)}}, /* DECL -> ENTERO DECL_ENTERO */
    {4, {T_VOF, T_ID, (T_COUNT+N_INIT), T_FIN}}, /* DECL -> VOF ID INIT FIN */
    {3, {T_ID, (T_COUNT+N_INIT), T_FIN}}, /* DECL_ENTERO -> ID INIT FIN */
    {7, {T_LISTA, T_ID, T_LB, (T_COUNT+N_DIM), T_RB, (T_COUNT+N_INIT_LIST), T_FIN}}, /* DECL_ENTERO -> LISTA ID LB DIM RB INIT_LIST FIN */
    {10, {T_MATRIZ, T_ID, T_LB, (T_COUNT+N_DIM), T_RB, T_LB, (T_COUNT+N_DIM), T_RB, (T_COUNT+N_INIT_MATRIX), T_FIN}}, /* DECL_ENTERO -> MATRIZ ID LB DIM RB LB DIM RB INIT_MATRIX FIN */
    {2, {T_ASSIGN, (T_COUNT+N_EXPR)}}, /* INIT -> ASSIGN EXPR */
    {0, {}}, /* INIT -> ε */
    {4, {T_ASSIGN, T_LB, (T_COUNT+N_ELEMENTS), T_RB}}, /* INIT_LIST -> ASSIGN LB ELEMENTS RB */
    {0, {}}, /* INIT_LIST -> ε */
    {4, {T_ASSIGN, T_LB, (T_COUNT+N_ROWS), T_RB}}, /* INIT_MATRIX -> ASSIGN LB ROWS RB */
    {0, {}}, /* INIT_MATRIX -> ε */
    {2, {(T_COUNT+N_ROW), (T_COUNT+N_ROWS_MORE)}}, /* ROWS -> ROW ROWS_MORE */
    {3, {T_COMMA, (T_COUNT+N_ROW), (T_COUNT+N_ROWS_MORE)}}, /* ROWS_MORE -> COMMA ROW ROWS_MORE */
    {0, {}}, /* ROWS_MORE -> ε */
    {3, {T_LB, (T_COUNT+N_ELEMENTS), T_RB}}, /* ROW -> LB ELEMENTS RB */
    {2, {(T_COUNT+N_EXPR), (T_COUNT+N_ELEMENTS_MORE)}}, /* ELEMENTS -> EXPR ELEMENTS_MORE */
    {0, {}}, /* ELEMENTS -> ε */
    {3, {T_COMMA, (T_COUNT+N_EXPR), (T_COUNT+N_ELEMENTS_MORE)}}, /* ELEMENTS_MORE -> COMMA EXPR ELEMENTS_MORE */
    {0, {}}, /* ELEMENTS_MORE -> ε */
    {4, {T_LP, (T_COUNT+N_ARGS), T_RP, T_FIN}}, /* ID_STMT -> LP ARGS RP FIN */
    {4, {(T_COUNT+N_ACCESS_TAIL), T_ASSIGN, (T_COUNT+N_EXPR), T_FIN}}, /* ID_STMT -> ACCESS_TAIL ASSIGN EXPR FIN */
    {4, {T_LB, (T_COUNT+N_ARIT), T_RB, (T_COUNT+N_ACCESS_SECOND)}}, /* ACCESS_TAIL -> LB ARIT RB ACCESS_SECOND */
    {0, {}}, /* ACCESS_TAIL -> ε */
    {3, {T_LB, (T_COUNT+N_ARIT), T_RB}}, /* ACCESS_SECOND -> LB ARIT RB */
    {0, {}}, /* ACCESS_SECOND -> ε */
    {6, {T_SI, T_LP, (T_COUNT+N_EXPR), T_RP, (T_COUNT+N_BLOCK), (T_COUNT+N_IF_MORE)}}, /* IF -> SI LP EXPR RP BLOCK IF_MORE */
    {1, {T_FIN}}, /* IF_MORE -> FIN */
    {3, {T_SINO, (T_COUNT+N_BLOCK), T_FIN}}, /* IF_MORE -> SINO BLOCK FIN */
    {6, {T_SINOSI, T_LP, (T_COUNT+N_EXPR), T_RP, (T_COUNT+N_BLOCK), (T_COUNT+N_IF_MORE)}}, /* IF_MORE -> SINOSI LP EXPR RP BLOCK IF_MORE */
    {6, {T_MIENTRAS, T_LP, (T_COUNT+N_EXPR), T_RP, (T_COUNT+N_BLOCK), T_FIN}}, /* WHILE -> MIENTRAS LP EXPR RP BLOCK FIN */
    {10, {T_PARA, T_LP, (T_COUNT+N_FOR_INIT), T_COMMA, (T_COUNT+N_EXPR), T_COMMA, (T_COUNT+N_ASSIGNMENT), T_RP, (T_COUNT+N_BLOCK), T_FIN}}, /* FOR -> PARA LP FOR_INIT COMMA EXPR COMMA ASSIGNMENT RP BLOCK FIN */
    {4, {T_ENTERO, T_ID, T_ASSIGN, (T_COUNT+N_EXPR)}}, /* FOR_INIT -> ENTERO ID ASSIGN EXPR */
    {1, {(T_COUNT+N_ASSIGNMENT)}}, /* FOR_INIT -> ASSIGNMENT */
    {4, {T_ID, (T_COUNT+N_ACCESS_TAIL), T_ASSIGN, (T_COUNT+N_EXPR)}}, /* ASSIGNMENT -> ID ACCESS_TAIL ASSIGN EXPR */
    {3, {T_DEVUELVA, (T_COUNT+N_EXPR), T_FIN}}, /* RETURN -> DEVUELVA EXPR FIN */
    {2, {T_ROMPA, T_FIN}}, /* BREAK -> ROMPA FIN */
    {2, {(T_COUNT+N_EXPR), (T_COUNT+N_ARGS_MORE)}}, /* ARGS -> EXPR ARGS_MORE */
    {0, {}}, /* ARGS -> ε */
    {3, {T_COMMA, (T_COUNT+N_EXPR), (T_COUNT+N_ARGS_MORE)}}, /* ARGS_MORE -> COMMA EXPR ARGS_MORE */
    {0, {}}, /* ARGS_MORE -> ε */
    {2, {(T_COUNT+N_AND_EXPR), (T_COUNT+N_OR_TAIL)}}, /* EXPR -> AND_EXPR OR_TAIL */
    {3, {T_OR, (T_COUNT+N_AND_EXPR), (T_COUNT+N_OR_TAIL)}}, /* OR_TAIL -> OR AND_EXPR OR_TAIL */
    {0, {}}, /* OR_TAIL -> ε */
    {2, {(T_COUNT+N_COMP_EXPR), (T_COUNT+N_AND_TAIL)}}, /* AND_EXPR -> COMP_EXPR AND_TAIL */
    {3, {T_AND, (T_COUNT+N_COMP_EXPR), (T_COUNT+N_AND_TAIL)}}, /* AND_TAIL -> AND COMP_EXPR AND_TAIL */
    {0, {}}, /* AND_TAIL -> ε */
    {2, {(T_COUNT+N_ARIT), (T_COUNT+N_COMP_TAIL)}}, /* COMP_EXPR -> ARIT COMP_TAIL */
    {2, {T_COMP, (T_COUNT+N_ARIT)}}, /* COMP_TAIL -> COMP ARIT */
    {0, {}}, /* COMP_TAIL -> ε */
    {2, {(T_COUNT+N_TERM), (T_COUNT+N_ARIT_TAIL)}}, /* ARIT -> TERM ARIT_TAIL */
    {3, {T_PLUS, (T_COUNT+N_TERM), (T_COUNT+N_ARIT_TAIL)}}, /* ARIT_TAIL -> PLUS TERM ARIT_TAIL */
    {3, {T_MINUS, (T_COUNT+N_TERM), (T_COUNT+N_ARIT_TAIL)}}, /* ARIT_TAIL -> MINUS TERM ARIT_TAIL */
    {0, {}}, /* ARIT_TAIL -> ε */
    {2, {(T_COUNT+N_POWER), (T_COUNT+N_TERM_TAIL)}}, /* TERM -> POWER TERM_TAIL */
    {3, {T_MUL, (T_COUNT+N_POWER), (T_COUNT+N_TERM_TAIL)}}, /* TERM_TAIL -> MUL POWER TERM_TAIL */
    {3, {T_DIV, (T_COUNT+N_POWER), (T_COUNT+N_TERM_TAIL)}}, /* TERM_TAIL -> DIV POWER TERM_TAIL */
    {3, {T_MOD, (T_COUNT+N_POWER), (T_COUNT+N_TERM_TAIL)}}, /* TERM_TAIL -> MOD POWER TERM_TAIL */
    {0, {}}, /* TERM_TAIL -> ε */
    {2, {(T_COUNT+N_UNARY), (T_COUNT+N_POWER_TAIL)}}, /* POWER -> UNARY POWER_TAIL */
    {2, {T_POW, (T_COUNT+N_POWER)}}, /* POWER_TAIL -> POW POWER */
    {0, {}}, /* POWER_TAIL -> ε */
    {2, {T_MINUS, (T_COUNT+N_UNARY)}}, /* UNARY -> MINUS UNARY */
    {1, {(T_COUNT+N_PRIMARY)}}, /* UNARY -> PRIMARY */
    {1, {T_NUM}}, /* PRIMARY -> NUM */
    {1, {T_TRUE}}, /* PRIMARY -> TRUE */
    {1, {T_FALSE}}, /* PRIMARY -> FALSE */
    {2, {T_ID, (T_COUNT+N_PRIMARY_SUFFIX)}}, /* PRIMARY -> ID PRIMARY_SUFFIX */
    {3, {T_LP, (T_COUNT+N_EXPR), T_RP}}, /* PRIMARY -> LP EXPR RP */
    {3, {T_LP, (T_COUNT+N_ARGS), T_RP}}, /* PRIMARY_SUFFIX -> LP ARGS RP */
    {1, {(T_COUNT+N_ACCESS_TAIL)}}, /* PRIMARY_SUFFIX -> ACCESS_TAIL */
};
/* TABLA PREDICTIVA LL(1) M[A,a], CONSTRUIDA CON FIRST Y FOLLOW. */
static const int16_t LL1_TABLE[N_COUNT][T_COUNT] = {
    {0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* PROGRAM */
    {2, 1, -1, -1, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* IMPORTS */
    {-1, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* IMPORT */
    {5, -1, -1, -1, 5, 4, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* GLOBALS */
    {7, -1, -1, -1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* UNITS */
    {-1, -1, -1, -1, -1, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* DEF_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* SIGNATURE */
    {-1, -1, -1, -1, -1, -1, -1, -1, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* VACIO_TAIL */
    {-1, -1, -1, -1, -1, 14, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* PARAMS */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, -1, -1, -1, -1, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* PARAMS_MORE */
    {-1, -1, -1, -1, -1, 18, 19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* PARAM */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 22, -1, -1, -1, -1, -1, -1, -1, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* PARAM_ENTERO */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 24, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* DIM */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* BLOCK */
    {-1, -1, -1, -1, -1, 26, 26, -1, -1, -1, -1, 26, -1, -1, 26, 26, 26, 26, 26, -1, -1, -1, -1, -1, -1, -1, -1, 27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* STMTS */
    {-1, -1, -1, -1, -1, 28, 28, -1, -1, -1, -1, 30, -1, -1, 31, 32, 33, 34, 29, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* STMT */
    {-1, -1, -1, -1, -1, 35, 36, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* DECL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 38, 39, -1, -1, -1, -1, -1, -1, -1, 37, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* DECL_ENTERO */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 41, 40, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* INIT */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 43, 42, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* INIT_LIST */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 45, 44, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* INIT_MATRIX */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 46, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ROWS */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 48, -1, -1, 47, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ROWS_MORE */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ROW */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, 50, 50, 50, 50, -1, -1, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1, -1, -1}, /* ELEMENTS */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 53, -1, -1, 52, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ELEMENTS_MORE */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 54, -1, 55, -1, -1, -1, -1, -1, 55, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ID_STMT */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 57, 56, 57, -1, -1, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57}, /* ACCESS_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 59, 58, 59, -1, -1, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59}, /* ACCESS_SECOND */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 60, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* IF */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, 63, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 61, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* IF_MORE */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 64, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* WHILE */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 65, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* FOR */
    {-1, -1, -1, -1, -1, 66, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 67, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* FOR_INIT */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 68, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ASSIGNMENT */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 69, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* RETURN */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 70, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* BREAK */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 71, 71, 71, 71, 71, 72, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 71, -1, -1, -1, -1}, /* ARGS */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 74, -1, -1, -1, -1, 73, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* ARGS_MORE */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 75, 75, 75, 75, 75, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 75, -1, -1, -1, -1}, /* EXPR */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 77, -1, 77, -1, -1, 77, 77, -1, 76, -1, -1, -1, -1, -1, -1, -1, -1}, /* OR_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 78, 78, 78, 78, 78, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 78, -1, -1, -1, -1}, /* AND_EXPR */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 80, -1, 80, -1, -1, 80, 80, -1, 80, 79, -1, -1, -1, -1, -1, -1, -1}, /* AND_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 81, 81, 81, 81, 81, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 81, -1, -1, -1, -1}, /* COMP_EXPR */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 83, -1, 83, -1, -1, 83, 83, -1, 83, 83, 82, -1, -1, -1, -1, -1, -1}, /* COMP_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, 84, 84, 84, 84, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84, -1, -1, -1, -1}, /* ARIT */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 87, -1, 87, -1, -1, 87, 87, -1, 87, 87, 87, 85, 86, -1, -1, -1, -1}, /* ARIT_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 88, 88, 88, 88, 88, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 88, -1, -1, -1, -1}, /* TERM */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 92, -1, 92, -1, -1, 92, 92, -1, 92, 92, 92, 92, 92, 89, 90, 91, -1}, /* TERM_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 93, 93, 93, 93, 93, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 93, -1, -1, -1, -1}, /* POWER */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 95, -1, 95, -1, -1, 95, 95, -1, 95, 95, 95, 95, 95, 95, 95, 95, 94}, /* POWER_TAIL */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 97, 97, 97, 97, 97, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 96, -1, -1, -1, -1}, /* UNARY */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 101, 98, 99, 100, 102, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* PRIMARY */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 103, 104, 104, 104, -1, -1, 104, 104, -1, 104, 104, 104, 104, 104, 104, 104, 104, 104}, /* PRIMARY_SUFFIX */
};
/* FIRST de cada NT (sin epsilon); LL1_NULLABLE almacena epsilon. */
static const uint64_t LL1_FIRST[N_COUNT] = {UINT64_C(114), UINT64_C(2), UINT64_C(2), UINT64_C(96), UINT64_C(16), UINT64_C(224), UINT64_C(262144), UINT64_C(262400), UINT64_C(96), UINT64_C(268435456), UINT64_C(96), UINT64_C(263680), UINT64_C(786432), UINT64_C(67108864), UINT64_C(510048), UINT64_C(510048), UINT64_C(96), UINT64_C(263680), UINT64_C(1073741824), UINT64_C(1073741824), UINT64_C(1073741824), UINT64_C(16777216), UINT64_C(268435456), UINT64_C(16777216), UINT64_C(34367864832), UINT64_C(268435456), UINT64_C(1094713344), UINT64_C(16777216), UINT64_C(16777216), UINT64_C(2048), UINT64_C(536883200), UINT64_C(16384), UINT64_C(32768), UINT64_C(262176), UINT64_C(262144), UINT64_C(65536), UINT64_C(131072), UINT64_C(34367864832), UINT64_C(268435456), UINT64_C(34367864832), UINT64_C(2147483648), UINT64_C(34367864832), UINT64_C(4294967296), UINT64_C(34367864832), UINT64_C(8589934592), UINT64_C(34367864832), UINT64_C(51539607552), UINT64_C(34367864832), UINT64_C(481036337152), UINT64_C(34367864832), UINT64_C(549755813888), UINT64_C(34367864832), UINT64_C(8126464), UINT64_C(20971520)};
static const unsigned char LL1_NULLABLE[N_COUNT] = {1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1};
static const uint64_t LL1_FOLLOW[N_COUNT] = {UINT64_C(1), UINT64_C(113), UINT64_C(115), UINT64_C(17), UINT64_C(1), UINT64_C(1), UINT64_C(17), UINT64_C(1), UINT64_C(8388608), UINT64_C(8388608), UINT64_C(276824064), UINT64_C(276824064), UINT64_C(33554432), UINT64_C(536883200), UINT64_C(134217728), UINT64_C(134727776), UINT64_C(134727793), UINT64_C(134727793), UINT64_C(536870912), UINT64_C(536870912), UINT64_C(536870912), UINT64_C(33554432), UINT64_C(33554432), UINT64_C(301989888), UINT64_C(33554432), UINT64_C(33554432), UINT64_C(134727776), UINT64_C(1099285135360), UINT64_C(1099285135360), UINT64_C(134727776), UINT64_C(134727776), UINT64_C(134727776), UINT64_C(134727776), UINT64_C(268435456), UINT64_C(276824064), UINT64_C(134727776), UINT64_C(134727776), UINT64_C(8388608), UINT64_C(8388608), UINT64_C(847249408), UINT64_C(847249408), UINT64_C(2994733056), UINT64_C(2994733056), UINT64_C(7289700352), UINT64_C(7289700352), UINT64_C(15879634944), UINT64_C(15879634944), UINT64_C(67419242496), UINT64_C(67419242496), UINT64_C(548455579648), UINT64_C(548455579648), UINT64_C(1098211393536), UINT64_C(1098211393536), UINT64_C(1098211393536)};
static const char *const LL1_TERMINAL_NAMES[T_COUNT] = {"EOF", "TRAIGASE", "QUOTE", "EXT_E", "DEFINA", "ENTERO", "VOF", "VACIO", "PRINCIPAL", "LISTA", "MATRIZ", "SI", "SINO", "SINOSI", "MIENTRAS", "PARA", "DEVUELVA", "ROMPA", "ID", "NUM", "TRUE", "FALSE", "LP", "RP", "LB", "RB", "LC", "RC", "COMMA", "FIN", "ASSIGN", "OR", "AND", "COMP", "PLUS", "MINUS", "MUL", "DIV", "MOD", "POW"};
static const char *const LL1_NONTERMINAL_NAMES[N_COUNT] = {"PROGRAM", "IMPORTS", "IMPORT", "GLOBALS", "UNITS", "DEF_TAIL", "SIGNATURE", "VACIO_TAIL", "PARAMS", "PARAMS_MORE", "PARAM", "PARAM_ENTERO", "DIM", "BLOCK", "STMTS", "STMT", "DECL", "DECL_ENTERO", "INIT", "INIT_LIST", "INIT_MATRIX", "ROWS", "ROWS_MORE", "ROW", "ELEMENTS", "ELEMENTS_MORE", "ID_STMT", "ACCESS_TAIL", "ACCESS_SECOND", "IF", "IF_MORE", "WHILE", "FOR", "FOR_INIT", "ASSIGNMENT", "RETURN", "BREAK", "ARGS", "ARGS_MORE", "EXPR", "OR_TAIL", "AND_EXPR", "AND_TAIL", "COMP_EXPR", "COMP_TAIL", "ARIT", "ARIT_TAIL", "TERM", "TERM_TAIL", "POWER", "POWER_TAIL", "UNARY", "PRIMARY", "PRIMARY_SUFFIX"};
#endif /* CDFF_LL1_TABLE_H */
