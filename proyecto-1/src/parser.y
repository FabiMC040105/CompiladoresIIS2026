%code requires {
#include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "tabla_simbolos.h"
#include "codegen.h"
#include "codegen_tabla.h"

int yylex(void);
extern int yylineno;
void yyerror(const char *s);

NodoAST *raiz = NULL;
TablaSimbolos *tabla = NULL;

/* Contextos de firmas: se preparan desde Flex al reconocer la cabecera.
 * Se registran como simbolos en las acciones existentes de Bison.
 * Nunca se modifica una produccion ni se recorre el AST.
 */
typedef struct ContextoFuncion {
    const NodoAST *nodo_nombre; /* NULL para principal */
    Ambito *ambito;            /* ambito de parametros de la funcion */
    TipoDato retorno;
    int linea;
    int es_principal;
    struct ContextoFuncion *siguiente;
} ContextoFuncion;

static ContextoFuncion *contextos_funciones = NULL;
static int esperando_tipo_funcion = 0;
static int esperando_nombre_funcion = 0;
static TipoDato retorno_lexico = TIPO_DESCONOCIDO;

/* Avisos del lexer para reconocer solamente la cabecera defina TIPO nombre.
 * El registro de los simbolos se realiza desde acciones de Bison.
 */
void parser_avisar_defina(void)
{
    esperando_tipo_funcion = 1;
    esperando_nombre_funcion = 0;
    retorno_lexico = TIPO_DESCONOCIDO;
}

void parser_avisar_tipo(TipoDato tipo)
{
    if (!esperando_tipo_funcion)
        return;

    retorno_lexico = tipo;
    esperando_tipo_funcion = 0;
    esperando_nombre_funcion = 1;
}

/* Se invoca cuando Flex encuentra el nombre de la funcion (o PRINCIPAL).
 * Crea el ambito de parametros antes de analizar la lista de parametros.
 */
static void parser_preparar_funcion(const NodoAST *nombre,
                                   const char *texto, int linea,
                                   int principal)
{
    ContextoFuncion *contexto;

    if (!esperando_nombre_funcion)
        return;

    if (tabla == NULL || tabla->ambito_actual != tabla->ambito_global)
    {
        fprintf(stderr, "Error interno: funcion fuera del ambito global.\n");
        exit(EXIT_FAILURE);
    }

    contexto = calloc(1, sizeof(*contexto));
    if (contexto == NULL)
    {
        fprintf(stderr, "Error interno: sin memoria para funcion.\n");
        exit(EXIT_FAILURE);
    }

    contexto->nodo_nombre = nombre;
    contexto->retorno = retorno_lexico;
    contexto->linea = linea;
    contexto->es_principal = principal;
    contexto->ambito = ts_entrar_ambito(tabla, texto, AMBITO_FUNCION);

    if (contexto->ambito == NULL)
    {
        free(contexto);
        fprintf(stderr, "Error interno: no se pudo crear ambito de funcion.\n");
        exit(EXIT_FAILURE);
    }

    contexto->siguiente = contextos_funciones;
    contextos_funciones = contexto;
    esperando_nombre_funcion = 0;
}

void parser_marcar_principal(int linea)
{
    parser_preparar_funcion(NULL, "principal", linea, 1);
}

static ContextoFuncion *parser_contexto_funcion(const NodoAST *nombre,
                                                int principal)
{
    ContextoFuncion *ctx;
    for (ctx = contextos_funciones; ctx != NULL; ctx = ctx->siguiente)
    {
        if (principal ? ctx->es_principal : (!ctx->es_principal && ctx->nodo_nombre == nombre))
            return ctx;
    }
    return NULL;
}

static void parser_liberar_contextos_funciones(void)
{
    while (contextos_funciones != NULL)
    {
        ContextoFuncion *siguiente = contextos_funciones->siguiente;
        free(contextos_funciones);
        contextos_funciones = siguiente;
    }
}

/* Contexto lexico para los ambitos, sin alterar las producciones GLC.
 * Cada IDENTIFICADOR conserva una referencia al ambito en el que
 * fue reconocido. Esto evita errores si Bison lee tokens adelantados.
 */
typedef struct MarcaIdentificador {
    const NodoAST *nodo;
    Ambito *ambito;
    struct MarcaIdentificador *siguiente;
} MarcaIdentificador;

static MarcaIdentificador *marcas_identificadores = NULL;
static int errores_semanticos = 0;

/* El lexer indica que clase de bloque se abrira con la proxima llave.
   Solo afecta la etiqueta de visualizacion; NO altera la GLC ni el AST. */
static const char *bloque_pendiente = NULL;
static int linea_bloque_pendiente = 0;

void parser_indicar_bloque(const char *tipo, int linea)
{
    bloque_pendiente = tipo;
    linea_bloque_pendiente = linea;
}

/* Invocadas desde lexer.l exclusivamente para registrar el contexto. */
void parser_abrir_ambito_lexico(int linea)
{
    char nombre[64];
    Ambito *padre;

    if (tabla == NULL || tabla->ambito_actual == NULL)
    {
        fprintf(stderr, "Error interno: no hay ambito activo.\n");
        exit(EXIT_FAILURE);
    }

    padre = tabla->ambito_actual;
    if (padre->tipo == AMBITO_FUNCION)
        snprintf(nombre, sizeof(nombre), "Cuerpo %.40s", padre->nombre);
    else if (padre->tipo == AMBITO_FOR)
        snprintf(nombre, sizeof(nombre), "Cuerpo %.40s", padre->nombre);
    else if (bloque_pendiente != NULL)
        snprintf(nombre, sizeof(nombre), "Bloque %s L%d",
                 bloque_pendiente, linea_bloque_pendiente);
    else
        snprintf(nombre, sizeof(nombre), "Bloque L%d", linea);

    bloque_pendiente = NULL;
    linea_bloque_pendiente = 0;

    if (ts_entrar_ambito(tabla, nombre, AMBITO_BLOQUE) == NULL)
    {
        fprintf(stderr, "Error interno: no se pudo abrir un ambito.\n");
        exit(EXIT_FAILURE);
    }
}

/* La cabecera para necesita su propio ambito (independiente del cuerpo). */
void parser_abrir_ambito_para_lexico(int linea)
{
    char nombre[64];
    snprintf(nombre, sizeof(nombre), "Para L%d", linea);

    if (tabla == NULL ||
        ts_entrar_ambito(tabla, nombre, AMBITO_FOR) == NULL)
    {
        fprintf(stderr, "Error interno: no se pudo abrir el ambito para.\n");
        exit(EXIT_FAILURE);
    }
}

void parser_cerrar_ambito_lexico(void)
{
    /* Una llave de cierre extra sera rechazada por Bison.
       Nunca se intenta salir del ambito global. */
    if (tabla != NULL && tabla->ambito_actual != tabla->ambito_global)
    {
        (void)ts_salir_ambito(tabla);
        /* Al cerrar el bloque exterior de una funcion o el cuerpo de
           un para, cerrar también su ámbito de cabecera asociado. */
        if (tabla->ambito_actual != tabla->ambito_global &&
            (tabla->ambito_actual->tipo == AMBITO_FUNCION ||
             tabla->ambito_actual->tipo == AMBITO_FOR))
            (void)ts_salir_ambito(tabla);
    }
}

void parser_marcar_identificador(NodoAST *nodo)
{
    MarcaIdentificador *marca;

    if (tabla == NULL || nodo == NULL)
    {
        fprintf(stderr, "Error interno: no hay contexto de identificador.\n");
        exit(EXIT_FAILURE);
    }

    marca = malloc(sizeof(*marca));
    if (marca == NULL)
    {
        fprintf(stderr, "Error interno: sin memoria para contexto de identificador.\n");
        exit(EXIT_FAILURE);
    }

    marca->nodo = nodo;
    marca->ambito = tabla->ambito_actual;
    marca->siguiente = marcas_identificadores;
    marcas_identificadores = marca;

    if (esperando_nombre_funcion)
        parser_preparar_funcion(nodo, nodo->valor, nodo->linea, 0);
}

Ambito *parser_ambito_identificador(const NodoAST *nodo)
{
    MarcaIdentificador *marca;
    for (marca = marcas_identificadores; marca != NULL; marca = marca->siguiente)
        if (marca->nodo == nodo)
            return marca->ambito;
    return NULL;
}

static void parser_liberar_marcas(void)
{
    while (marcas_identificadores != NULL)
    {
        MarcaIdentificador *siguiente = marcas_identificadores->siguiente;
        free(marcas_identificadores);
        marcas_identificadores = siguiente;
    }
}

/* Insercion al reducir Declaracion, con el ambito capturado por Flex.
 * No modifica el AST ni necesita recorrerlo.
 * Las dimensiones se guardan como texto, ya que la GLC acepta
 * tanto NUMERO como IDENTIFICADOR en Dimension.
 */
static int parser_registrar_variable(const NodoAST *identificador,
                                    TipoDato tipo,
                                    FormaSimbolo forma,
                                    const NodoAST *dimension1,
                                    const NodoAST *dimension2)
{
    Ambito *ambito_declaracion;
    Ambito *ambito_guardado;
    Simbolo *nuevo = NULL;
    ResultadoTS resultado;

    if (tabla == NULL || identificador == NULL || identificador->valor == NULL)
        return 0;

    if ((forma == FORMA_LISTA &&
         (dimension1 == NULL || dimension1->valor == NULL)) ||
        (forma == FORMA_MATRIZ &&
         (dimension1 == NULL || dimension1->valor == NULL ||
          dimension2 == NULL || dimension2->valor == NULL)))
    {
        fprintf(stderr, "Error interno: dimensiones incompletas de '%s'.\n",
                identificador->valor);
        return 0;
    }

    ambito_declaracion = parser_ambito_identificador(identificador);
    if (ambito_declaracion == NULL)
    {
        fprintf(stderr, "Error interno: no se encontro el ambito de '%s'.\n",
                identificador->valor);
        return 0;
    }

    /* ts_insertar utiliza ambito_actual: se cambia temporalmente
       para evitar que el lookahead de Bison cambie el ambito asignado. */
    ambito_guardado = tabla->ambito_actual;
    tabla->ambito_actual = ambito_declaracion;

    resultado = ts_insertar(tabla, identificador->valor,
                            SIM_VARIABLE, tipo, forma,
                            identificador->linea, &nuevo);

    if (resultado == TS_OK && forma != FORMA_ESCALAR)
    {
        resultado = ts_definir_dimensiones(
            nuevo,
            dimension1->valor,
            dimension2 != NULL ? dimension2->valor : NULL
        );
    }

    tabla->ambito_actual = ambito_guardado;

    if (resultado == TS_ERROR_DUPLICADO)
    {
        fprintf(stderr,
                "Error semantico en linea %d: variable '%s' "
                "ya declarada en el ambito %d.\n",
                identificador->linea, identificador->valor,
                ambito_declaracion->id);
        errores_semanticos++;
        return 1; /* Es un error semantico; seguimos analizando. */
    }

    if (resultado != TS_OK)
    {
        fprintf(stderr, "Error interno al registrar '%s' o sus dimensiones.\n",
                identificador->valor);
        return 0;
    }

    return 1;
}

/* Registra un parametro en el ambito de la funcion.
 * La firma se enlaza al simbolo de la funcion cuando Bison la reduce.
 */
static int parser_registrar_parametro(const NodoAST *identificador,
                                     TipoDato tipo, FormaSimbolo forma,
                                     const NodoAST *dim1, const NodoAST *dim2)
{
    Ambito *ambito, *guardado;
    Simbolo *nuevo = NULL;
    ResultadoTS resultado;

    if (tabla == NULL || identificador == NULL || identificador->valor == NULL)
        return 0;

    ambito = parser_ambito_identificador(identificador);
    if (ambito == NULL || ambito->tipo != AMBITO_FUNCION ||
        (forma == FORMA_LISTA && (dim1 == NULL || dim1->valor == NULL)) ||
        (forma == FORMA_MATRIZ && (dim1 == NULL || dim1->valor == NULL ||
                                   dim2 == NULL || dim2->valor == NULL)))
    {
        fprintf(stderr, "Error interno: contexto o dimensiones de parametro.\n");
        return 0;
    }

    guardado = tabla->ambito_actual;
    tabla->ambito_actual = ambito;
    resultado = ts_insertar(tabla, identificador->valor, SIM_PARAMETRO,
                            tipo, forma, identificador->linea, &nuevo);
    if (resultado == TS_OK && forma != FORMA_ESCALAR)
        resultado = ts_definir_dimensiones(nuevo, dim1->valor,
                                          dim2 != NULL ? dim2->valor : NULL);
    tabla->ambito_actual = guardado;

    if (resultado == TS_ERROR_DUPLICADO)
    {
        fprintf(stderr, "Error semantico en linea %d: parametro '%s' duplicado.\n",
                identificador->linea, identificador->valor);
        errores_semanticos++;
        return 1;
    }

    if (resultado != TS_OK)
    {
        fprintf(stderr, "Error interno: no se pudo registrar parametro '%s'.\n",
                identificador->valor);
        return 0;
    }
    return 1;
}

/* Registra la funcion en Global y vincula los parametros ya insertados.
 * Solo recorre la tabla de simbolos, NUNCA el AST.
 */
static int parser_registrar_funcion(const NodoAST *nombre, int principal)
{
    ContextoFuncion *ctx = parser_contexto_funcion(nombre, principal);
    Ambito *guardado;
    Simbolo *funcion = NULL;
    Simbolo *simbolo;
    ResultadoTS resultado;
    const char *nombre_funcion = principal ? "principal" :
                                  (nombre != NULL ? nombre->valor : NULL);

    if (tabla == NULL || ctx == NULL || nombre_funcion == NULL)
    {
        fprintf(stderr, "Error interno: contexto de funcion no encontrado.\n");
        return 0;
    }

    guardado = tabla->ambito_actual;
    tabla->ambito_actual = tabla->ambito_global;
    resultado = ts_insertar(tabla, nombre_funcion, SIM_FUNCION,
                            ctx->retorno, FORMA_ESCALAR, ctx->linea,
                            &funcion);
    tabla->ambito_actual = guardado;

    if (resultado == TS_ERROR_DUPLICADO)
    {
        fprintf(stderr, "Error semantico en linea %d: funcion '%s' duplicada.\n",
                ctx->linea, nombre_funcion);
        errores_semanticos++;
        return 1;
    }
    if (resultado != TS_OK)
    {
        fprintf(stderr, "Error interno al registrar funcion '%s'.\n",
                nombre_funcion);
        return 0;
    }

    funcion->ambito_funcion = ctx->ambito;

    /* La lista general guarda los parametros en orden de declaracion. */
    for (simbolo = tabla->primero; simbolo != NULL; simbolo = simbolo->siguiente)
    {
        if (simbolo->categoria == SIM_PARAMETRO && simbolo->ambito == ctx->ambito)
        {
            if (ts_agregar_parametro(funcion, simbolo) != TS_OK)
            {
                fprintf(stderr, "Error interno al construir firma de '%s'.\n",
                        nombre_funcion);
                return 0;
            }
        }
    }

    return 1;
}

static void parser_imprimir_firmas(FILE *salida)
{
    const Simbolo *simbolo;
    if (salida == NULL || tabla == NULL)
        return;

    fprintf(salida, "\nFIRMAS DE FUNCIONES\n");
    for (simbolo = tabla->primero; simbolo != NULL; simbolo = simbolo->siguiente)
    {
        const ParametroFirma *parametro;
        if (simbolo->categoria != SIM_FUNCION)
            continue;

        fprintf(salida, "%s %s(",
                simbolo->tipo == TIPO_ENTERO ? "entero" :
                simbolo->tipo == TIPO_VOF ? "vof" :
                simbolo->tipo == TIPO_VACIO ? "vacio" : "desconocido",
                simbolo->nombre);

        for (parametro = simbolo->parametros; parametro != NULL;
             parametro = parametro->siguiente)
        {
            const Simbolo *p = parametro->simbolo;
            if (parametro != simbolo->parametros)
                fprintf(salida, ", ");
            fprintf(salida, "%s ", p->tipo == TIPO_ENTERO ? "entero" :
                                    p->tipo == TIPO_VOF ? "vof" : "vacio");
            if (p->forma == FORMA_LISTA)
                fprintf(salida, "lista ");
            else if (p->forma == FORMA_MATRIZ)
                fprintf(salida, "matriz ");
            fprintf(salida, "%s", p->nombre);
            if (p->cantidad_dimensiones >= 1)
                fprintf(salida, "[%s]", p->dimensiones[0]);
            if (p->cantidad_dimensiones >= 2)
                fprintf(salida, "[%s]", p->dimensiones[1]);
        }
        fprintf(salida, ")  [ambito %d, %d parametros]\n",
                simbolo->ambito_funcion != NULL ? simbolo->ambito_funcion->id : -1,
                simbolo->cantidad_parametros);
    }
}

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

/* Identificadores y números */
%token NUMERO IDENTIFICADOR

/* Importaciones */
%token TRAIGASE EXTENSION_E

/* Otros tokens */
%token ERROR_LEXICO FIN_LINEA

/*
 * Los siguientes tokens crean nodos en lexer.l.
 * Solo se les aplica destructor a esos tokens con valores
 * semánticos propios.
 */
%destructor { ast_liberar($$); } NUMERO IDENTIFICADOR VERDADERO FALSO

/*
 * Estos no terminales pueden contener nodos AST.
 * Si Bison los descarta durante la recuperación de un error,
 * se libera el árbol que posean.
 *
 * Programa se excluye porque su valor se conserva en raiz
 * cuando el análisis termina correctamente.
 */
%destructor { ast_liberar($$); }
    Importaciones
    Importacion
    Globales
    Unidades
    RestoDefinicion
    FirmaFuncion
    RestoVacio
    Parametros
    ParametrosResto
    Parametro
    Dimension
    Bloque
    Sentencias
    Sentencia
    Declaracion
    Inicializacion
    Inicializadorlista
    Inicializadormatriz
    Filas
    RestoFilas
    Fila
    Elementos
    RestoElementos
    Asignacion
    Destino
    Acceso
    If
    Continuacionif
    While
    For
    InicioFor
    Return
    Break
    Llamada
    Argumentos
    RestoArgumentos
    Expresion
    ExprAnd
    ExprComp
    Comparador
    ExprArit
    Termino
    Potencia
    Unario
    Primario

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
          $$->linea = $3->linea;
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
          if (!parser_registrar_funcion($1, 0))
              YYABORT;

          $$ = ast_crear(AST_FUNCION, "entero");

          ast_agregar_hijo($$, $1);
          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
      }
    ;

RestoVacio
    : IDENTIFICADOR '(' Parametros ')' Bloque FIN_LINEA Unidades
      {
          if (!parser_registrar_funcion($1, 0))
              YYABORT;

          NodoAST *funcion = ast_crear(AST_FUNCION, "vacio");

          ast_agregar_hijo(funcion, $1);
          ast_agregar_hijo(funcion, $3);
          ast_agregar_hijo(funcion, $5);

          $$ = ast_unir(funcion, $7);
      }
    | PRINCIPAL '(' ')' Bloque FIN_LINEA
      {
          if (!parser_registrar_funcion(NULL, 1))
              YYABORT;

          $$ = ast_crear(AST_PRINCIPAL, "principal");
          ast_agregar_hijo($$, $4);
      }
    ;

/* Parámetros */
Parametros
    : Parametro ParametrosResto
      {
          $$ = ast_crear_parametros(ast_unir($1, $2));
      }
    |
      {
          $$ = ast_crear_parametros(NULL);
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
          if (!parser_registrar_parametro($2, TIPO_ENTERO, FORMA_ESCALAR, NULL, NULL))
              YYABORT;

          $$ = ast_crear(AST_PARAMETRO, "entero");
          ast_agregar_hijo($$, $2);
      }
    | VOF IDENTIFICADOR
      {
          if (!parser_registrar_parametro($2, TIPO_VOF, FORMA_ESCALAR, NULL, NULL))
              YYABORT;

          $$ = ast_crear(AST_PARAMETRO, "vof");
          ast_agregar_hijo($$, $2);
      }
    | ENTERO LISTA IDENTIFICADOR '[' Dimension ']'
      {
          if (!parser_registrar_parametro($3, TIPO_ENTERO, FORMA_LISTA, $5, NULL))
              YYABORT;

          $$ = ast_crear(AST_PARAMETRO, "entero lista");

          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
      }
    | ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']' '[' Dimension ']'
      {
          if (!parser_registrar_parametro($3, TIPO_ENTERO, FORMA_MATRIZ, $5, $8))
              YYABORT;

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

/* Bloques de código */
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
        if (!parser_registrar_variable($2, TIPO_ENTERO, FORMA_ESCALAR, NULL, NULL))
            YYABORT;

        $$ = ast_crear(AST_DECLARACION, "entero");
        ast_agregar_hijo($$, $2);
        ast_agregar_hijo($$, $3);
    }
    | VOF IDENTIFICADOR Inicializacion FIN_LINEA
    {
        if (!parser_registrar_variable($2, TIPO_VOF, FORMA_ESCALAR, NULL, NULL))
            YYABORT;

        $$ = ast_crear(AST_DECLARACION, "vof");
        ast_agregar_hijo($$, $2);
        ast_agregar_hijo($$, $3);
    }
    | ENTERO LISTA IDENTIFICADOR '[' Dimension ']'
      Inicializadorlista FIN_LINEA
      {
          if (!parser_registrar_variable($3, TIPO_ENTERO, FORMA_LISTA,
                                         $5, NULL))
              YYABORT;

          $$ = ast_crear(AST_DECL_LISTA, "entero");

          ast_agregar_hijo($$, $3);
          ast_agregar_hijo($$, $5);
          ast_agregar_hijo($$, $7);
      }
    | ENTERO MATRIZ IDENTIFICADOR '[' Dimension ']'
      '[' Dimension ']' Inicializadormatriz FIN_LINEA
      {
          if (!parser_registrar_variable($3, TIPO_ENTERO, FORMA_MATRIZ,
                                         $5, $8))
              YYABORT;

          $$ = ast_crear(AST_DECL_MATRIZ, "entero");

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
          $$ = ast_crear(AST_LISTA, "literal");
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
          $$ = ast_crear(AST_MATRIZ, "literal");
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
          $$ = ast_asignacion($1, $3);
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
          $$ = ast_crear(AST_ACCESO_LISTA, NULL);

          ast_agregar_hijo($$, $1);
          ast_agregar_hijo($$, $3);
      }
    | IDENTIFICADOR '[' ExprArit ']' '[' ExprArit ']'
      {
          $$ = ast_crear(AST_ACCESO_MATRIZ, NULL);

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
        $$ = $2;
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
         /* La declaración del inicializador pertenece al ámbito Para. */
         if (!parser_registrar_variable($2, TIPO_ENTERO,
                                        FORMA_ESCALAR, NULL, NULL))
             YYABORT;

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
          $$->linea = $1->linea;
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

/* Expresiones lógicas */
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

/* Operaciones aritméticas */
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

/* Valores, variables, llamadas y expresiones entre paréntesis */
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

void yyerror(const char *s)
{
    fprintf(stderr,
            "Error sintactico en la linea %d: %s\n",
            yylineno,
            s);
}

int main(void)
{
    int resultado;

    /* Paso 1: inicializar la tabla antes de iniciar Bison. */
    tabla = ts_crear();
    if (tabla == NULL)
    {
        fprintf(stderr, "Error: no se pudo crear la tabla de simbolos.\n");
        return EXIT_FAILURE;
    }

    resultado = yyparse();

    if (resultado == 0)
    {
        printf("\nPrograma sintacticamente correcto.\n");

        if (ts_asignar_direcciones_globales(tabla) != TS_OK)
        {
            fprintf(stderr,
                    "Error: no se pudieron asignar las direcciones globales.\n");

            parser_liberar_marcas();
            parser_liberar_contextos_funciones();
            ts_destruir(tabla);
            tabla = NULL;

            return EXIT_FAILURE;
        }

        if (raiz != NULL)
        {
            printf("\nArbol de sintaxis abstracta:\n");
            ast_imprimir(raiz, 0);

            /*
             * Fase 4: generar ensamblador solo si no hubo errores
             * semanticos. La tabla ya contiene direcciones globales,
             * offsets FP y tamanos de marco definitivos.
             */
            if (errores_semanticos == 0)
            {
                CodegenTablaContexto contexto_codegen;
                CodegenConfig config_codegen = {0};

                contexto_codegen.tabla = tabla;
                contexto_codegen.obtener_ambito_nodo =
                    parser_ambito_identificador;

                config_codegen.resolver_simbolo_nodo =
                    codegen_resolver_desde_tabla;
                config_codegen.contexto_simbolos = &contexto_codegen;
                config_codegen.nops_dependencia = 4;

                if (!codegen_generar_archivo(
                        raiz, "salida.asm", &config_codegen))
                {
                    fprintf(stderr,
                            "Error: fallo la generacion de ensamblador.\n");

                    ast_liberar(raiz);
                    raiz = NULL;
                    parser_liberar_marcas();
                    parser_liberar_contextos_funciones();
                    ts_destruir(tabla);
                    tabla = NULL;
                    return EXIT_FAILURE;
                }

                printf("\nEnsamblador generado: salida.asm\n");
            }

            ast_liberar(raiz);
            raiz = NULL;
        }

        /* Mostrar los simbolos, sus ambitos y las firmas de funciones. */
        ts_imprimir(tabla, stdout);
        ts_imprimir_ambitos(tabla, stdout);
        parser_imprimir_firmas(stdout);

        if (errores_semanticos > 0)
            fprintf(stderr, "\nSe encontraron %d errores semanticos.\n",
                    errores_semanticos);

        parser_liberar_marcas();
        parser_liberar_contextos_funciones();
        ts_destruir(tabla);
        tabla = NULL;
        return errores_semanticos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    printf("Programa sintacticamente incorrecto.\n");

    /*
     * Si el análisis falla antes de reducir Programa,
     * Bison libera los valores semánticos descartados mediante
     * los destructores definidos arriba.
     */
    raiz = NULL;

    parser_liberar_marcas();
    parser_liberar_contextos_funciones();
    ts_destruir(tabla);
    tabla = NULL;
    return EXIT_FAILURE;
}
