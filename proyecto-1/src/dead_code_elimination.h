
#ifndef DEAD_CODE_ELIMINATION_H
#define DEAD_CODE_ELIMINATION_H

#include "ir.h"
#include "liveness.h"

/*
Guardamos las estadisticas del analisis DCE
Por ahora contamos cuantas instrucciones revisamos
y cuantas encontramos como posibles candidatas
*/
typedef struct {
    size_t instrucciones_originales;
    size_t posibles_eliminaciones;
} EstadisticasDCE;

/*
Revisa si una instruccion es una asignacion o calculo
que podemos considerar para eliminar
- Retorna 1 si es una operacion candidata
- Retorna 0 si debemos conservarla por seguridad
*/
int es_operacion_eliminable(const IRInstruction *instruccion);

/*
Revisa las instrucciones del programa y muestra cuales
podrian eliminarse segun el resultado de liveness
Por ahora solamente las identifica.
Todavia no modifica el programa IR
*/
int buscar_codigo_muerto(
    const IRProgram *programa,
    const DatosInstruccion *resultados,
    EstadisticasDCE *estadisticas
);


/*
Guarda las estadisticas de la eliminacion real
Nos permite saber cuantas instrucciones habia antes,
cuantas quitamos y cuantas quedaron al final
*/
typedef struct {
    size_t originales;
    size_t eliminadas;
    size_t restantes;
    size_t pasadas;
} ResultadoDCE;

/*
Elimina las asignaciones y calculos cuyos resultados
ya no se necesitan
- Retorna 1 si el proceso termino correctamente
- Retorna 0 si ocurrio algun error
*/
int eliminar_codigo_muerto(
    IRProgram *programa,
    ResultadoDCE *resultado
);

//Muestra las estadisticas finales de la optimizacion
void mostrar_estadisticas_dce(const ResultadoDCE *resultado);

#endif
