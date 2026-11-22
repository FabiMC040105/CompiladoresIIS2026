#ifndef LIVENESS_H
#define LIVENESS_H

#include <stddef.h>
#include <stdio.h>
#include "ir.h"
#include "basic_blocks.h"

/*
Guardamos nombres de variables sin repetirlos.
Por ejemplo, USE puede contener {a, b, %t0}.
Cantidad dice cuantos nombres tenemos y capacidad cuantos espacios reservamos.
Los textos originales pertenecen al IR, aqui solo guardamos sus referencias.
*/
typedef struct {
    const char **nombres;
    size_t cantidad;
    size_t capacidad;
} ConjuntoVariables;

/*
Guardamos los conjuntos que necesitamos para analizar cada bloque.
USE y DEF nos dicen que variables se leen y definen.
IN y OUT nos permiten saber que valores siguen siendo necesarios
antes y despues de ejecutar el bloque.
*/
typedef struct {
    ConjuntoVariables use;
    ConjuntoVariables def;

    // Variables que necesitamos al entrar al bloque.
    ConjuntoVariables in;

    // Variables que necesitamos conservar al salir del bloque.
    ConjuntoVariables out;
} DatosBloque;

/*
Aqui guardamos los resultados de TODOS los bloques del programa.
Por ejemplo, bloques[0] tiene los conjuntos de B0.
*/
typedef struct {
    DatosBloque *bloques;
    size_t cantidad_bloques;
} AnalisisBloques;

// Funciones para trabajar con conjuntos de nombres.
void iniciar_conjunto(ConjuntoVariables *conjunto);
void liberar_conjunto(ConjuntoVariables *conjunto);
int contiene_variable(const ConjuntoVariables *conjunto, const char *nombre);
int agregar_variable(ConjuntoVariables *conjunto, const char *nombre);
void mostrar_conjunto(const ConjuntoVariables *conjunto, FILE *salida);

// Calcula USE y DEF de UNA instruccion. Retorna 1 si todo salio bien.
int obtener_use_def_instruccion(const IRInstruction *instruccion,
                               ConjuntoVariables *use,
                               ConjuntoVariables *def);

// Calcula y guarda USE y DEF de TODOS los bloques. Retorna 1 si funciona.
int analizar_use_def_bloques(const BasicBlockCFG *cfg, AnalisisBloques *analisis);

/*
Calculamos IN y OUT de todos los bloques usando sus conexiones.
Repetimos el proceso hasta que los conjuntos ya no cambien.
Retorna 1 si termina correctamente y 0 si ocurre un error.
*/
int calcular_in_out(const BasicBlockCFG *cfg, AnalisisBloques *analisis);

/*
Mostramos los conjuntos IN y OUT de todos los bloques.
Esto nos permite revisar los resultados del analisis.
*/
void mostrar_in_out_bloques(const BasicBlockCFG *cfg, const AnalisisBloques *analisis);

/*
Calcula las variables vivas al entrar y salir de cada bloque.
Usamos USE y DEF, junto con las conexiones del CFG.
Repetimos los calculos hasta que los conjuntos IN y OUT
dejen de cambiar, porque un bloque puede depender de otros.
Retorna 1 si el analisis termina correctamente y 0 si ocurre un error.
*/
int calcular_in_out(const BasicBlockCFG *cfg, AnalisisBloques *analisis);

// Libera la memoria que utilizamos para guardar el analisis.
void liberar_analisis_bloques(AnalisisBloques *analisis);

// Muestran los resultados en pantalla, sin modificar el programa.
void mostrar_use_def_instrucciones(const IRProgram *programa);
void mostrar_use_def_bloques(const BasicBlockCFG *cfg);

// Se mantienen estos dos nombres para que la prueba anterior siga compilando.
void liveness_print_uses_defs(const IRProgram *programa);
void liveness_print_block_uses_defs(const BasicBlockCFG *cfg);


/*
Guardamos IN y OUT de una instruccion.
IN indica las variables necesarias antes de ejecutarla.
OUT indica las variables necesarias despues de ejecutarla.
*/
typedef struct {
    ConjuntoVariables in;
    ConjuntoVariables out;
} DatosInstruccion;

/*
Calculamos IN y OUT de todas las instrucciones del programa.

Usamos los resultados por bloque que ya calculamos antes.
El arreglo resultados debe tener un espacio por instruccion.
*/
int calcular_in_out_instrucciones(
    const BasicBlockCFG *cfg,
    const AnalisisBloques *analisis,
    DatosInstruccion *resultados
);

/*
Liberamos los conjuntos IN y OUT que guardamos
para cada instruccion.
*/
void liberar_analisis_instrucciones(
    DatosInstruccion *resultados,
    size_t cantidad
);

/*
Mostramos los resultados para comprobar que el
analisis por instruccion funciona correctamente.
*/
void mostrar_in_out_instrucciones(
    const IRProgram *programa,
    const DatosInstruccion *resultados
);


#endif