
#include "basic_blocks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Verifica si una instruccion cambia o termina el flujo normal de ejecucion
Estas instrucciones nos ayudan a identificar donde termina un bloque basico
Retorna:
1 -> La instruccion termina el bloque
0 -> La instruccion no termina el bloque
*/
static int bb_is_terminator(IROp op) {

    switch (op) {

        // Salto incondicional
        case IR_GOTO:
            return 1;

        // Saltos condicionales
        case IR_IF_TRUE:
        case IR_IF_FALSE:
            return 1;

        // Retorno de una funcion
        case IR_RETURN:
            return 1;

        // Fin de una funcion
        case IR_FUNC_END:
            return 1;

        // Cualquier otra operacion
        default:
            return 0;
    }
}


/*
Busca la posicion de una etiqueta dentro del IR (para los saltos)
Retorna:
El indice de la etiqueta si la encuentra
-1 si la etiqueta no existe
*/
static int bb_find_label(
    const IRProgram *program,
    const char *label
) {
    // Verificamos que los datos sean validos
    if (program == NULL || label == NULL) {
        return -1;
    }

    // Recorremos todas las instrucciones
    for (size_t i = 0; i < program->count; i++) {

        const IRInstruction *inst =
            &program->instructions[i];

        // Solo nos interesan las etiquetas
        if (inst->op == IR_LABEL && inst->result != NULL) {

            // strcmp devuelve 0 si son iguales
            if (strcmp(inst->result, label) == 0) {
                return (int)i;
            }
        }
    }

    // No encontramos la etiqueta
    return -1;
}


/*
Identifica los lideres del programa IR
leaders[i] = 1 -> La instruccion i es lider
leaders[i] = 0 -> No es lider
El arreglo leaders debe tener espacio para program->count elementos
Retorna:
1 -> Analisis completado
0 -> Error
*/
static int bb_find_leaders(
    const IRProgram *program,
    int *leaders
) {
    if (program == NULL || leaders == NULL) {
        return 0;
    }

    if (program->count == 0) {
        return 1;
    }

    if (program->instructions == NULL) {
        return 0;
    }

    // Inicialmente ninguna instruccion es lider
    for (size_t i = 0; i < program->count; i++) {
        leaders[i] = 0;
    }

    // REGLA 1: La primera instruccion es lider
    leaders[0] = 1;

    // Recorremos todas las instrucciones del IR
    for (size_t i = 0; i < program->count; i++) {

        const IRInstruction *inst =
            &program->instructions[i];

        // REGLA 2: La instruccion despues de un salto o terminador es lider
        if (bb_is_terminator(inst->op)) {

            if (i + 1 < program->count) {
                leaders[i + 1] = 1;
            }
        }

        // Los inicios de funciones tambien deben comenzar un bloque nuevo
        if (inst->op == IR_FUNC_BEGIN) {
            leaders[i] = 1;
        }

        // REGLA 3: El destino de un salto es lider
        if (inst->op == IR_GOTO ||
            inst->op == IR_IF_TRUE ||
            inst->op == IR_IF_FALSE) {

            int target = bb_find_label(
                program,
                inst->result
            );

            // Un salto sin destino valido es un error en el IR que recibimos
            if (target < 0) {
                fprintf(
                    stderr,
                    "Error: etiqueta de salto no encontrada\n"
                );
                return 0;
            }

            leaders[target] = 1;
        }
    }

    return 1;
}


/*
Construye los bloques basicos de un programa IR
Recibe:
program: programa con las instrucciones IR.
Retorna:
Un puntero a BasicBlockCFG si todo sale bien
NULL si ocurre un error
 */
BasicBlockCFG *bb_build(const IRProgram *program) {

    // 1. Verificamos que el programa exista
    if (program == NULL) {
        return NULL;
    }

    // Si hay instrucciones, debe existir el arreglo
    if (program->count > 0 && program->instructions == NULL) {
        return NULL;
    }

    // 2. Reservamos memoria para la estructura CFG
    BasicBlockCFG *cfg = malloc(sizeof(BasicBlockCFG));

    // Comprobamos que malloc haya funcionado
    if (cfg == NULL) {
        fprintf(stderr, "Error: no se pudo reservar memoria para CFG\n");
        return NULL;
    }

    // 3. Inicializamos los campos de la estructura
    cfg->program = program;

    cfg->blocks = NULL;
    cfg->block_count = 0;

    cfg->edges = NULL;
    cfg->edge_count = 0;

    // Si el programa esta vacio, no hay bloques
    if (program->count == 0) {
        return cfg;
    }

    // 4. Reservamos espacio para los lideres
    int *leaders = malloc(
        program->count * sizeof(int)
    );

    if (leaders == NULL) {
        fprintf(stderr, "Error: no se pudo reservar memoria para lideres\n");
        free(cfg);
        return NULL;
    }

    // 5. Identificamos los lideres del programa
    if (!bb_find_leaders(program, leaders)) {
        fprintf(stderr, "Error: no se pudieron identificar los lideres\n");
        free(leaders);
        free(cfg);
        return NULL;
    }

    // 6. Contamos cuantos bloques necesitamos
    size_t total_blocks = 0;

    for (size_t i = 0; i < program->count; i++) {
        if (leaders[i] == 1) {
            total_blocks++;
        }
    }

    // 7. Reservamos memoria para los bloques
    cfg->blocks = malloc(
        total_blocks * sizeof(BasicBlock)
    );

    if (cfg->blocks == NULL) {
        fprintf(stderr, "Error: no se pudo reservar memoria para bloques\n");
        free(leaders);
        free(cfg);
        return NULL;
    }

    // 8. Construimos los bloques basicos
    size_t current_block = 0;

    for (size_t i = 0; i < program->count; i++) {

        // Cada lider marca el inicio de un bloque
        if (leaders[i] == 1) {

            // Cerramos el bloque anterior
            if (current_block > 0) {
                cfg->blocks[current_block - 1].end = i - 1;
            }

            // Creamos el nuevo bloque
            cfg->blocks[current_block].id = current_block;
            cfg->blocks[current_block].start = i;

            // El final se actualizara al encontrar el siguiente lider o terminar el IR
            cfg->blocks[current_block].end = i;

            current_block++;
        }
    }

    // 9. Cerramos el ultimo bloque
    cfg->blocks[current_block - 1].end =
        program->count - 1;

    // Guardamos la cantidad total de bloques
    cfg->block_count = current_block;

    // 10. Liberamos el arreglo temporal de lideres
    free(leaders);

    // Devolvemos el resultado construido
    return cfg;
}


/*
Busca el bloque que contiene una instruccion
Recibe:
cfg: conjunto de bloques basicos
instruction_index: indice de la instruccion
Retorna:
El indice del bloque si lo encuentra
-1 si no existe
 */
static int bb_find_block_by_instruction(
    const BasicBlockCFG *cfg,
    size_t instruction_index
) {
    if (cfg == NULL) {
        return -1;
    }

    // Recorremos todos los bloques
    for (size_t i = 0; i < cfg->block_count; i++) {

        const BasicBlock *block = &cfg->blocks[i];

        // Verificamos si la instruccion esta dentro del rango de este bloque
        if (instruction_index >= block->start &&
            instruction_index <= block->end) {

            return (int)i;
        }
    }

    return -1;
}


/*
Construye el grafo de flujo de control (CFG)
Analiza la ultima instruccion de cada bloque y determina hacia cuales bloques puede continuar
Retorna:
1 -> CFG construido correctamente
0 -> Ocurrio un error
*/
int bb_build_cfg(BasicBlockCFG *cfg) {

    // 1. Verificamos los datos recibidos
    if (cfg == NULL || cfg->program == NULL) {
        return 0;
    }

    if (cfg->block_count > 0 &&
        (cfg->blocks == NULL ||
         cfg->program->instructions == NULL)) {
        return 0;
    }

    // 2. Eliminamos conexiones anteriores, por si esta funcion se ejecuta otra vez
    free(cfg->edges);

    cfg->edges = NULL;
    cfg->edge_count = 0;

    // Si no hay bloques, no hay conexiones
    if (cfg->block_count == 0) {
        return 1;
    }

    // 3. Reservamos espacio para las conexiones
    // Cada bloque puede tener como maximo dos sucesores en el IR que estamos manejando
    if (cfg->block_count > ((size_t)-1) /
                           (2 * sizeof(CFGEdge))) {
        return 0;
    }

    size_t max_edges = cfg->block_count * 2;

    cfg->edges = malloc(max_edges * sizeof(CFGEdge));

    if (cfg->edges == NULL) {
        fprintf(stderr,
                "Error: no se pudo reservar memoria para CFG\n");
        return 0;
    }

    // 4. Recorremos todos los bloques
    for (size_t i = 0; i < cfg->block_count; i++) {

        const BasicBlock *block = &cfg->blocks[i];

        // Obtenemos la ultima instruccion
        const IRInstruction *last =
            &cfg->program->instructions[block->end];

        // Verificamos si existe un bloque siguiente
        int has_next = (i + 1 < cfg->block_count);

        // Evitamos conectar funciones diferentes
        if (has_next) {

            size_t next_start = cfg->blocks[i + 1].start;

            const IRInstruction *next =
                &cfg->program->instructions[next_start];

            if (next->op == IR_FUNC_BEGIN) {
                has_next = 0;
            }
        }

        // CASO 1: SALTO INCONDICIONAL
        if (last->op == IR_GOTO) {

            int label_index = bb_find_label(
                cfg->program,
                last->result
            );

            if (label_index < 0) {
                goto error;
            }

            int target = bb_find_block_by_instruction(
                cfg,
                (size_t)label_index
            );

            if (target < 0) {
                goto error;
            }

            // Agregamos la conexion hacia el destino
            cfg->edges[cfg->edge_count].from = i;
            cfg->edges[cfg->edge_count].to =
                (size_t)target;

            cfg->edge_count++;
        }

        // CASO 2: SALTO CONDICIONAL
        else if (last->op == IR_IF_TRUE ||
                 last->op == IR_IF_FALSE) {

            int label_index = bb_find_label(
                cfg->program,
                last->result
            );

            if (label_index < 0) {
                goto error;
            }

            int target = bb_find_block_by_instruction(
                cfg,
                (size_t)label_index
            );

            if (target < 0) {
                goto error;
            }

            // Primera conexion: destino del salto
            cfg->edges[cfg->edge_count].from = i;
            cfg->edges[cfg->edge_count].to =
                (size_t)target;

            cfg->edge_count++;

            // Segunda conexion: siguiente bloque
            // Evitamos duplicar la misma conexion
            if (has_next && (size_t)target != i + 1) {

                cfg->edges[cfg->edge_count].from = i;
                cfg->edges[cfg->edge_count].to = i + 1;

                cfg->edge_count++;
            }
        }

        // CASO 3: RETORNO O FIN DE FUNCION
        else if (last->op == IR_RETURN ||
                 last->op == IR_FUNC_END) {

            // No agregamos conexiones. La ejecucion termina en este bloque
        }

        // CASO 4: CONTINUACION NORMAL
        else if (has_next) {

            cfg->edges[cfg->edge_count].from = i;
            cfg->edges[cfg->edge_count].to = i + 1;

            cfg->edge_count++;
        }
    }

    // CFG construido correctamente
    return 1;

error:
    fprintf(stderr,
            "Error: no se pudo construir una conexion del CFG\n");

    free(cfg->edges);
    cfg->edges = NULL;
    cfg->edge_count = 0;

    return 0;
}


/*
Imprime los bloques basicos construidos
Muestra:
Identificador de cada bloque
Indice de la primera instruccion
Indice de la ultima instruccion
Cantidad de instrucciones del bloque
Recibe:
cfg: estructura con los bloques basicos
*/
void bb_print(const BasicBlockCFG *cfg) {

    // 1. Verificamos que el CFG exista
    if (cfg == NULL) {
        printf("Error: el CFG no existe.\n");
        return;
    }

    // 2. Imprimimos el encabezado
    printf("\n================================\n");
    printf("       BLOQUES BASICOS\n");
    printf("================================\n");

    // 3. Verificamos si existen bloques
    if (cfg->block_count == 0) {
        printf("No hay bloques basicos.\n");
        return;
    }

    if (cfg->blocks == NULL) {
        printf("Error: no existe el arreglo de bloques.\n");
        return;
    }

    // 4. Recorremos todos los bloques
    for (size_t i = 0; i < cfg->block_count; i++) {

        // Obtenemos el bloque actual
        const BasicBlock *block = &cfg->blocks[i];

        // Calculamos cuantas instrucciones contiene
        size_t instruction_count =
            block->end - block->start + 1;

        // 5. Mostramos su informacion
        printf("\nBloque B%zu\n", block->id);

        printf("  Inicio: instruccion %zu\n",
               block->start);

        printf("  Final:  instruccion %zu\n",
               block->end);

        printf("  Cantidad de instrucciones: %zu\n",
               instruction_count);
    }

    // 6. Mostramos la cantidad total de bloques
    printf("\nTotal de bloques: %zu\n",
           cfg->block_count);

    printf("================================\n");
}


/*
Imprime las conexiones del grafo de flujo de control
Cada conexion indica que desde un bloque se puede continuar hacia otro bloque
Ejemplo:
B0 -> B1
B0 -> B2
B1 -> B3
Recibe:
cfg: estructura con los bloques y conexiones
 */
void bb_print_cfg(const BasicBlockCFG *cfg) {

    // 1. Verificamos que el CFG exista
    if (cfg == NULL) {
        printf("Error: el CFG no existe.\n");
        return;
    }

    // 2. Imprimimos el encabezado
    printf("\n================================\n");
    printf("    GRAFO DE FLUJO DE CONTROL\n");
    printf("================================\n");

    // 3. Verificamos si existen conexiones
    if (cfg->edge_count == 0) {
        printf("No hay conexiones entre bloques.\n");
        return;
    }

    // Verificamos que el arreglo exista
    if (cfg->edges == NULL) {
        printf("Error: no existe el arreglo de conexiones.\n");
        return;
    }

    // 4. Recorremos todas las conexiones
    for (size_t i = 0; i < cfg->edge_count; i++) {

        // Obtenemos la conexion actual
        const CFGEdge *edge = &cfg->edges[i];

        // 5. Mostramos el origen y el destino
        printf("B%zu -> B%zu\n",
               edge->from,
               edge->to);
    }

    // 6. Mostramos el total de conexiones
    printf("\nTotal de conexiones: %zu\n",
           cfg->edge_count);

    printf("================================\n");
}


/*
Libera la memoria utilizada por los bloques basicos y el grafo de flujo de control
Libera:
El arreglo de bloques basicos
El arreglo de conexiones del CFG
La estructura principal BasicBlockCFG
No libera el IR original, porque pertenece al modulo encargado de generar el IR
Recibe:
cfg: estructura que queremos liberar
*/
void bb_free(BasicBlockCFG *cfg) {

    // 1. Verificamos que el CFG exista
    if (cfg == NULL) {
        return;
    }

    // 2. Liberamos el arreglo de bloques basicos
    free(cfg->blocks);

    // 3. Liberamos las conexiones del CFG
    free(cfg->edges);

    // 4. Liberamos la estructura principal
    free(cfg);
}


/*
Integra la representacion intermedia con el analisis de bloques basicos y CFG
Recibe un programa IR que ya fue generado por el modulo de representacion intermedia
Retorna:
1 si el analisis termina correctamente
0 si ocurre algun error
*/
int bb_analyze_and_print(const IRProgram *program)
{
    BasicBlockCFG *cfg = NULL;

    // Verificamos que exista un programa IR
    if (program == NULL) {
        fprintf(stderr,
                "Error: no se recibio un programa IR.\n");
        return 0;
    }

    // Mostramos las instrucciones de tres direcciones
    printf("\n========== REPRESENTACION INTERMEDIA ==========\n");
    ir_print(program, stdout);

    // Construimos los bloques basicos utilizando el IR
    cfg = bb_build(program);

    if (cfg == NULL) {
        fprintf(stderr,
                "Error: no se pudieron construir los bloques basicos.\n");
        return 0;
    }

    // Construimos las conexiones entre los bloques
    if (!bb_build_cfg(cfg)) {
        fprintf(stderr,
                "Error: no se pudo construir el CFG.\n");

        bb_free(cfg);
        return 0;
    }

    // Mostramos los bloques y las conexiones
    bb_print(cfg);
    bb_print_cfg(cfg);

    // Liberamos el CFG, pero no el programa IR original
    bb_free(cfg);

    return 1;
}