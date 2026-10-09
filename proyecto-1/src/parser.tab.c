/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 5 "parser.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

int yylex(void);
extern int yylineno;
void yyerror(const char *s);

NodoAST *raiz = NULL;

/* Une dos listas de nodos hermanos. */
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

/* Cambia el valor de un nodo sin perder el valor anterior
   hasta que la nueva memoria se haya reservado correctamente. */
static void ast_cambiar_valor(NodoAST *nodo, const char *valor)
{
    char *nuevo;

    if (nodo == NULL || valor == NULL)
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

/* Crea un nodo de operación con sus operandos. */
static NodoAST *ast_operacion(const char *operador,
                              NodoAST *izquierdo,
                              NodoAST *derecho)
{
    NodoAST *nodo = ast_crear(AST_OPERACION, operador);

    ast_agregar_hijo(nodo, izquierdo);
    ast_agregar_hijo(nodo, derecho);

    return nodo;
}

/* Crea una asignación con destino y expresión. */
static NodoAST *ast_asignacion(NodoAST *destino, NodoAST *expresion)
{
    NodoAST *nodo = ast_crear(AST_ASIGNACION, "=");

    ast_agregar_hijo(nodo, destino);
    ast_agregar_hijo(nodo, expresion);

    return nodo;
}

/* Envuelve una lista de parámetros en un nodo explícito. */
static NodoAST *ast_crear_parametros(NodoAST *lista)
{
    NodoAST *nodo = ast_crear(AST_PARAMETROS, "parametros");

    ast_agregar_hijo(nodo, lista);

    return nodo;
}

#line 157 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SI = 3,                         /* SI  */
  YYSYMBOL_SINO = 4,                       /* SINO  */
  YYSYMBOL_SINOSI = 5,                     /* SINOSI  */
  YYSYMBOL_MIENTRAS = 6,                   /* MIENTRAS  */
  YYSYMBOL_PARA = 7,                       /* PARA  */
  YYSYMBOL_DEVUELVA = 8,                   /* DEVUELVA  */
  YYSYMBOL_DEFINA = 9,                     /* DEFINA  */
  YYSYMBOL_ROMPA = 10,                     /* ROMPA  */
  YYSYMBOL_PRINCIPAL = 11,                 /* PRINCIPAL  */
  YYSYMBOL_LISTA = 12,                     /* LISTA  */
  YYSYMBOL_MATRIZ = 13,                    /* MATRIZ  */
  YYSYMBOL_VOF = 14,                       /* VOF  */
  YYSYMBOL_ENTERO = 15,                    /* ENTERO  */
  YYSYMBOL_VACIO = 16,                     /* VACIO  */
  YYSYMBOL_POTENCIA = 17,                  /* POTENCIA  */
  YYSYMBOL_IGUALDAD = 18,                  /* IGUALDAD  */
  YYSYMBOL_DIFERENTE = 19,                 /* DIFERENTE  */
  YYSYMBOL_MENOR_IGUAL = 20,               /* MENOR_IGUAL  */
  YYSYMBOL_MAYOR_IGUAL = 21,               /* MAYOR_IGUAL  */
  YYSYMBOL_AND = 22,                       /* AND  */
  YYSYMBOL_OR = 23,                        /* OR  */
  YYSYMBOL_VERDADERO = 24,                 /* VERDADERO  */
  YYSYMBOL_FALSO = 25,                     /* FALSO  */
  YYSYMBOL_NUMERO = 26,                    /* NUMERO  */
  YYSYMBOL_IDENTIFICADOR = 27,             /* IDENTIFICADOR  */
  YYSYMBOL_TRAIGASE = 28,                  /* TRAIGASE  */
  YYSYMBOL_EXTENSION_E = 29,               /* EXTENSION_E  */
  YYSYMBOL_ERROR_LEXICO = 30,              /* ERROR_LEXICO  */
  YYSYMBOL_FIN_LINEA = 31,                 /* FIN_LINEA  */
  YYSYMBOL_32_ = 32,                       /* '"'  */
  YYSYMBOL_33_ = 33,                       /* '('  */
  YYSYMBOL_34_ = 34,                       /* ')'  */
  YYSYMBOL_35_ = 35,                       /* ','  */
  YYSYMBOL_36_ = 36,                       /* '['  */
  YYSYMBOL_37_ = 37,                       /* ']'  */
  YYSYMBOL_38_ = 38,                       /* '{'  */
  YYSYMBOL_39_ = 39,                       /* '}'  */
  YYSYMBOL_40_ = 40,                       /* '='  */
  YYSYMBOL_41_ = 41,                       /* '<'  */
  YYSYMBOL_42_ = 42,                       /* '>'  */
  YYSYMBOL_43_ = 43,                       /* '+'  */
  YYSYMBOL_44_ = 44,                       /* '-'  */
  YYSYMBOL_45_ = 45,                       /* '*'  */
  YYSYMBOL_46_ = 46,                       /* '/'  */
  YYSYMBOL_47_ = 47,                       /* '%'  */
  YYSYMBOL_YYACCEPT = 48,                  /* $accept  */
  YYSYMBOL_Programa = 49,                  /* Programa  */
  YYSYMBOL_Importaciones = 50,             /* Importaciones  */
  YYSYMBOL_Importacion = 51,               /* Importacion  */
  YYSYMBOL_Globales = 52,                  /* Globales  */
  YYSYMBOL_Unidades = 53,                  /* Unidades  */
  YYSYMBOL_RestoDefinicion = 54,           /* RestoDefinicion  */
  YYSYMBOL_FirmaFuncion = 55,              /* FirmaFuncion  */
  YYSYMBOL_RestoVacio = 56,                /* RestoVacio  */
  YYSYMBOL_Parametros = 57,                /* Parametros  */
  YYSYMBOL_ParametrosResto = 58,           /* ParametrosResto  */
  YYSYMBOL_Parametro = 59,                 /* Parametro  */
  YYSYMBOL_Dimension = 60,                 /* Dimension  */
  YYSYMBOL_Bloque = 61,                    /* Bloque  */
  YYSYMBOL_Sentencias = 62,                /* Sentencias  */
  YYSYMBOL_Sentencia = 63,                 /* Sentencia  */
  YYSYMBOL_Declaracion = 64,               /* Declaracion  */
  YYSYMBOL_Inicializacion = 65,            /* Inicializacion  */
  YYSYMBOL_Inicializadorlista = 66,        /* Inicializadorlista  */
  YYSYMBOL_Inicializadormatriz = 67,       /* Inicializadormatriz  */
  YYSYMBOL_Filas = 68,                     /* Filas  */
  YYSYMBOL_RestoFilas = 69,                /* RestoFilas  */
  YYSYMBOL_Fila = 70,                      /* Fila  */
  YYSYMBOL_Elementos = 71,                 /* Elementos  */
  YYSYMBOL_RestoElementos = 72,            /* RestoElementos  */
  YYSYMBOL_Asignacion = 73,                /* Asignacion  */
  YYSYMBOL_Destino = 74,                   /* Destino  */
  YYSYMBOL_Acceso = 75,                    /* Acceso  */
  YYSYMBOL_If = 76,                        /* If  */
  YYSYMBOL_Continuacionif = 77,            /* Continuacionif  */
  YYSYMBOL_While = 78,                     /* While  */
  YYSYMBOL_For = 79,                       /* For  */
  YYSYMBOL_InicioFor = 80,                 /* InicioFor  */
  YYSYMBOL_Return = 81,                    /* Return  */
  YYSYMBOL_Break = 82,                     /* Break  */
  YYSYMBOL_Llamada = 83,                   /* Llamada  */
  YYSYMBOL_Argumentos = 84,                /* Argumentos  */
  YYSYMBOL_RestoArgumentos = 85,           /* RestoArgumentos  */
  YYSYMBOL_Expresion = 86,                 /* Expresion  */
  YYSYMBOL_ExprAnd = 87,                   /* ExprAnd  */
  YYSYMBOL_ExprComp = 88,                  /* ExprComp  */
  YYSYMBOL_Comparador = 89,                /* Comparador  */
  YYSYMBOL_ExprArit = 90,                  /* ExprArit  */
  YYSYMBOL_Termino = 91,                   /* Termino  */
  YYSYMBOL_Potencia = 92,                  /* Potencia  */
  YYSYMBOL_Unario = 93,                    /* Unario  */
  YYSYMBOL_Primario = 94                   /* Primario  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   216

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  48
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  47
/* YYNRULES -- Number of rules.  */
#define YYNRULES  103
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  234

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   286


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    32,     2,     2,    47,     2,     2,
      33,    34,    45,    43,    35,    44,     2,    46,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      41,    40,    42,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    36,     2,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    38,     2,    39,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   182,   182,   196,   201,   207,   217,   222,   229,   234,
     240,   245,   250,   257,   268,   278,   287,   292,   298,   303,
     309,   314,   319,   326,   338,   342,   350,   358,   363,   370,
     374,   378,   382,   386,   390,   394,   398,   406,   412,   418,
     427,   441,   446,   452,   458,   464,   470,   477,   484,   489,
     495,   503,   508,   514,   519,   526,   533,   541,   545,   552,
     564,   575,   579,   583,   595,   606,   618,   622,   632,   641,
     649,   659,   664,   670,   675,   682,   686,   693,   697,   705,
     710,   717,   721,   725,   729,   733,   737,   745,   749,   753,
     760,   764,   768,   772,   779,   783,   790,   794,   802,   806,
     810,   814,   818,   822
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SI", "SINO", "SINOSI",
  "MIENTRAS", "PARA", "DEVUELVA", "DEFINA", "ROMPA", "PRINCIPAL", "LISTA",
  "MATRIZ", "VOF", "ENTERO", "VACIO", "POTENCIA", "IGUALDAD", "DIFERENTE",
  "MENOR_IGUAL", "MAYOR_IGUAL", "AND", "OR", "VERDADERO", "FALSO",
  "NUMERO", "IDENTIFICADOR", "TRAIGASE", "EXTENSION_E", "ERROR_LEXICO",
  "FIN_LINEA", "'\"'", "'('", "')'", "','", "'['", "']'", "'{'", "'}'",
  "'='", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'", "$accept",
  "Programa", "Importaciones", "Importacion", "Globales", "Unidades",
  "RestoDefinicion", "FirmaFuncion", "RestoVacio", "Parametros",
  "ParametrosResto", "Parametro", "Dimension", "Bloque", "Sentencias",
  "Sentencia", "Declaracion", "Inicializacion", "Inicializadorlista",
  "Inicializadormatriz", "Filas", "RestoFilas", "Fila", "Elementos",
  "RestoElementos", "Asignacion", "Destino", "Acceso", "If",
  "Continuacionif", "While", "For", "InicioFor", "Return", "Break",
  "Llamada", "Argumentos", "RestoArgumentos", "Expresion", "ExprAnd",
  "ExprComp", "Comparador", "ExprArit", "Termino", "Potencia", "Unario",
  "Primario", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-157)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      10,    13,    42,     1,    10,    50,  -157,    58,    31,    80,
       1,  -157,    40,    79,    81,    91,    79,    88,  -157,  -157,
      90,    30,    89,    87,    92,    93,    98,    98,    12,  -157,
      95,  -157,  -157,  -157,   -16,    30,    30,  -157,  -157,   104,
     107,  -157,     6,    60,  -157,   113,  -157,  -157,    54,    54,
    -157,    99,    80,    80,   100,   101,  -157,  -157,    30,    30,
      36,  -157,    30,    30,  -157,  -157,  -157,  -157,  -157,  -157,
      30,    30,    30,    30,    30,    30,    30,  -157,  -157,    94,
     102,    78,  -157,  -157,   103,    78,   108,    -4,    47,  -157,
     107,  -157,    60,    60,    68,  -157,  -157,  -157,  -157,    96,
     109,   111,    39,   110,   112,    97,   115,  -157,    30,  -157,
     114,   116,   120,    54,  -157,   119,   128,  -157,    97,    78,
    -157,    26,   125,    97,    -4,    30,    30,  -157,   106,   121,
     122,   129,   112,   126,   130,   131,    30,   134,   123,    26,
    -157,   135,   127,  -157,  -157,  -157,  -157,  -157,  -157,   137,
    -157,   138,  -157,    51,   124,    37,   132,    54,    54,  -157,
    -157,    30,    30,    46,   -10,  -157,  -157,  -157,  -157,    30,
    -157,    80,  -157,  -157,    30,  -157,   139,   140,   133,   136,
      44,    45,   147,   141,  -157,   143,  -157,   104,  -157,    37,
     144,  -157,  -157,   145,    97,    97,   142,    30,  -157,    30,
     146,   150,    54,     4,   148,    30,    41,   149,  -157,   144,
    -157,   151,    97,   154,  -157,  -157,  -157,   104,   162,  -157,
     150,  -157,   159,    30,   157,  -157,  -157,    52,    97,    97,
     161,     4,  -157,  -157
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       4,     0,     0,     7,     4,     0,     1,     0,     0,     9,
       7,     3,     0,    42,     0,     0,    42,     0,     2,     6,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       0,    99,   100,    98,    57,     0,     0,   101,   102,    41,
      76,    78,    80,    89,    93,    95,    97,    38,     0,     0,
      37,     0,     9,     9,     0,     0,    12,     5,    72,     0,
       0,    96,     0,     0,    81,    82,    83,    84,    85,    86,
       0,     0,     0,     0,     0,     0,     0,    24,    25,     0,
       0,    17,    11,    10,     0,    17,     0,    74,     0,   103,
      75,    77,    87,    88,    79,    90,    91,    92,    94,    44,
       0,     0,     0,     0,    19,     0,     0,    70,     0,    71,
      58,     0,     0,     0,    21,     0,     0,    20,     0,     0,
      16,    28,     0,     0,    74,     0,    52,    39,     0,     0,
       0,     0,    19,     0,     0,     0,     0,     0,     0,    28,
      29,     0,     0,    56,    32,    33,    34,    35,    36,     0,
      15,     0,    73,     0,     0,    54,    46,     0,     0,    13,
      18,     0,     0,     0,     0,    69,    26,    27,    30,     0,
      31,     9,    59,    43,     0,    51,     0,     0,     0,     0,
       0,     0,     0,    57,    66,     0,    68,    55,    14,    54,
       0,    40,    22,     0,     0,     0,     0,     0,    53,    52,
       0,    49,     0,     0,     0,     0,     0,     0,    45,     0,
      47,     0,     0,     0,    61,    60,    64,    67,     0,    50,
      49,    23,     0,     0,     0,    48,    62,     0,     0,     0,
       0,     0,    65,    63
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -157,  -157,   189,  -157,   184,   -50,  -157,   168,  -157,   118,
      64,    82,   -48,  -112,    59,  -157,  -111,   181,  -157,  -157,
    -157,   -20,    -5,     0,    16,  -156,  -157,  -117,  -157,   -25,
    -157,  -157,  -157,  -157,  -157,  -109,  -157,    83,   -21,   152,
     153,  -157,   -54,    43,    23,   172,  -157
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     2,     3,     4,     9,    18,    29,    52,    56,   103,
     120,   104,    79,   122,   138,   139,    10,    22,   112,   177,
     200,   210,   201,   154,   175,   141,   142,    37,   144,   215,
     145,   146,   185,   147,   148,    38,    86,   109,   155,    40,
      41,    72,    42,    43,    44,    45,    46
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      39,    80,    82,    83,   143,    88,   131,   184,   212,   213,
     140,   151,   149,    62,    60,     7,     8,    58,    94,    62,
      59,   186,   143,    54,    64,    65,    66,    67,   140,   133,
     149,   108,   134,   135,   136,   214,   137,    87,     1,    55,
       7,     8,     6,    14,    15,     5,   143,    68,    69,    70,
      71,   115,   116,    34,    31,    32,    33,    34,    16,    62,
      62,   182,   224,    35,    62,   128,   117,    62,    62,    20,
      89,   153,   174,   183,    36,    62,   218,    12,   194,   195,
      77,    78,   203,   204,   110,    13,   229,   124,   172,    17,
      70,    71,   101,   102,    70,    71,    95,    96,    97,    98,
     222,   143,    26,    27,    28,    73,    74,    75,    23,   178,
     179,    70,    71,    92,    93,   164,   230,   231,    24,    21,
      47,   188,    30,    48,    50,    51,    57,    62,    49,    63,
      76,    99,    81,    84,    85,   121,   111,   105,   114,   100,
     180,   181,   107,   156,   118,   113,   129,   119,   187,   123,
     125,   127,   126,   189,   211,   130,   150,   157,   158,   161,
     159,   173,   166,   162,   163,   165,   168,   169,   170,   171,
     192,   191,   176,   193,   196,   190,   206,    59,   197,   216,
     199,   202,   205,   208,   217,   209,   219,   223,   221,   183,
     226,   228,   232,    11,    19,    53,   160,    25,   167,   207,
     225,   132,   227,   106,   220,   198,   233,   152,    61,     0,
       0,     0,     0,     0,    90,     0,    91
};

static const yytype_int16 yycheck[] =
{
      21,    49,    52,    53,   121,    59,   118,   163,     4,     5,
     121,   123,   121,    23,    35,    14,    15,    33,    72,    23,
      36,    31,   139,    11,    18,    19,    20,    21,   139,     3,
     139,    35,     6,     7,     8,    31,    10,    58,    28,    27,
      14,    15,     0,    12,    13,    32,   163,    41,    42,    43,
      44,    12,    13,    27,    24,    25,    26,    27,    27,    23,
      23,    15,   218,    33,    23,   113,    27,    23,    23,    29,
      34,   125,    35,    27,    44,    23,    35,    27,    34,    34,
      26,    27,   194,   195,    37,    27,    34,   108,    37,     9,
      43,    44,    14,    15,    43,    44,    73,    74,    75,    76,
     212,   218,    14,    15,    16,    45,    46,    47,    27,   157,
     158,    43,    44,    70,    71,   136,   228,   229,    27,    40,
      31,   171,    32,    36,    31,    27,    31,    23,    36,    22,
      17,    37,    33,    33,    33,    38,    40,    34,    27,    37,
     161,   162,    34,    37,    34,    36,    27,    35,   169,    34,
      36,    31,    36,   174,   202,    27,    31,    36,    36,    33,
      31,    37,    39,    33,    33,    31,    31,    40,    31,    31,
      37,    31,    40,    37,    27,    36,   197,    36,    35,    31,
      36,    36,    40,    37,   205,    35,    37,    33,    37,    27,
      31,    34,    31,     4,    10,    27,   132,    16,   139,   199,
     220,   119,   223,    85,   209,   189,   231,   124,    36,    -1,
      -1,    -1,    -1,    -1,    62,    -1,    63
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    28,    49,    50,    51,    32,     0,    14,    15,    52,
      64,    50,    27,    27,    12,    13,    27,     9,    53,    52,
      29,    40,    65,    27,    27,    65,    14,    15,    16,    54,
      32,    24,    25,    26,    27,    33,    44,    75,    83,    86,
      87,    88,    90,    91,    92,    93,    94,    31,    36,    36,
      31,    27,    55,    55,    11,    27,    56,    31,    33,    36,
      86,    93,    23,    22,    18,    19,    20,    21,    41,    42,
      43,    44,    89,    45,    46,    47,    17,    26,    27,    60,
      60,    33,    53,    53,    33,    33,    84,    86,    90,    34,
      87,    88,    91,    91,    90,    92,    92,    92,    92,    37,
      37,    14,    15,    57,    59,    34,    57,    34,    35,    85,
      37,    40,    66,    36,    27,    12,    13,    27,    34,    35,
      58,    38,    61,    34,    86,    36,    36,    31,    60,    27,
      27,    61,    59,     3,     6,     7,     8,    10,    62,    63,
      64,    73,    74,    75,    76,    78,    79,    81,    82,    83,
      31,    61,    85,    90,    71,    86,    37,    36,    36,    31,
      58,    33,    33,    33,    86,    31,    39,    62,    31,    40,
      31,    31,    37,    37,    35,    72,    40,    67,    60,    60,
      86,    86,    15,    27,    73,    80,    31,    86,    53,    86,
      36,    31,    37,    37,    34,    34,    27,    35,    72,    36,
      68,    70,    36,    61,    61,    40,    86,    71,    37,    35,
      69,    60,     4,     5,    31,    77,    31,    86,    35,    37,
      70,    37,    61,    33,    73,    69,    31,    86,    34,    34,
      61,    61,    31,    77
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    48,    49,    50,    50,    51,    52,    52,    53,    53,
      54,    54,    54,    55,    56,    56,    57,    57,    58,    58,
      59,    59,    59,    59,    60,    60,    61,    62,    62,    63,
      63,    63,    63,    63,    63,    63,    63,    64,    64,    64,
      64,    65,    65,    66,    66,    67,    67,    68,    69,    69,
      70,    71,    71,    72,    72,    73,    74,    75,    75,    75,
      76,    77,    77,    77,    78,    79,    80,    80,    81,    82,
      83,    84,    84,    85,    85,    86,    86,    87,    87,    88,
      88,    89,    89,    89,    89,    89,    89,    90,    90,    90,
      91,    91,    91,    91,    92,    92,    93,    93,    94,    94,
      94,    94,    94,    94
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     2,     0,     6,     2,     0,     2,     0,
       3,     3,     2,     6,     7,     5,     2,     0,     3,     0,
       2,     2,     6,     9,     1,     1,     3,     2,     0,     1,
       2,     2,     1,     1,     1,     1,     1,     4,     4,     8,
      11,     2,     0,     4,     0,     4,     0,     2,     3,     0,
       3,     2,     0,     3,     0,     3,     1,     1,     4,     7,
       6,     1,     3,     6,     6,    10,     1,     4,     3,     2,
       4,     2,     0,     3,     0,     3,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     1,
       3,     3,     3,     1,     3,     1,     2,     1,     1,     1,
       1,     1,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_VERDADERO: /* VERDADERO  */
#line 121 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1398 "parser.tab.c"
        break;

    case YYSYMBOL_FALSO: /* FALSO  */
#line 121 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1404 "parser.tab.c"
        break;

    case YYSYMBOL_NUMERO: /* NUMERO  */
#line 121 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1410 "parser.tab.c"
        break;

    case YYSYMBOL_IDENTIFICADOR: /* IDENTIFICADOR  */
#line 121 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1416 "parser.tab.c"
        break;

    case YYSYMBOL_Importaciones: /* Importaciones  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1422 "parser.tab.c"
        break;

    case YYSYMBOL_Importacion: /* Importacion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1428 "parser.tab.c"
        break;

    case YYSYMBOL_Globales: /* Globales  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1434 "parser.tab.c"
        break;

    case YYSYMBOL_Unidades: /* Unidades  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1440 "parser.tab.c"
        break;

    case YYSYMBOL_RestoDefinicion: /* RestoDefinicion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1446 "parser.tab.c"
        break;

    case YYSYMBOL_FirmaFuncion: /* FirmaFuncion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1452 "parser.tab.c"
        break;

    case YYSYMBOL_RestoVacio: /* RestoVacio  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1458 "parser.tab.c"
        break;

    case YYSYMBOL_Parametros: /* Parametros  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1464 "parser.tab.c"
        break;

    case YYSYMBOL_ParametrosResto: /* ParametrosResto  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1470 "parser.tab.c"
        break;

    case YYSYMBOL_Parametro: /* Parametro  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1476 "parser.tab.c"
        break;

    case YYSYMBOL_Dimension: /* Dimension  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1482 "parser.tab.c"
        break;

    case YYSYMBOL_Bloque: /* Bloque  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1488 "parser.tab.c"
        break;

    case YYSYMBOL_Sentencias: /* Sentencias  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1494 "parser.tab.c"
        break;

    case YYSYMBOL_Sentencia: /* Sentencia  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1500 "parser.tab.c"
        break;

    case YYSYMBOL_Declaracion: /* Declaracion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1506 "parser.tab.c"
        break;

    case YYSYMBOL_Inicializacion: /* Inicializacion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1512 "parser.tab.c"
        break;

    case YYSYMBOL_Inicializadorlista: /* Inicializadorlista  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1518 "parser.tab.c"
        break;

    case YYSYMBOL_Inicializadormatriz: /* Inicializadormatriz  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1524 "parser.tab.c"
        break;

    case YYSYMBOL_Filas: /* Filas  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1530 "parser.tab.c"
        break;

    case YYSYMBOL_RestoFilas: /* RestoFilas  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1536 "parser.tab.c"
        break;

    case YYSYMBOL_Fila: /* Fila  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1542 "parser.tab.c"
        break;

    case YYSYMBOL_Elementos: /* Elementos  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1548 "parser.tab.c"
        break;

    case YYSYMBOL_RestoElementos: /* RestoElementos  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1554 "parser.tab.c"
        break;

    case YYSYMBOL_Asignacion: /* Asignacion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1560 "parser.tab.c"
        break;

    case YYSYMBOL_Destino: /* Destino  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1566 "parser.tab.c"
        break;

    case YYSYMBOL_Acceso: /* Acceso  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1572 "parser.tab.c"
        break;

    case YYSYMBOL_If: /* If  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1578 "parser.tab.c"
        break;

    case YYSYMBOL_Continuacionif: /* Continuacionif  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1584 "parser.tab.c"
        break;

    case YYSYMBOL_While: /* While  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1590 "parser.tab.c"
        break;

    case YYSYMBOL_For: /* For  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1596 "parser.tab.c"
        break;

    case YYSYMBOL_InicioFor: /* InicioFor  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1602 "parser.tab.c"
        break;

    case YYSYMBOL_Return: /* Return  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1608 "parser.tab.c"
        break;

    case YYSYMBOL_Break: /* Break  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1614 "parser.tab.c"
        break;

    case YYSYMBOL_Llamada: /* Llamada  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1620 "parser.tab.c"
        break;

    case YYSYMBOL_Argumentos: /* Argumentos  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1626 "parser.tab.c"
        break;

    case YYSYMBOL_RestoArgumentos: /* RestoArgumentos  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1632 "parser.tab.c"
        break;

    case YYSYMBOL_Expresion: /* Expresion  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1638 "parser.tab.c"
        break;

    case YYSYMBOL_ExprAnd: /* ExprAnd  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1644 "parser.tab.c"
        break;

    case YYSYMBOL_ExprComp: /* ExprComp  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1650 "parser.tab.c"
        break;

    case YYSYMBOL_Comparador: /* Comparador  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1656 "parser.tab.c"
        break;

    case YYSYMBOL_ExprArit: /* ExprArit  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1662 "parser.tab.c"
        break;

    case YYSYMBOL_Termino: /* Termino  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1668 "parser.tab.c"
        break;

    case YYSYMBOL_Potencia: /* Potencia  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1674 "parser.tab.c"
        break;

    case YYSYMBOL_Unario: /* Unario  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1680 "parser.tab.c"
        break;

    case YYSYMBOL_Primario: /* Primario  */
#line 131 "parser.y"
            { ast_liberar((*yyvaluep)); }
#line 1686 "parser.tab.c"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* Programa: Importaciones Globales Unidades  */
#line 183 "parser.y"
      {
          raiz = ast_crear(AST_PROGRAMA, "programa");

          ast_agregar_hijo(raiz, yyvsp[-2]);
          ast_agregar_hijo(raiz, yyvsp[-1]);
          ast_agregar_hijo(raiz, yyvsp[0]);

          yyval = raiz;
      }
#line 1967 "parser.tab.c"
    break;

  case 3: /* Importaciones: Importacion Importaciones  */
#line 197 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 1975 "parser.tab.c"
    break;

  case 4: /* Importaciones: %empty  */
#line 201 "parser.y"
      {
          yyval = NULL;
      }
#line 1983 "parser.tab.c"
    break;

  case 5: /* Importacion: TRAIGASE '"' IDENTIFICADOR EXTENSION_E '"' FIN_LINEA  */
#line 208 "parser.y"
      {
          yyval = ast_crear(AST_IMPORTACION, yyvsp[-3]->valor);
          yyval->linea = yyvsp[-3]->linea;
          ast_liberar(yyvsp[-3]);
      }
#line 1993 "parser.tab.c"
    break;

  case 6: /* Globales: Declaracion Globales  */
#line 218 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2001 "parser.tab.c"
    break;

  case 7: /* Globales: %empty  */
#line 222 "parser.y"
      {
          yyval = NULL;
      }
#line 2009 "parser.tab.c"
    break;

  case 8: /* Unidades: DEFINA RestoDefinicion  */
#line 230 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2017 "parser.tab.c"
    break;

  case 9: /* Unidades: %empty  */
#line 234 "parser.y"
      {
          yyval = NULL;
      }
#line 2025 "parser.tab.c"
    break;

  case 10: /* RestoDefinicion: ENTERO FirmaFuncion Unidades  */
#line 241 "parser.y"
      {
          ast_cambiar_valor(yyvsp[-1], "entero");
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2034 "parser.tab.c"
    break;

  case 11: /* RestoDefinicion: VOF FirmaFuncion Unidades  */
#line 246 "parser.y"
      {
          ast_cambiar_valor(yyvsp[-1], "vof");
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2043 "parser.tab.c"
    break;

  case 12: /* RestoDefinicion: VACIO RestoVacio  */
#line 251 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2051 "parser.tab.c"
    break;

  case 13: /* FirmaFuncion: IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA  */
#line 258 "parser.y"
      {
          yyval = ast_crear(AST_FUNCION, "entero");

          ast_agregar_hijo(yyval, yyvsp[-5]);
          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2063 "parser.tab.c"
    break;

  case 14: /* RestoVacio: IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA Unidades  */
#line 269 "parser.y"
      {
          NodoAST *funcion = ast_crear(AST_FUNCION, "vacio");

          ast_agregar_hijo(funcion, yyvsp[-6]);
          ast_agregar_hijo(funcion, yyvsp[-4]);
          ast_agregar_hijo(funcion, yyvsp[-2]);

          yyval = ast_unir(funcion, yyvsp[0]);
      }
#line 2077 "parser.tab.c"
    break;

  case 15: /* RestoVacio: PRINCIPAL '(' ')' Bloque FIN_LINEA  */
#line 279 "parser.y"
      {
          yyval = ast_crear(AST_PRINCIPAL, "principal");
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2086 "parser.tab.c"
    break;

  case 16: /* Parametros: Parametro ParametrosResto  */
#line 288 "parser.y"
      {
          yyval = ast_crear_parametros(ast_unir(yyvsp[-1], yyvsp[0]));
      }
#line 2094 "parser.tab.c"
    break;

  case 17: /* Parametros: %empty  */
#line 292 "parser.y"
      {
          yyval = ast_crear_parametros(NULL);
      }
#line 2102 "parser.tab.c"
    break;

  case 18: /* ParametrosResto: ',' Parametro ParametrosResto  */
#line 299 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2110 "parser.tab.c"
    break;

  case 19: /* ParametrosResto: %empty  */
#line 303 "parser.y"
      {
          yyval = NULL;
      }
#line 2118 "parser.tab.c"
    break;

  case 20: /* Parametro: ENTERO IDENTIFICADOR  */
#line 310 "parser.y"
      {
          yyval = ast_crear(AST_PARAMETRO, "entero");
          ast_agregar_hijo(yyval, yyvsp[0]);
      }
#line 2127 "parser.tab.c"
    break;

  case 21: /* Parametro: VOF IDENTIFICADOR  */
#line 315 "parser.y"
      {
          yyval = ast_crear(AST_PARAMETRO, "vof");
          ast_agregar_hijo(yyval, yyvsp[0]);
      }
#line 2136 "parser.tab.c"
    break;

  case 22: /* Parametro: ENTERO LISTA IDENTIFICADOR '[' Dimension ']'  */
#line 320 "parser.y"
      {
          yyval = ast_crear(AST_PARAMETRO, "entero lista");

          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2147 "parser.tab.c"
    break;

  case 23: /* Parametro: ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']'  */
#line 327 "parser.y"
      {
          yyval = ast_crear(AST_PARAMETRO, "entero matriz");

          ast_agregar_hijo(yyval, yyvsp[-6]);
          ast_agregar_hijo(yyval, yyvsp[-4]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2159 "parser.tab.c"
    break;

  case 24: /* Dimension: NUMERO  */
#line 339 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2167 "parser.tab.c"
    break;

  case 25: /* Dimension: IDENTIFICADOR  */
#line 343 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2175 "parser.tab.c"
    break;

  case 26: /* Bloque: '{' Sentencias '}'  */
#line 351 "parser.y"
      {
          yyval = ast_crear(AST_BLOQUE, "bloque");
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2184 "parser.tab.c"
    break;

  case 27: /* Sentencias: Sentencia Sentencias  */
#line 359 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2192 "parser.tab.c"
    break;

  case 28: /* Sentencias: %empty  */
#line 363 "parser.y"
      {
          yyval = NULL;
      }
#line 2200 "parser.tab.c"
    break;

  case 29: /* Sentencia: Declaracion  */
#line 371 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2208 "parser.tab.c"
    break;

  case 30: /* Sentencia: Asignacion FIN_LINEA  */
#line 375 "parser.y"
      {
          yyval = yyvsp[-1];
      }
#line 2216 "parser.tab.c"
    break;

  case 31: /* Sentencia: Llamada FIN_LINEA  */
#line 379 "parser.y"
      {
          yyval = yyvsp[-1];
      }
#line 2224 "parser.tab.c"
    break;

  case 32: /* Sentencia: If  */
#line 383 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2232 "parser.tab.c"
    break;

  case 33: /* Sentencia: While  */
#line 387 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2240 "parser.tab.c"
    break;

  case 34: /* Sentencia: For  */
#line 391 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2248 "parser.tab.c"
    break;

  case 35: /* Sentencia: Return  */
#line 395 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2256 "parser.tab.c"
    break;

  case 36: /* Sentencia: Break  */
#line 399 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2264 "parser.tab.c"
    break;

  case 37: /* Declaracion: ENTERO IDENTIFICADOR Inicializacion FIN_LINEA  */
#line 407 "parser.y"
    {
        yyval = ast_crear(AST_DECLARACION, "entero");
        ast_agregar_hijo(yyval, yyvsp[-2]);
        ast_agregar_hijo(yyval, yyvsp[-1]);
    }
#line 2274 "parser.tab.c"
    break;

  case 38: /* Declaracion: VOF IDENTIFICADOR Inicializacion FIN_LINEA  */
#line 413 "parser.y"
    {
        yyval = ast_crear(AST_DECLARACION, "vof");
        ast_agregar_hijo(yyval, yyvsp[-2]);
        ast_agregar_hijo(yyval, yyvsp[-1]);
    }
#line 2284 "parser.tab.c"
    break;

  case 39: /* Declaracion: ENTERO LISTA IDENTIFICADOR '[' Dimension ']' Inicializadorlista FIN_LINEA  */
#line 420 "parser.y"
      {
          yyval = ast_crear(AST_DECL_LISTA, "entero");

          ast_agregar_hijo(yyval, yyvsp[-5]);
          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2296 "parser.tab.c"
    break;

  case 40: /* Declaracion: ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']' Inicializadormatriz FIN_LINEA  */
#line 429 "parser.y"
      {
          yyval = ast_crear(AST_DECL_MATRIZ, "entero");

          ast_agregar_hijo(yyval, yyvsp[-8]);
          ast_agregar_hijo(yyval, yyvsp[-6]);
          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2309 "parser.tab.c"
    break;

  case 41: /* Inicializacion: '=' Expresion  */
#line 442 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2317 "parser.tab.c"
    break;

  case 42: /* Inicializacion: %empty  */
#line 446 "parser.y"
      {
          yyval = NULL;
      }
#line 2325 "parser.tab.c"
    break;

  case 43: /* Inicializadorlista: '=' '[' Elementos ']'  */
#line 453 "parser.y"
      {
          yyval = ast_crear(AST_LISTA, "literal");
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2334 "parser.tab.c"
    break;

  case 44: /* Inicializadorlista: %empty  */
#line 458 "parser.y"
      {
          yyval = NULL;
      }
#line 2342 "parser.tab.c"
    break;

  case 45: /* Inicializadormatriz: '=' '[' Filas ']'  */
#line 465 "parser.y"
      {
          yyval = ast_crear(AST_MATRIZ, "literal");
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2351 "parser.tab.c"
    break;

  case 46: /* Inicializadormatriz: %empty  */
#line 470 "parser.y"
      {
          yyval = NULL;
      }
#line 2359 "parser.tab.c"
    break;

  case 47: /* Filas: Fila RestoFilas  */
#line 478 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2367 "parser.tab.c"
    break;

  case 48: /* RestoFilas: ',' Fila RestoFilas  */
#line 485 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2375 "parser.tab.c"
    break;

  case 49: /* RestoFilas: %empty  */
#line 489 "parser.y"
      {
          yyval = NULL;
      }
#line 2383 "parser.tab.c"
    break;

  case 50: /* Fila: '[' Elementos ']'  */
#line 496 "parser.y"
      {
          yyval = ast_crear(AST_LISTA, "fila");
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2392 "parser.tab.c"
    break;

  case 51: /* Elementos: Expresion RestoElementos  */
#line 504 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2400 "parser.tab.c"
    break;

  case 52: /* Elementos: %empty  */
#line 508 "parser.y"
      {
          yyval = NULL;
      }
#line 2408 "parser.tab.c"
    break;

  case 53: /* RestoElementos: ',' Expresion RestoElementos  */
#line 515 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2416 "parser.tab.c"
    break;

  case 54: /* RestoElementos: %empty  */
#line 519 "parser.y"
      {
          yyval = NULL;
      }
#line 2424 "parser.tab.c"
    break;

  case 55: /* Asignacion: Destino '=' Expresion  */
#line 527 "parser.y"
      {
          yyval = ast_asignacion(yyvsp[-2], yyvsp[0]);
      }
#line 2432 "parser.tab.c"
    break;

  case 56: /* Destino: Acceso  */
#line 534 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2440 "parser.tab.c"
    break;

  case 57: /* Acceso: IDENTIFICADOR  */
#line 542 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2448 "parser.tab.c"
    break;

  case 58: /* Acceso: IDENTIFICADOR '[' ExprArit ']'  */
#line 546 "parser.y"
      {
          yyval = ast_crear(AST_ACCESO_LISTA, NULL);

          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2459 "parser.tab.c"
    break;

  case 59: /* Acceso: IDENTIFICADOR '[' ExprArit ']' '[' ExprArit ']'  */
#line 553 "parser.y"
      {
          yyval = ast_crear(AST_ACCESO_MATRIZ, NULL);

          ast_agregar_hijo(yyval, yyvsp[-6]);
          ast_agregar_hijo(yyval, yyvsp[-4]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2471 "parser.tab.c"
    break;

  case 60: /* If: SI '(' Expresion ')' Bloque Continuacionif  */
#line 565 "parser.y"
      {
          yyval = ast_crear(AST_IF, "si");

          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
          ast_agregar_hijo(yyval, yyvsp[0]);
      }
#line 2483 "parser.tab.c"
    break;

  case 61: /* Continuacionif: FIN_LINEA  */
#line 576 "parser.y"
      {
          yyval = NULL;
      }
#line 2491 "parser.tab.c"
    break;

  case 62: /* Continuacionif: SINO Bloque FIN_LINEA  */
#line 580 "parser.y"
      {
        yyval = yyvsp[-1];
      }
#line 2499 "parser.tab.c"
    break;

  case 63: /* Continuacionif: SINOSI '(' Expresion ')' Bloque Continuacionif  */
#line 584 "parser.y"
      {
          yyval = ast_crear(AST_IF, "sinosi");

          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
          ast_agregar_hijo(yyval, yyvsp[0]);
      }
#line 2511 "parser.tab.c"
    break;

  case 64: /* While: MIENTRAS '(' Expresion ')' Bloque FIN_LINEA  */
#line 596 "parser.y"
      {
          yyval = ast_crear(AST_WHILE, "mientras");

          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2522 "parser.tab.c"
    break;

  case 65: /* For: PARA '(' InicioFor ',' Expresion ',' Asignacion ')' Bloque FIN_LINEA  */
#line 607 "parser.y"
      {
          yyval = ast_crear(AST_FOR, "para");

          ast_agregar_hijo(yyval, yyvsp[-7]);
          ast_agregar_hijo(yyval, yyvsp[-5]);
          ast_agregar_hijo(yyval, yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2535 "parser.tab.c"
    break;

  case 66: /* InicioFor: Asignacion  */
#line 619 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2543 "parser.tab.c"
    break;

  case 67: /* InicioFor: ENTERO IDENTIFICADOR '=' Expresion  */
#line 623 "parser.y"
     {
         yyval = ast_crear(AST_DECLARACION, "entero");
         ast_agregar_hijo(yyval, yyvsp[-2]);
         ast_agregar_hijo(yyval, yyvsp[0]);
     }
#line 2553 "parser.tab.c"
    break;

  case 68: /* Return: DEVUELVA Expresion FIN_LINEA  */
#line 633 "parser.y"
      {
          yyval = ast_crear(AST_RETURN, "devuelva");
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2562 "parser.tab.c"
    break;

  case 69: /* Break: ROMPA FIN_LINEA  */
#line 642 "parser.y"
      {
          yyval = ast_crear(AST_BREAK, "rompa");
      }
#line 2570 "parser.tab.c"
    break;

  case 70: /* Llamada: IDENTIFICADOR '(' Argumentos ')'  */
#line 650 "parser.y"
      {
          yyval = ast_crear(AST_LLAMADA, yyvsp[-3]->valor);
          yyval->linea = yyvsp[-3]->linea;
          ast_liberar(yyvsp[-3]);
          ast_agregar_hijo(yyval, yyvsp[-1]);
      }
#line 2581 "parser.tab.c"
    break;

  case 71: /* Argumentos: Expresion RestoArgumentos  */
#line 660 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2589 "parser.tab.c"
    break;

  case 72: /* Argumentos: %empty  */
#line 664 "parser.y"
      {
          yyval = NULL;
      }
#line 2597 "parser.tab.c"
    break;

  case 73: /* RestoArgumentos: ',' Expresion RestoArgumentos  */
#line 671 "parser.y"
      {
          yyval = ast_unir(yyvsp[-1], yyvsp[0]);
      }
#line 2605 "parser.tab.c"
    break;

  case 74: /* RestoArgumentos: %empty  */
#line 675 "parser.y"
      {
          yyval = NULL;
      }
#line 2613 "parser.tab.c"
    break;

  case 75: /* Expresion: Expresion OR ExprAnd  */
#line 683 "parser.y"
      {
          yyval = ast_operacion("OR", yyvsp[-2], yyvsp[0]);
      }
#line 2621 "parser.tab.c"
    break;

  case 76: /* Expresion: ExprAnd  */
#line 687 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2629 "parser.tab.c"
    break;

  case 77: /* ExprAnd: ExprAnd AND ExprComp  */
#line 694 "parser.y"
      {
          yyval = ast_operacion("AND", yyvsp[-2], yyvsp[0]);
      }
#line 2637 "parser.tab.c"
    break;

  case 78: /* ExprAnd: ExprComp  */
#line 698 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2645 "parser.tab.c"
    break;

  case 79: /* ExprComp: ExprArit Comparador ExprArit  */
#line 706 "parser.y"
      {
          yyval = ast_operacion(yyvsp[-1]->valor, yyvsp[-2], yyvsp[0]);
          ast_liberar(yyvsp[-1]);
      }
#line 2654 "parser.tab.c"
    break;

  case 80: /* ExprComp: ExprArit  */
#line 711 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2662 "parser.tab.c"
    break;

  case 81: /* Comparador: IGUALDAD  */
#line 718 "parser.y"
      {
          yyval = ast_crear(AST_OPERACION, "==");
      }
#line 2670 "parser.tab.c"
    break;

  case 82: /* Comparador: DIFERENTE  */
#line 722 "parser.y"
      {
          yyval = ast_crear(AST_OPERACION, "!=");
      }
#line 2678 "parser.tab.c"
    break;

  case 83: /* Comparador: MENOR_IGUAL  */
#line 726 "parser.y"
      {
          yyval = ast_crear(AST_OPERACION, "<=");
      }
#line 2686 "parser.tab.c"
    break;

  case 84: /* Comparador: MAYOR_IGUAL  */
#line 730 "parser.y"
      {
          yyval = ast_crear(AST_OPERACION, ">=");
      }
#line 2694 "parser.tab.c"
    break;

  case 85: /* Comparador: '<'  */
#line 734 "parser.y"
      {
          yyval = ast_crear(AST_OPERACION, "<");
      }
#line 2702 "parser.tab.c"
    break;

  case 86: /* Comparador: '>'  */
#line 738 "parser.y"
      {
          yyval = ast_crear(AST_OPERACION, ">");
      }
#line 2710 "parser.tab.c"
    break;

  case 87: /* ExprArit: ExprArit '+' Termino  */
#line 746 "parser.y"
      {
          yyval = ast_operacion("+", yyvsp[-2], yyvsp[0]);
      }
#line 2718 "parser.tab.c"
    break;

  case 88: /* ExprArit: ExprArit '-' Termino  */
#line 750 "parser.y"
      {
          yyval = ast_operacion("-", yyvsp[-2], yyvsp[0]);
      }
#line 2726 "parser.tab.c"
    break;

  case 89: /* ExprArit: Termino  */
#line 754 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2734 "parser.tab.c"
    break;

  case 90: /* Termino: Termino '*' Potencia  */
#line 761 "parser.y"
      {
          yyval = ast_operacion("*", yyvsp[-2], yyvsp[0]);
      }
#line 2742 "parser.tab.c"
    break;

  case 91: /* Termino: Termino '/' Potencia  */
#line 765 "parser.y"
      {
          yyval = ast_operacion("/", yyvsp[-2], yyvsp[0]);
      }
#line 2750 "parser.tab.c"
    break;

  case 92: /* Termino: Termino '%' Potencia  */
#line 769 "parser.y"
      {
          yyval = ast_operacion("%", yyvsp[-2], yyvsp[0]);
      }
#line 2758 "parser.tab.c"
    break;

  case 93: /* Termino: Potencia  */
#line 773 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2766 "parser.tab.c"
    break;

  case 94: /* Potencia: Unario POTENCIA Potencia  */
#line 780 "parser.y"
      {
          yyval = ast_operacion("**", yyvsp[-2], yyvsp[0]);
      }
#line 2774 "parser.tab.c"
    break;

  case 95: /* Potencia: Unario  */
#line 784 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2782 "parser.tab.c"
    break;

  case 96: /* Unario: '-' Unario  */
#line 791 "parser.y"
      {
          yyval = ast_operacion("unario -", yyvsp[0], NULL);
      }
#line 2790 "parser.tab.c"
    break;

  case 97: /* Unario: Primario  */
#line 795 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2798 "parser.tab.c"
    break;

  case 98: /* Primario: NUMERO  */
#line 803 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2806 "parser.tab.c"
    break;

  case 99: /* Primario: VERDADERO  */
#line 807 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2814 "parser.tab.c"
    break;

  case 100: /* Primario: FALSO  */
#line 811 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2822 "parser.tab.c"
    break;

  case 101: /* Primario: Acceso  */
#line 815 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2830 "parser.tab.c"
    break;

  case 102: /* Primario: Llamada  */
#line 819 "parser.y"
      {
          yyval = yyvsp[0];
      }
#line 2838 "parser.tab.c"
    break;

  case 103: /* Primario: '(' Expresion ')'  */
#line 823 "parser.y"
      {
          yyval = yyvsp[-1];
      }
#line 2846 "parser.tab.c"
    break;


#line 2850 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 828 "parser.y"


void yyerror(const char *s)
{
    fprintf(stderr,
            "Error sintactico en la linea %d: %s\n",
            yylineno,
            s);
}

int main(void)
{
    int resultado = yyparse();

    if (resultado == 0)
    {
        printf("\nPrograma sintacticamente correcto.\n");

        if (raiz != NULL)
        {
            printf("\nArbol de sintaxis abstracta:\n");
            ast_imprimir(raiz, 0);
            ast_liberar(raiz);
            raiz = NULL;
        }

        return EXIT_SUCCESS;
    }

    printf("Programa sintacticamente incorrecto.\n");

    /*
     * Si el análisis falla antes de reducir Programa,
     * Bison libera los valores semánticos descartados mediante
     * los destructores definidos arriba.
     */
    raiz = NULL;

    return EXIT_FAILURE;
}
