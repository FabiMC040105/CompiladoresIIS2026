/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "parser.y"

#include "ast.h"

#line 53 "parser.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    SI = 258,                      /* SI  */
    SINO = 259,                    /* SINO  */
    SINOSI = 260,                  /* SINOSI  */
    MIENTRAS = 261,                /* MIENTRAS  */
    PARA = 262,                    /* PARA  */
    DEVUELVA = 263,                /* DEVUELVA  */
    DEFINA = 264,                  /* DEFINA  */
    ROMPA = 265,                   /* ROMPA  */
    PRINCIPAL = 266,               /* PRINCIPAL  */
    LISTA = 267,                   /* LISTA  */
    MATRIZ = 268,                  /* MATRIZ  */
    VOF = 269,                     /* VOF  */
    ENTERO = 270,                  /* ENTERO  */
    VACIO = 271,                   /* VACIO  */
    POTENCIA = 272,                /* POTENCIA  */
    IGUALDAD = 273,                /* IGUALDAD  */
    DIFERENTE = 274,               /* DIFERENTE  */
    MENOR_IGUAL = 275,             /* MENOR_IGUAL  */
    MAYOR_IGUAL = 276,             /* MAYOR_IGUAL  */
    AND = 277,                     /* AND  */
    OR = 278,                      /* OR  */
    VERDADERO = 279,               /* VERDADERO  */
    FALSO = 280,                   /* FALSO  */
    NUMERO = 281,                  /* NUMERO  */
    IDENTIFICADOR = 282,           /* IDENTIFICADOR  */
    TRAIGASE = 283,                /* TRAIGASE  */
    EXTENSION_E = 284,             /* EXTENSION_E  */
    ERROR_LEXICO = 285,            /* ERROR_LEXICO  */
    FIN_LINEA = 286                /* FIN_LINEA  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef NodoAST * YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */
