#ifndef DEPENDENCIAS_H
#define DEPENDENCIAS_H

#include "basic_blocks.h"

/* Campos reales del cuadruplo. Las mascaras permiten localizar usos para
   el futuro renombramiento sin confundir etiquetas, tipos y nombres de funciones. */
enum {
    DEP_ARG1 = 1u, DEP_ARG2 = 2u, DEP_RESULT = 4u, DEP_EXTRA = 8u
};

typedef struct {
    unsigned lecturas;       /* Mascara de campos que leen un nombre. */
    unsigned escrituras;     /* Mascara de campos que definen un valor escalar. */
    unsigned declaracion;    /* Campo que introduce una variable (no un uso). */
    int lee_memoria;         /* Acceso explicito a memoria de arreglos o llamada opaca. */
    int escribe_memoria;
    int barrera;             /* No mover otras instrucciones a traves de esta. */
} DepAccesos;

typedef enum {
    DEP_RAW = 1u, DEP_WAR = 2u, DEP_WAW = 4u,
    DEP_MEMORIA = 8u, DEP_BARRERA = 16u
} DepTipo;

typedef struct {
    size_t origen, destino;  /* Indices absolutos en IRProgram, desde cero. */
    DepTipo tipo;
    const char *nombre;      /* Prestado de IRProgram; NULL para restricciones sin nombre. */
} DepArista;

typedef struct {
    DepArista *aristas;
    size_t cantidad, capacidad;
} DepGrafo;

/* 1 = opcode reconocido y operandos de valor validos; 0 = error.
   No modifica la IR. Los nombres son textuales, no identidades de simbolo. */
int dep_accesos(const IRInstruction *inst, DepAccesos *out);
const char *dep_campo(const IRInstruction *inst, unsigned campo);
int dep_es_nombre(const char *texto);

/* Intersecciones textuales para dos instrucciones, sin contexto de bloque.
   Puede devolver varios tipos a la vez; no implica que RAW alcance realmente j.
   Devuelve 0 si hay un operando invalido o no hay intersecciones; usar
   dep_accesos() para distinguir ambos casos. */
unsigned dep_tipos_par(const IRInstruction *i, const IRInstruction *j);

/* Analiza un bloque: RAW de la ultima definicion, WAR hacia la siguiente
   escritura y WAW entre escrituras consecutivas; restricciones conservadoras
   por memoria y barreras. Una arista por nombre y tipo.
   Inicializa out (debe estar vacio); 0 en error, 1 en exito.
   Reconstruir tras transformar la IR; no conservar punteros a nombres viejos. */
int dep_analizar_bloque(const IRProgram *programa, const BasicBlock *bloque, DepGrafo *out);
void dep_liberar(DepGrafo *grafo);
void dep_imprimir(const DepGrafo *grafo, FILE *out);
const char *dep_tipo_nombre(DepTipo tipo);

#endif
