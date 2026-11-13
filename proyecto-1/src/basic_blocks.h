
#ifndef BASIC_BLOCKS_H
#define BASIC_BLOCKS_H

#include <stddef.h>
#include "ir.h"

/*
BLOQUES BASICOS Y GRAFO DE FLUJO DE CONTROL
Este modulo recibe un programa IR y lo divide en bloques basicos. Despues identifica las
conexiones entre esos bloques para formar el CFG (Control Flow Graph)
 */

//Cada bloque contiene un rango de instrucciones del programa IR original
typedef struct {
    size_t id;       // Identificador del bloque
    size_t start;    // Indice de la primera instruccion
    size_t end;      // Indice de la ultima instruccion
} BasicBlock;

//Representa una conexion entre dos bloques
typedef struct {
    size_t from;     // Bloque de origen
    size_t to;       // Bloque de destino
} CFGEdge;

//Representa el conjunto de bloques basicos y sus conexiones
typedef struct {
    const IRProgram *program; //Referencia al código de tres direcciones original

    BasicBlock *blocks; //Arreglo dinámico de bloques básicos
    size_t block_count;

    CFGEdge *edges; //Arreglo dinámico de conexiones
    size_t edge_count;
} BasicBlockCFG;

//Construye los bloques basicos a partir del IR
/*
Retorna un puntero al CFG creado
Retorna NULL si ocurre un error
*/
BasicBlockCFG *bb_build(const IRProgram *program);

//Construye las conexiones entre los bloques
/*
Retorna 1 si termina correctamente
Retorna 0 si ocurre un error
*/
int bb_build_cfg(BasicBlockCFG *cfg);

//Imprime los bloques basicos
void bb_print(const BasicBlockCFG *cfg);

//Imprime las conexiones del CFG
void bb_print_cfg(const BasicBlockCFG *cfg);

//Libera la memoria utilizada
void bb_free(BasicBlockCFG *cfg);

// Construye e imprime los bloques basicos y el CFG a partir de el programa IR existente
int bb_analyze_and_print(const IRProgram *program);

#endif