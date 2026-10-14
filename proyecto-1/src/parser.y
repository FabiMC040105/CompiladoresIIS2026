%code requires {

/*
Incluye las definiciones del AST en el encabezado generado por
Bison (parser.tab.h), lo cual permite compartir el tipo NodoAST
entre el lexer y el parser.
*/
#include "ast.h"

}

%{
    /*
    Incluye las biblotecas y los modulos necesarios para la implementacion del parser
    y la integracion con las otras fases del compilador (tabla de simbolos, semantica,
    codegen, resolucion de etiquetas y generacion de codigo maquina), se ponen aqui
    porque estas van en el codigo en C que genera bison y no en el encabezado de parser.tab.h.
     */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "tabla_simbolos.h"
#include "semantico.h"
#include "codegen.h"
#include "codegen_tabla.h"
#include "resolucion_etiquetas.h"
#include "binario.h"
#include "vliw_packer.h"

    /*
    Declaraciones para la comunicacion entre lexer y parser.
    yylex()-> obtiene tokens
    yylineno-> contiene numero de lineas del source
    yyerror()-> reporta erroes sintacticos
    yyin-> flujo de entrada para el archivo del source
    */
int yylex(void);
extern int yylineno;
void yyerror(const char *s);
extern FILE *yyin;

    /*
    Declara las estructuras globales que se utilizan en error_compilacion.
    raiz alamcena la direccion al nodo prinicpal del AST y tabla mantiene.
    direccion a la tabla de simbolos que se va generando durante el analisis.
    */
NodoAST *raiz = NULL;
TablaSimbolos *tabla = NULL;


    /* Opciones de compilacion (-v,-t,-m,-s,-p,-x) recibidas desde la terminal. */
int opcion_verbose = 0;
int opcion_ast = 0;
int opcion_simbolos = 0;
int opcion_asm = 0;
int opcion_paquetes = 0;
int opcion_ensamblar = 0;

    /* ruta_asm y ruta_bin almacenan las rutas de los archivos de ensamblador y codigo maquina. */
char ruta_asm[1024];
char ruta_bin[1024];

    /*---------------------------FUNCIONES CABECERAS Y AMBITOS ---------------------------------------------------------------------------------------------*/

    /*
    Almacenamiento temporal de contexto de funciones reconocidas durante lexer y parser.
    Todos se conserva en lista enlazada.
    */
typedef struct ContextoFuncion {
    const NodoAST *nodo_nombre; /* NULL para principal */
    Ambito *ambito;            /* ambito de parametros de la funcion */
    TipoDato retorno;
    int linea;
    int es_principal;
    struct ContextoFuncion *siguiente;
} ContextoFuncion;

    /* Variables de reconocimiento de cabecera de las funciones del source. */
static ContextoFuncion *contextos_funciones = NULL; /*mantiene lista de funcs*/
static int esperando_tipo_funcion = 0; /*indicacion que se espera el tipo(entero, vof, vacio)*/
static int esperando_nombre_funcion = 0; /*indicacion que se espera nombre de func, y crea ambito con params*/
static TipoDato retorno_lexico = TIPO_DESCONOCIDO;/*almacena tipo de retorno temporalmente*/

    /* Funciones invocadas por lexer para cabecera de funcion. Usadas en Defina y lexer.l y en tipos lexer.l */
void parser_avisar_defina(void) /* inicia reconocimiento*/
{
    esperando_tipo_funcion = 1;
    esperando_nombre_funcion = 0;
    retorno_lexico = TIPO_DESCONOCIDO;
}

void parser_avisar_tipo(TipoDato tipo) /*almacena tipo de retorno y prepara reconocimiento de nombre.*/
{
    if (!esperando_tipo_funcion)
        return;

    retorno_lexico = tipo;
    esperando_tipo_funcion = 0;
    esperando_nombre_funcion = 1;
}

    /*
    Perpara contexto de funcion cuando se reconoce el nombre(lexer), verificacion de de que se declare en ambito global,
    reserva memoria para almacenar informacion y crea el ambito donde se reconocen sus params. Agrega el nuevo contexto
    a la lista de funciones y termina reconocimiento de cabecera.
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

void parser_marcar_principal(int linea) /*Perpara contexto de funcion principal identificandola como punto de entrada al programa*/
{
    parser_preparar_funcion(NULL, "principal", linea, 1);
}

static ContextoFuncion *parser_contexto_funcion(const NodoAST *nombre,
                                                int principal)        /* busca y retorna contexto de funcion previamente reconocida, Null si no encuentra contexto que coincida */
{
    ContextoFuncion *ctx;
    for (ctx = contextos_funciones; ctx != NULL; ctx = ctx->siguiente)
    {
        if (principal ? ctx->es_principal : (!ctx->es_principal && ctx->nodo_nombre == nombre))
            return ctx;
    }
    return NULL;
}

static void parser_liberar_contextos_funciones(void) /*recorre lista de contextos de funcs y libera memoria reservada de elementos(no ambitos)*/
{
    while (contextos_funciones != NULL)
    {
        ContextoFuncion *siguiente = contextos_funciones->siguiente;
        free(contextos_funciones);
        contextos_funciones = siguiente;
    }
}
    /*-------------------------------ADMIN DE AMBITOS Y RELACIONES DE AST----------------------------------------------------------------------------------*/

    /*
    Estructura para asociacion de identificadores en AST con el ambito en que se reconocio.
    nodo -> referencia a id dentro AST
    ambito-> ambito donde se reconocio id
    siguiente-> almacenamiento de marcas en la lista enlazada
    */
typedef struct MarcaIdentificador {
    const NodoAST *nodo;
    Ambito *ambito;
    struct MarcaIdentificador *siguiente;
} MarcaIdentificador;

static MarcaIdentificador *marcas_identificadores = NULL; /* alamcena lista de id */
static int errores_semanticos = 0; /*lleva  conteo de errores detectados*/

/* El lexer indica que clase de bloque se abrira con la proxima llave.
   Solo afecta la etiqueta de visualizacion; NO altera la GLC ni el AST. */
static const char *bloque_pendiente = NULL;
static int linea_bloque_pendiente = 0;

void parser_indicar_bloque(const char *tipo, int linea)/*  Almacena temporalmente el tipo y la linea del siguiente bloque que sera abierto. Info desde lexer*/
{
    bloque_pendiente = tipo;
    linea_bloque_pendiente = linea;
}


void parser_abrir_ambito_lexico(int linea)/*Abre ambito cuando reconoce llave de apertura. Invocadas desde lexer.l exclusivamente para registrar el contexto. */
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


void parser_abrir_ambito_para_lexico(int linea)/*Crae ambito especifico para ciclos "para" mantiene variables declaradas durante inicializacion y dentro de cuerpo. */
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

void parser_cerrar_ambito_lexico(void) /* Cierra ambito actual cuando lexer reconconoce llave de cierre */
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

void parser_marcar_identificador(NodoAST *nodo) /* Registra ambito donde fue reconocido un id, crea marca que relaciona nodo AST con ambito activo y almacena en lista enlazada */
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

Ambito *parser_ambito_identificador(const NodoAST *nodo) /*Busca ambito asociado a id del AST, recorre marcas registradas y retorna el ambito*/
{
    MarcaIdentificador *marca;
    for (marca = marcas_identificadores; marca != NULL; marca = marca->siguiente)
        if (marca->nodo == nodo)
            return marca->ambito;
    return NULL;
}


/* Permite al analisis semantico consultar el ambito registrado por el parser. */
static const Ambito *resolver_ambito_semantico(
    const NodoAST *nodo,
    void *contexto
)
{
    (void)contexto;

    return parser_ambito_identificador(nodo);
}



static void parser_liberar_marcas(void) /*libera la memoria utlizada por lista de marcas de id*/
{
    while (marcas_identificadores != NULL)
    {
        MarcaIdentificador *siguiente = marcas_identificadores->siguiente;
        free(marcas_identificadores);
        marcas_identificadores = siguiente;
    }
}

  /*------------------------------------------------REGISTRO VARIABLES Y PARAMENTROS EN TABLA DE SIMBOLOS--------------------------------------------------------------------------------------------*/

    /*
    Registra variables declaradas en TS durante reduccion de Declaracion,
    obtiene ambito original del id y establece tempralemnte como activo para la insecion,
    permite registrar diferente formas de variables y almacena las dimensiones, detecta
    declaraciones duplicadas en ambitos y errores semanticos, restaura ambito original

    permite acumular errores semanticos sin detener el analisis inmediatamente

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

    if (resultado == TS_ERROR_DUPLICADO) /*nota: permite continuar aunque se registre error semantico si devuelve 1*/
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

    /*
    Registra parametros de funcion en TS, verifica identificadores pertenezcan al ambito de func
    y que la simensionse sean validas para lista y matriz. Parametros se incetan en SIM_PARAMETRO

    deteccion de parametros duplicados y registra errores semanticos
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

    /*
    Registra una funcion en el ambito global de la tabla de simbolos durante su reduccion en Bison.
    Recupera el contexto previamente preparado por el lexer y utiliza su informacion para registrar el nombre,
    tipo de retorno y linea de declaracion.

    Asocia el ambito de la funcion con su simbolo y recorre los parametros registrados previamente para construir
    su firma, conservando el orden de declaracion.

    Detecta funciones duplicadas en el ambito global y registra los errores semanticos correspondientes.

    Tambien permite registrar la funcion principal
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

    /*
    Imprime las firmas de las funciones registradas en la tabla de simbolos.

    Recorre los simbolos de categoria SIM_FUNCION y muestra el tipo de retorno, nombre, parametros,
    formas y dimensiones correspondientes.

    Tambien muestra el identificador del ambito de cada funcion y su cantidad total de parametros.

    Se utiliza para visualizar la informacion de las funciones durante la depuracion del compilador.
    */

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

/*
Configuracion de Bison para construir el parser.
ielr permite generar el analizador con las reglas definidas,
parse.error verbose muestra errores mas detallados y api.value.type
hace que los valores semanticos sean punteros a nodos del AST.
*/
%define lr.type ielr
%define parse.error verbose
%define api.value.type {NodoAST *}
%token SI SINO SINOSI MIENTRAS PARA DEVUELVA DEFINA ROMPA PRINCIPAL
%token LISTA MATRIZ VOF ENTERO VACIO
%token POTENCIA IGUALDAD DIFERENTE MENOR_IGUAL MAYOR_IGUAL AND OR
%token VERDADERO FALSO
%token NUMERO IDENTIFICADOR
%token TRAIGASE EXTENSION_E
%token ERROR_LEXICO FIN_LINEA

    /*
    Los siguientes tokens crean nodos en lexer.l.
    Solo se les aplica destructor a esos tokens con valores
    semánticos propios.
    */
%destructor { ast_liberar($$); } NUMERO IDENTIFICADOR VERDADERO FALSO

    /*
    Estos no terminales pueden contener nodos AST.
    Si Bison los descarta durante la recuperación de un error,
    se libera el árbol que posean.
    Programa se excluye porque su valor se conserva en raiz
    cuando el análisis termina correctamente.
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

/*---------------------------------REGLAS DE LA GRAMATICA Y CONSTRUCCION DEL AST---------------------------------*/
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
    | %empty
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
    |  %empty
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
    | %empty
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
    |  %empty
      {
          $$ = ast_crear_parametros(NULL);
      }
    ;


ParametrosResto
    : ',' Parametro ParametrosResto
      {
          $$ = ast_unir($2, $3);
      }
    |  %empty
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
    | %empty
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
    | %empty
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
    | %empty
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
    | %empty
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
    | %empty
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
    | %empty
      {
          $$ = NULL;
      }
    ;

RestoElementos
    : ',' Expresion RestoElementos
      {
          $$ = ast_unir($2, $3);
      }
    | %empty
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
    | %empty
      {
          $$ = NULL;
      }
    ;

RestoArgumentos
    : ',' Expresion RestoArgumentos
      {
          $$ = ast_unir($2, $3);
      }
    | %empty
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

/*---------------------------------FUNCIONES AUXILIARES Y EJECUCION DEL COMPILADOR---------------------------------*/

/*
Termina la definicion de la gramatica. A partir de aqui se incluyen
las funciones en C para procesar opciones y ejecutar las fases.
*/
%%

    /*
    Recibe los errores detectados por Bison e imprime el numero
    de linea del archivo fuente junto con la descripcion del error.
    */
void yyerror(const char *s)
{
    fprintf(stderr,
            "Error sintactico en la linea %d: %s\n",
            yylineno,
            s);
}

    /*
    Muestra la sintaxis de uso de cdff y explica las opciones de
    compilacion disponibles cuando se solicita ayuda o faltan datos.
    */
static void mostrar_uso(FILE *f)
{
    fprintf(f,
        "Uso:\n"
        "  cdff [opciones] <archivo.e>\n"
        "  cdff -x <archivo.asm> [-o salida.bin] [-p]\n\n"
        "Opciones:\n"
        "  -v              Modo detallado: muestra las fases y decisiones importantes.\n"
        "  -t              Imprime el AST y genera arbol.dot.\n"
        "  -m              Imprime tabla de simbolos, ambitos y firmas.\n"
        "  -o <archivo>    Nombre del archivo binario de salida.\n"
        "  -s              Conserva tambien el ensamblador final (.asm).\n"
        "  -x <archivo>    Ensambla directamente un archivo .asm a codigo maquina.\n"
        "  -p              Muestra cada paquete en ASM, HEX y binario.\n"
        "  -h, --help      Muestra esta ayuda.\n");
}

    /*
    Construye una ruta de salida a partir del archivo de entrada.
    Retira su extension cuando corresponde y agrega la nueva,
    respetando la capacidad del arreglo y separadores de ruta.
    */
static int construir_ruta_con_extension(const char *entrada,
                                         const char *extension,
                                         char *salida,
                                         size_t capacidad)
{
    char *punto;
    char *barra1;
    char *barra2;
    char *barra;

    if (entrada == NULL || extension == NULL || salida == NULL)
        return 0;

    if (snprintf(salida, capacidad, "%s", entrada) >= (int)capacidad)
        return 0;

    punto = strrchr(salida, '.');
    barra1 = strrchr(salida, '/');
    barra2 = strrchr(salida, '\\');
    barra = barra1;
    if (barra2 != NULL && (barra == NULL || barra2 > barra))
        barra = barra2;

    if (punto != NULL && (barra == NULL || punto > barra))
        *punto = '\0';

    if (strlen(salida) + strlen(extension) >= capacidad)
        return 0;

    strcat(salida, extension);
    return 1;
}

    /*
    Copia una ruta al arreglo destino comprobando que no exceda
    el espacio disponible para el nombre del archivo.
    */
static int copiar_ruta(const char *origen, char *destino, size_t capacidad)
{
    return snprintf(destino, capacidad, "%s", origen) < (int)capacidad;
}

    /*
    Funcion principal del compilador. Procesa las opciones recibidas,
    prepara los archivos de salida y ejecuta las fases desde el source
    .e hasta la generacion de bundles VLIW en codigo maquina.
    */
int main(int argc, char *argv[])
{
    int resultado;
    const char *archivo_fuente = NULL;
    const char *archivo_asm_directo = NULL;
    const char *salida_solicitada = NULL;
    int i;
    FILE *entrada = NULL;
    char ruta_asm_intermedia[1024];
    char ruta_asm_resuelta[1024];

    /* ------------------------------------------------------------
     * Procesamiento de opciones de linea de comandos.
     * ------------------------------------------------------------ */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
            opcion_verbose = 1;
        else if (strcmp(argv[i], "-t") == 0)
            opcion_ast = 1;
        else if (strcmp(argv[i], "-m") == 0)
            opcion_simbolos = 1;
        else if (strcmp(argv[i], "-s") == 0)
            opcion_asm = 1;
        else if (strcmp(argv[i], "-p") == 0)
            opcion_paquetes = 1;
        else if (strcmp(argv[i], "-o") == 0)
        {
            if (++i >= argc) {
                fprintf(stderr, "Error: -o requiere un archivo de salida.\n");
                return EXIT_FAILURE;
            }
            salida_solicitada = argv[i];
        }
        else if (strcmp(argv[i], "-x") == 0)
        {
            if (++i >= argc) {
                fprintf(stderr, "Error: -x requiere un archivo .asm.\n");
                return EXIT_FAILURE;
            }
            opcion_ensamblar = 1;
            archivo_asm_directo = argv[i];
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            mostrar_uso(stdout);
            return EXIT_SUCCESS;
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "Error: opcion desconocida: %s\n", argv[i]);
            mostrar_uso(stderr);
            return EXIT_FAILURE;
        }
        else if (archivo_fuente == NULL)
            archivo_fuente = argv[i];
        else
        {
            fprintf(stderr, "Error: se indico mas de un archivo fuente.\n");
            return EXIT_FAILURE;
        }
    }

    /*
    Cuando se utiliza -x, cdff recibe directamente un archivo .asm.
    En ese modo no ejecuta el lexer, parser ni semantico y genera
    el archivo .bin mediante el ensamblador integrado.
    */
    if (opcion_ensamblar)
    {
        if (archivo_fuente != NULL) {
            fprintf(stderr, "Error: -x no se combina con un archivo fuente .e.\n");
            return EXIT_FAILURE;
        }

        if (salida_solicitada != NULL) {
            if (!copiar_ruta(salida_solicitada, ruta_bin, sizeof(ruta_bin))) {
                fprintf(stderr, "Error: ruta de salida demasiado larga.\n");
                return EXIT_FAILURE;
            }
        } else if (!construir_ruta_con_extension(
                       archivo_asm_directo, ".bin", ruta_bin, sizeof(ruta_bin))) {
            fprintf(stderr, "Error: ruta de salida demasiado larga.\n");
            return EXIT_FAILURE;
        }

        if (opcion_verbose)
            printf("[FASE 6] Ensamblando %s -> %s\n",
                   archivo_asm_directo, ruta_bin);

        return binario_ensamblar_archivo(
                   archivo_asm_directo,
                   ruta_bin,
                   opcion_paquetes,
                   stdout)
               ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /*
    Si no se utiliza -x debe recibirse un archivo fuente .e.
    Cuando falta, se imprime el modo de uso y termina la ejecucion.
    */
    if (archivo_fuente == NULL)
    {
        mostrar_uso(stderr);
        return EXIT_FAILURE;
    }

    /*
    Prepara las rutas de los tres archivos de ensamblador:
    intermedio, resuelto y final. Cada uno corresponde a una
    etapa distinta de la traduccion a codigo maquina.
    */
    if (!construir_ruta_con_extension(
            archivo_fuente, ".intermedio.asm",
            ruta_asm_intermedia, sizeof(ruta_asm_intermedia)) ||
        !construir_ruta_con_extension(
            archivo_fuente, ".resuelto.asm",
            ruta_asm_resuelta, sizeof(ruta_asm_resuelta)) ||
        !construir_ruta_con_extension(
            archivo_fuente, ".asm", ruta_asm, sizeof(ruta_asm)))
    {
        fprintf(stderr, "Error: la ruta del archivo es demasiado larga.\n");
        return EXIT_FAILURE;
    }

    if (salida_solicitada != NULL) {
        if (!copiar_ruta(salida_solicitada, ruta_bin, sizeof(ruta_bin))) {
            fprintf(stderr, "Error: ruta de salida demasiado larga.\n");
            return EXIT_FAILURE;
        }
    } else if (!construir_ruta_con_extension(
                   archivo_fuente, ".bin", ruta_bin, sizeof(ruta_bin))) {
        fprintf(stderr, "Error: la ruta binaria es demasiado larga.\n");
        return EXIT_FAILURE;
    }

    /*
    Comprueba que el archivo binario tenga una ruta distinta
    a los archivos de ensamblador para no sobreescribirlos.
    */
    if (strcmp(ruta_asm, ruta_bin) == 0 ||
        strcmp(ruta_asm_intermedia, ruta_bin) == 0 ||
        strcmp(ruta_asm_resuelta, ruta_bin) == 0)
    {
        fprintf(stderr, "Error: el archivo binario no puede usar la misma ruta que el ASM.\n");
        return EXIT_FAILURE;
    }

    /*
    Abre el programa fuente y entrega su flujo a yyin, utilizado
    por el lexer para comenzar a reconocer los tokens.
    */
    entrada = fopen(archivo_fuente, "r");
    if (entrada == NULL)
    {
        perror("Error al abrir el archivo fuente");
        return EXIT_FAILURE;
    }
    yyin = entrada;

    /*
    Crea la tabla de simbolos antes de iniciar yyparse.
    El lexer y las acciones de Bison la utilizan para registrar
    funciones, parametros, variables y ambitos durante el analisis.
    */
    tabla = ts_crear();
    if (tabla == NULL)
    {
        fprintf(stderr, "Error: no se pudo crear la tabla de simbolos.\n");
        fclose(entrada);
        return EXIT_FAILURE;
    }

    if (opcion_verbose)
        printf("[FASES 1-2] Analisis lexico, sintactico y construccion del AST...\n");

    /*
    Ejecuta el reconocimiento lexico y sintactico del source.
    Durante las reducciones se construye el AST y se registran
    las declaraciones; despues se cierra el archivo de entrada.
    */
    resultado = yyparse();
    fclose(entrada);
    entrada = NULL;

    if (resultado == 0)
    {
        if (opcion_verbose)
            printf("\nPrograma sintacticamente correcto.\n");

        if (opcion_verbose)
            printf("[FASE 3] Asignando memoria y ejecutando analisis semantico...\n");

    /*
    Asigna las direcciones de memoria de las variables globales
    antes de ejecutar las comprobaciones semanticas del programa.
    */
        if (ts_asignar_direcciones_globales(tabla) != TS_OK)
        {
            fprintf(stderr,
                    "Error: no se pudieron asignar las direcciones globales.\n");
            goto error_compilacion;
        }

    /*
    Cuando se construyo un AST valido se pueden imprimir sus nodos
    si se solicito -t y continuar con las verificaciones semanticas.
    */
        if (raiz != NULL)
        {
            if (opcion_ast)
            {
                printf("\nArbol de sintaxis abstracta:\n");
                ast_imprimir_consola(raiz, 0);
                ast_imprimir(raiz, 0);
            }

    /*
    Revisa el arbol usando la tabla de simbolos y las marcas de ambito.
    Acumula los errores encontrados para evitar generar ensamblador
    cuando el programa tiene problemas semanticos.
    */
            errores_semanticos += semantico_analizar(
                raiz,
                tabla,
                resolver_ambito_semantico,
                NULL
            );

            if (opcion_verbose)
            {
                if (errores_semanticos == 0)
                    printf("[FASE 3] Analisis semantico finalizado sin errores.\n");
                else
                    printf("[FASE 3] Analisis semantico finalizado con %d errores.\n",
                           errores_semanticos);
            }

    /*
    Las fases de generacion de codigo solo se ejecutan si el
    analisis semantico termino sin errores.
    */
            if (errores_semanticos == 0)
            {
    /*
    Configura la comunicacion del generador de codigo con la tabla
    de simbolos. Permite resolver referencias a identificadores del AST
    y consultar el ambito en el cual fue reconocido cada uno.
    */
                CodegenTablaContexto contexto_codegen;
                CodegenConfig config_codegen = {0};

                contexto_codegen.tabla = tabla;
                contexto_codegen.obtener_ambito_nodo =
                    parser_ambito_identificador;

                config_codegen.resolver_simbolo_nodo =
                    codegen_resolver_desde_tabla;
                config_codegen.contexto_simbolos =
                    &contexto_codegen;

                if (opcion_verbose)
                    printf("[FASE 4] Generando ensamblador y calendarizando dependencias...\n");

    /*
    FASE 4: traduce el AST a ensamblador intermedio y aplica la
    calendarizacion de dependencias para el procesador VLIW.
    */
                if (!codegen_generar_archivo(
                        raiz, ruta_asm_intermedia, &config_codegen))
                {
                    fprintf(stderr,
                            "Error: fallo la generacion de ensamblador.\n");
                    goto error_compilacion;
                }

                if (opcion_verbose)
                    printf("[FASE 5] Resolviendo etiquetas y offsets...\n");

    /*
    FASE 5: sustituye las etiquetas del ensamblador intermedio
    por los offsets necesarios para las instrucciones de salto.
    */
                if (!resolver_etiquetas_asm(
                        ruta_asm_intermedia,
                        ruta_asm_resuelta,
                        opcion_verbose ? stdout : NULL))
                {
                    fprintf(stderr,
                            "Error: fallo la resolucion de etiquetas.\n");
                    goto error_compilacion;
                }

                if (opcion_verbose)
                    printf("[FASE 5.5] Empaquetando operaciones independientes en bundles VLIW...\n");

    /*
    FASE 5.5: revisa las instrucciones del ensamblador resuelto
    y agrupa operaciones independientes dentro de los distintos
    slots de un mismo bundle VLIW antes de generar el .bin.
    */
                if (!vliw_empaquetar_archivo(
                        ruta_asm_resuelta,
                        ruta_asm,
                        opcion_verbose ? stdout : NULL))
                {
                    fprintf(stderr,
                            "Error: fallo el empaquetamiento VLIW.\n");
                    goto error_compilacion;
                }

                if (opcion_verbose)
                    printf("[FASE 6] Generando codigo maquina...\n");

    /*
    FASE 6: transforma los paquetes del ensamblador final en
    la codificacion de codigo maquina definida por la ISA.
    */
                if (!binario_ensamblar_archivo(
                        ruta_asm,
                        ruta_bin,
                        opcion_paquetes,
                        stdout))
                {
                    fprintf(stderr,
                            "Error: fallo la generacion del archivo binario.\n");
                    goto error_compilacion;
                }

    /*
    Al terminar la generacion se eliminan los archivos intermedios.
    El ensamblador final solo se conserva si se solicito la opcion -s.
    */
                remove(ruta_asm_intermedia);
                remove(ruta_asm_resuelta);

                if (!opcion_asm)
                    remove(ruta_asm);
                else if (opcion_verbose)
                    printf("Ensamblador conservado: %s\n", ruta_asm);
            }

    /*
    Libera la memoria reservada para el AST despues de utilizarlo
    en las fases semanticas y de generacion de codigo.
    */
            ast_liberar(raiz);
            raiz = NULL;
        }

    /*
    Con la opcion -m se muestran la tabla de simbolos, los ambitos
    y las firmas de funciones que se construyeron para el source.
    */
        if (opcion_simbolos)
        {
            printf("\nTabla de simbolos:\n");
            ts_imprimir(tabla, stdout);

            printf("\nAmbitos:\n");
            ts_imprimir_ambitos(tabla, stdout);

            printf("\nFirmas de funciones:\n");
            parser_imprimir_firmas(stdout);
        }

        if (errores_semanticos > 0)
            fprintf(stderr, "\nSe encontraron %d errores semanticos.\n",
                    errores_semanticos);

    /*
    Libera las marcas de identificadores, contextos de funciones
    y tabla de simbolos antes de terminar la compilacion.
    */
        parser_liberar_marcas();
        parser_liberar_contextos_funciones();
        ts_destruir(tabla);
        tabla = NULL;
        return errores_semanticos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /*
    Si yyparse detecta un error sintactico se descartan las
    estructuras auxiliares y el compilador termina con error.
    */
    fprintf(stderr, "Programa sintacticamente incorrecto.\n");
    raiz = NULL;
    parser_liberar_marcas();
    parser_liberar_contextos_funciones();
    ts_destruir(tabla);
    tabla = NULL;
    return EXIT_FAILURE;

    /*
    Ruta de limpieza para los errores de generacion de codigo.
    Elimina archivos temporales y libera el AST, los contextos
    y la tabla de simbolos antes de devolver EXIT_FAILURE.
    */
error_compilacion:
    remove(ruta_asm_intermedia);
    remove(ruta_asm_resuelta);
    if (!opcion_asm)
        remove(ruta_asm);
    if (raiz != NULL) {
        ast_liberar(raiz);
        raiz = NULL;
    }
    parser_liberar_marcas();
    parser_liberar_contextos_funciones();
    ts_destruir(tabla);
    tabla = NULL;
    return EXIT_FAILURE;
}
