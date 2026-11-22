#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include "ast.h"

typedef enum {
    CODEGEN_FORMA_ESCALAR = 0,
    CODEGEN_FORMA_LISTA,
    CODEGEN_FORMA_MATRIZ
} CodegenForma;

typedef enum {
    CODEGEN_CAT_VARIABLE = 0,
    CODEGEN_CAT_PARAMETRO,
    CODEGEN_CAT_FUNCION
} CodegenCategoria;

typedef enum {
    CODEGEN_UBICACION_GLOBAL = 0,
    CODEGEN_UBICACION_LOCAL,
    CODEGEN_UBICACION_PARAMETRO,
    CODEGEN_UBICACION_FUNCION
} CodegenUbicacion;

/*
 * Vista de un simbolo que necesita el backend.
 * direccion es firmada porque los locales usan offsets negativos respecto a FP.
 */
typedef struct {
    const char *nombre;
    long direccion;
    const char *tipo;

    CodegenForma forma;
    unsigned cantidad_dimensiones;
    uint32_t dimensiones[2];

    CodegenCategoria categoria;
    CodegenUbicacion ubicacion;
    uint32_t tamano_bytes;

    /* Solo tienen significado para SIM_FUNCION. */
    uint32_t tamano_parametros;
    uint32_t tamano_locales;
    uint32_t tamano_marco;

    /* Parametros lista/matriz contienen una direccion de 4 bytes. */
    int parametro_por_referencia;
} CodegenSimbolo;

/* Compatibilidad con pruebas antiguas sin informacion de ambito. */
typedef int (*CodegenResolverSimbolo)(void *contexto,
                                      const char *nombre,
                                      CodegenSimbolo *salida);

/* Resolvedor recomendado para la integracion con la Fase 3. */
typedef int (*CodegenResolverSimboloNodo)(void *contexto,
                                          const NodoAST *nodo_identificador,
                                          const char *nombre,
                                          CodegenSimbolo *salida);

typedef struct {
    CodegenResolverSimbolo resolver_simbolo;
    CodegenResolverSimboloNodo resolver_simbolo_nodo;
    void *contexto_simbolos;
    /* Compatibilidad historica: el scheduler actual ignora este campo. */
    unsigned nops_dependencia;
    /* Primer byte libre despues de las globales (limite para x14/SP). */
    uint32_t fin_globales;
} CodegenConfig;

int codegen_generar_archivo(NodoAST *raiz,
                            const char *ruta_salida,
                            const CodegenConfig *config);


/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/

/*
 * Interfaz compartida para reutilizar el CodeGen original.
 *
 * El adaptador IR podra enviar instrucciones de ensamblador
 * a este motor sin tener que implementar nuevamente el
 * emisor, el scheduler y el manejo de registros.
 *
 * La estructura es opaca para que los detalles internos
 * del generador sigan definidos solamente en codegen.c.
 */
typedef struct CodegenMotor CodegenMotor;

/*
 * Esta funcion preparara el motor de generacion.
 * Recibe la ruta del ASM y la configuracion de simbolos.
 */
CodegenMotor *codegen_motor_crear(
    const char *ruta_salida,
    const CodegenConfig *config
);

/*
 * Permite emitir una instruccion utilizando el scheduler
 * y las convenciones del CodeGen original.
 */
int codegen_motor_emitir(
    CodegenMotor *motor,
    const char *slot,
    const char *instruccion
);

/*
 * Permite colocar etiquetas en el ensamblador.
 */
int codegen_motor_etiqueta(
    CodegenMotor *motor,
    const char *etiqueta
);

/*
 * Carga el valor de un simbolo en un registro.
 *
 * El simbolo ya debe estar resuelto desde la tabla de simbolos.
 * El motor se encarga de calcular su direccion, ya sea global,
 * local o un parametro de funcion.
 */
int codegen_motor_cargar_simbolo(
    CodegenMotor *motor,
    const CodegenSimbolo *simbolo,
    int reg_destino
);

/*
 * Guarda el contenido de un registro en un simbolo.
 *
 * Utilizamos las mismas convenciones de memoria del
 * CodeGen original para no duplicar esa logica en el IR.
 */
int codegen_motor_guardar_simbolo(
    CodegenMotor *motor,
    const CodegenSimbolo *simbolo,
    int reg_valor
);


/*
 * Prepara el marco de pila de una funcion.
 *
 * Las funciones normales guardan el FP anterior y
 * reservan espacio para sus variables locales.
 *
 * Principal se maneja aparte porque no tiene
 * una funcion que la haya llamado.
 */
int codegen_motor_iniciar_funcion(
    CodegenMotor *motor,
    const char *nombre,
    uint32_t tamano_marco,
    int es_principal
);

/*
 * Finaliza una funcion y restaura su marco de pila.
 *
 * Las funciones normales recuperan el FP anterior
 * y la direccion de retorno.
 *
 * Principal simplemente libera sus variables
 * locales y salta al final del programa.
 */
int codegen_motor_finalizar_funcion(
    CodegenMotor *motor,
    const char *nombre,
    int es_principal
);


/*
 * Finaliza la generacion y libera los recursos del motor.
 */
int codegen_motor_cerrar(CodegenMotor *motor);

/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/
/* ------------------------------------------ NUEVO -----------------------------------------*/

#endif
