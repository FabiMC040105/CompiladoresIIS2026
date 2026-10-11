
#ifndef TABLA_SIMBOLOS_H
#define TABLA_SIMBOLOS_H

#include <stdio.h>

/*1. TIPOS DE DATOS */

typedef enum {
    TIPO_ENTERO,
    TIPO_VOF,
    TIPO_VACIO,
    TIPO_DESCONOCIDO
} TipoDato;

/*2. CATEGORIAS DE SIMBOLOS*/

typedef enum {
    SIM_VARIABLE,
    SIM_PARAMETRO,
    SIM_FUNCION,
    SIM_ETIQUETA
} CategoriaSimbolo;

/*3. FORMAS DE ALMACENAMIENTO*/

typedef enum {
    FORMA_ESCALAR,
    FORMA_LISTA,
    FORMA_MATRIZ
} FormaSimbolo;

/*4. TIPOS DE AMBITO*/

typedef enum {
    AMBITO_GLOBAL,
    AMBITO_FUNCION,
    AMBITO_BLOQUE,
    AMBITO_FOR
} TipoAmbito;

/* Declaraciones anticipadas */
typedef struct Ambito Ambito;
typedef struct Simbolo Simbolo;
typedef struct ParametroFirma ParametroFirma;

/* ==========================================
   5. ESTRUCTURA DE AMBITOS
   ========================================== */

struct Ambito {
    int id;
    char *nombre;
    TipoAmbito tipo;

    Ambito *padre;
    Ambito *siguiente;
};

/* ==========================================
   6. FIRMA DE FUNCIONES
   ========================================== */

struct ParametroFirma {
    Simbolo *simbolo;
    ParametroFirma *siguiente;
};

/* ==========================================
   7. ESTRUCTURA DE SIMBOLOS
   ========================================== */

struct Simbolo {
    int id;
    char *nombre;

    CategoriaSimbolo categoria;
    TipoDato tipo;
    FormaSimbolo forma;

    /* Listas y matrices */
    int cantidad_dimensiones;
    char *dimensiones[2];

    /* Informacion general */
    int linea;
    Ambito *ambito;

    /* Memoria */
        
    /*
    * Informacion de memoria administrada por el compilador.
    *
    * Si categoria == SIM_VARIABLE y ambito == ambito_global:
    *     direccion contiene una direccion absoluta.
    *
    * Si categoria == SIM_VARIABLE y pertenece a una funcion:
    *     direccion contiene un desplazamiento relativo a x15 (FP).
    *
    * Si categoria == SIM_PARAMETRO:
    *     direccion contiene un desplazamiento relativo a x15 (FP).
    *
    * Si categoria == SIM_FUNCION:
    *     direccion_asignada permanece en 0. La direccion del codigo
    *     se resolvera durante la generacion de codigo.
    *
    * direccion_asignada permite distinguir una direccion valida de
    * un simbolo al que todavia no se le ha asignado ubicacion.
    */
    long direccion;
    int direccion_asignada;


    /* Informacion de funciones */
    ParametroFirma *parametros;
    int cantidad_parametros;
    Ambito *ambito_funcion;

    /* Lista enlazada */
    Simbolo *siguiente;
};

/* ==========================================
   8. TABLA DE SIMBOLOS
   ========================================== */

typedef struct {
    Simbolo *primero;
    Simbolo *ultimo;

    Ambito *ambitos;
    Ambito *ultimo_ambito;

    Ambito *ambito_global;
    Ambito *ambito_actual;

    int siguiente_id_simbolo;
    int siguiente_id_ambito;

    /*
    * Siguiente direccion absoluta disponible para datos globales.
    * No representa el puntero de pila ni el desplazamiento de una
    * variable local.
    */
    long siguiente_direccion_global;
} TablaSimbolos;

/* ==========================================
   9. RESULTADOS DE OPERACIONES
   ========================================== */

typedef enum {
    TS_OK,
    TS_ERROR_DUPLICADO,
    TS_ERROR_MEMORIA,
    TS_ERROR_AMBITO,
    TS_ERROR_ARGUMENTO
} ResultadoTS;

/* ==========================================
   10. ADMINISTRACION DE LA TABLA
   ========================================== */

TablaSimbolos *ts_crear(void);

void ts_destruir(TablaSimbolos *tabla);

/* ==========================================
   11. ADMINISTRACION DE AMBITOS
   ========================================== */

Ambito *ts_entrar_ambito(
    TablaSimbolos *tabla,
    const char *nombre,
    TipoAmbito tipo
);

ResultadoTS ts_salir_ambito(
    TablaSimbolos *tabla
);

/* ==========================================
   12. ADMINISTRACION DE SIMBOLOS
   ========================================== */

ResultadoTS ts_insertar(
    TablaSimbolos *tabla,
    const char *nombre,
    CategoriaSimbolo categoria,
    TipoDato tipo,
    FormaSimbolo forma,
    int linea,
    Simbolo **nuevo
);

Simbolo *ts_buscar_local(
    const TablaSimbolos *tabla,
    const char *nombre,
    const Ambito *ambito
);

Simbolo *ts_buscar_visible(
    const TablaSimbolos *tabla,
    const char *nombre
);

/* ==========================================
   13. LISTAS Y MATRICES
   ========================================== */

ResultadoTS ts_definir_dimensiones(
    Simbolo *simbolo,
    const char *dimension1,
    const char *dimension2
);

ResultadoTS ts_asignar_direcciones_globales(TablaSimbolos *tabla);

/* ==========================================
   14. FUNCIONES Y PARAMETROS
   ========================================== */

ResultadoTS ts_agregar_parametro(
    Simbolo *funcion,
    Simbolo *parametro
);

/* ==========================================
   15. VISUALIZACION
   ========================================== */

void ts_imprimir(
    const TablaSimbolos *tabla,
    FILE *salida
);

void ts_imprimir_ambitos(
    const TablaSimbolos *tabla,
    FILE *salida
);

#endif
