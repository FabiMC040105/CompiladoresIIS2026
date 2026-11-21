#ifndef DAG_H
#define DAG_H

#include "dependencias.h"
#include <stdint.h>

#define DAG_NULO SIZE_MAX
#define DAG_CAMPOS 4u

typedef enum {
    DAG_ENTRADA,    /* Valor inicial o desconocido tras invalidacion. */
    DAG_CONSTANTE,
    DAG_OPERACION,  /* Operacion escalar sin barrera. */
    DAG_OPACA       /* Instruccion con barrera: no se comparte su nodo. */
} DAGTipo;

/* Posiciones de operandos: arg1, arg2, result, extra (orden DEP_*). */
typedef struct {
    size_t nodo;          /* ID del valor, o DAG_NULO. */
    size_t definicion;    /* Indice absoluto de la ultima escritura explicita
                             de ESTE nombre, o DAG_NULO para entrada/constante. */
} DAGUso;

typedef struct {
    DAGTipo tipo;
    IROp op;              /* Valido para OPERACION y OPACA. */
    const char *texto;    /* Nombre inicial o constante; prestado de la IR. */
    size_t instruccion;   /* Primera instruccion que crea el nodo, o DAG_NULO. */
    size_t operandos[DAG_CAMPOS]; /* IDs de nodos hijos. */
} DAGNodo;

typedef struct {
    const char *nombre;   /* Prestado de la IR. */
    size_t nodo;
    size_t definicion;
} DAGNombre;

typedef struct {
    size_t indice;        /* Indice absoluto en IRProgram. */
    DepAccesos accesos;
    DAGUso usos[DAG_CAMPOS];
    size_t nodo;          /* Operacion o valor copiado. */
    size_t resultado;     /* Valor escalar definido, o DAG_NULO. */
} DAGInstruccion;

typedef struct {
    BasicBlock bloque;
    DAGNodo *nodos;
    size_t cantidad_nodos, capacidad_nodos;
    DAGNombre *nombres;   /* Tabla de valores vigentes al final del bloque. */
    size_t cantidad_nombres, capacidad_nombres;
    DAGInstruccion *instrucciones;
    size_t cantidad_instrucciones;
} DAGBloque;

/* Construye un DAG local, sin modificar la IR. out debe estar vacio.
   El DAG presta cadenas de la IR: liberar/reconstruir antes de modificarla.
   No prueba identidad de simbolos, liveness, ni equivalencia de memoria.
   Las aristas de datos NO sustituyen las restricciones de DepGrafo. */
int dag_construir(const IRProgram *p, const BasicBlock *b, DAGBloque *out);
void dag_liberar(DAGBloque *dag);
const DAGNombre *dag_buscar_nombre(const DAGBloque *dag, const char *nombre);
const DAGInstruccion *dag_buscar_instruccion(const DAGBloque *dag, size_t indice);
int dag_indice_campo(unsigned campo); /* -1 si no es un campo individual. */
void dag_imprimir(const DAGBloque *dag, FILE *out);

#endif
