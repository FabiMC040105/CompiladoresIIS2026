
#include "../basic_blocks.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
PRUEBAS DE BLOQUES BASICOS Y CFG
Construimos instrucciones IR manualmente
No necesitamos ejecutar el analizador ni generar un AST
Cada prueba verifica:
Cantidad de bloques
Rangos de instrucciones
Conexiones del CFG
*/

static IRProgram crear_programa(
    IRInstruction *instructions,
    size_t count
) {
    IRProgram program = {0};

    program.instructions = instructions;
    program.count = count;
    program.capacity = count;

    return program;
}

/*
Comprueba si existe una conexion desde el bloque 'from' hasta 'to'
*/
static int existe_conexion(
    const BasicBlockCFG *cfg,
    size_t from,
    size_t to
) {
    for (size_t i = 0; i < cfg->edge_count; i++) {
        if (cfg->edges[i].from == from &&
            cfg->edges[i].to == to) {
            return 1;
        }
    }

    return 0;
}

/*
Construye los bloques y sus conexiones
Detiene la prueba si ocurre un error
*/
static BasicBlockCFG *construir_cfg(
    const IRProgram *program
) {
    BasicBlockCFG *cfg = bb_build(program);

    assert(cfg != NULL);
    assert(bb_build_cfg(cfg) == 1);

    return cfg;
}

/*
PRUEBA 1: Programa sin saltos
Esperamos:
Un unico bloque
Ninguna conexion
*/
static void test_sin_saltos(void) {

    IRInstruction instructions[] = {
        {.op = IR_COPY, .arg1 = "5", .result = "a"},
        {.op = IR_COPY, .arg1 = "10", .result = "b"},
        {.op = IR_ADD, .arg1 = "a", .arg2 = "b",
         .result = "c"},
        {.op = IR_RETURN, .arg1 = "c"}
    };

    IRProgram program = crear_programa(instructions, 4);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 1);
    assert(cfg->blocks[0].start == 0);
    assert(cfg->blocks[0].end == 3);
    assert(cfg->edge_count == 0);

    printf("OK: programa sin saltos\n");

    bb_free(cfg);
}

/*
PRUEBA 2: Salto condicional
B0 -> B2 (salto)
B0 -> B1 (continuacion)
B1 -> B2
*/
static void test_condicional(void) {

    IRInstruction instructions[] = {
        {.op = IR_COPY, .arg1 = "5", .result = "a"},
        {.op = IR_IF_FALSE, .arg1 = "a", .result = "L1"},
        {.op = IR_ADD, .arg1 = "a", .arg2 = "1",
         .result = "a"},
        {.op = IR_LABEL, .result = "L1"},
        {.op = IR_RETURN, .arg1 = "a"}
    };

    IRProgram program = crear_programa(instructions, 5);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 3);
    assert(cfg->edge_count == 3);

    assert(existe_conexion(cfg, 0, 2));
    assert(existe_conexion(cfg, 0, 1));
    assert(existe_conexion(cfg, 1, 2));

    printf("OK: salto condicional\n");

    bb_free(cfg);
}

/*
PRUEBA 3: Salto incondicional
B0 -> B2
No debe existir B0 -> B1
*/
static void test_goto(void) {

    IRInstruction instructions[] = {
        {.op = IR_GOTO, .result = "L1"},
        {.op = IR_COPY, .arg1 = "100", .result = "a"},
        {.op = IR_LABEL, .result = "L1"},
        {.op = IR_RETURN, .arg1 = "a"}
    };

    IRProgram program = crear_programa(instructions, 4);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 3);
    assert(cfg->edge_count == 2);

    assert(existe_conexion(cfg, 0, 2));
    assert(!existe_conexion(cfg, 0, 1));
    assert(existe_conexion(cfg, 1, 2));

    printf("OK: salto incondicional\n");

    bb_free(cfg);
}

/*
PRUEBA 4: Ciclo con salto hacia atras
El ultimo bloque regresa al primero
*/
static void test_ciclo(void) {

    IRInstruction instructions[] = {
        {.op = IR_LABEL, .result = "L0"},
        {.op = IR_IF_FALSE, .arg1 = "cond",
         .result = "L1"},
        {.op = IR_ADD, .arg1 = "i", .arg2 = "1",
         .result = "i"},
        {.op = IR_GOTO, .result = "L0"},
        {.op = IR_LABEL, .result = "L1"},
        {.op = IR_RETURN, .arg1 = "i"}
    };

    IRProgram program = crear_programa(instructions, 6);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 3);
    assert(cfg->edge_count == 3);

    assert(existe_conexion(cfg, 0, 1));
    assert(existe_conexion(cfg, 0, 2));
    assert(existe_conexion(cfg, 1, 0));

    printf("OK: ciclo con salto hacia atras\n");

    bb_free(cfg);
}

/*
PRUEBA 5: Retorno de funcion
Un return no puede conectarse con la instruccion siguiente
*/
static void test_return(void) {

    IRInstruction instructions[] = {
        {.op = IR_COPY, .arg1 = "10", .result = "a"},
        {.op = IR_RETURN, .arg1 = "a"},
        {.op = IR_COPY, .arg1 = "20", .result = "b"}
    };

    IRProgram program = crear_programa(instructions, 3);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 2);
    assert(cfg->edge_count == 0);

    printf("OK: return sin conexion posterior\n");

    bb_free(cfg);
}

/*
PRUEBA 6: Programa vacio
*/
static void test_programa_vacio(void) {

    IRProgram program = {0};

    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 0);
    assert(cfg->edge_count == 0);

    printf("OK: programa vacio\n");

    bb_free(cfg);
}

/*
PRUEBA 7: Etiqueta inexistente
bb_build debe detectar el error
*/
static void test_etiqueta_inexistente(void) {

    IRInstruction instructions[] = {
        {.op = IR_GOTO, .result = "NO_EXISTE"}
    };

    IRProgram program = crear_programa(instructions, 1);

    BasicBlockCFG *cfg = bb_build(&program);

    assert(cfg == NULL);

    printf("OK: etiqueta inexistente detectada\n");
}

/*
PRUEBA 8: Condicional cuyo destino es el bloque siguiente
No debe duplicar la conexion
*/
static void test_sin_conexion_duplicada(void) {

    IRInstruction instructions[] = {
        {.op = IR_IF_TRUE, .arg1 = "cond",
         .result = "L1"},
        {.op = IR_LABEL, .result = "L1"},
        {.op = IR_RETURN, .arg1 = "a"}
    };

    IRProgram program = crear_programa(instructions, 3);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 2);
    assert(cfg->edge_count == 1);
    assert(existe_conexion(cfg, 0, 1));

    printf("OK: conexiones duplicadas evitadas\n");

    bb_free(cfg);
}

/*
PRUEBA 9: Dos funciones independientes
El final de la primera funcion no debe conectarse con la segunda
*/
static void test_dos_funciones(void) {

    IRInstruction instructions[] = {
        {.op = IR_FUNC_BEGIN, .result = "f1"},
        {.op = IR_RETURN, .arg1 = "1"},
        {.op = IR_FUNC_END, .result = "f1"},
        {.op = IR_FUNC_BEGIN, .result = "f2"},
        {.op = IR_RETURN, .arg1 = "2"},
        {.op = IR_FUNC_END, .result = "f2"}
    };

    IRProgram program = crear_programa(instructions, 6);
    BasicBlockCFG *cfg = construir_cfg(&program);

    assert(cfg->block_count == 4);
    assert(cfg->edge_count == 0);

    printf("OK: funciones independientes\n");

    bb_free(cfg);
}

/*
PRUEBA 10: Impresion de bloques y conexiones
Comprobamos que las funciones puedan ejecutarse con un CFG valido
*/
static void test_impresion(void) {

    IRInstruction instructions[] = {
        {.op = IR_LABEL, .result = "L0"},
        {.op = IR_IF_FALSE, .arg1 = "cond",
         .result = "L1"},
        {.op = IR_GOTO, .result = "L0"},
        {.op = IR_LABEL, .result = "L1"},
        {.op = IR_RETURN, .arg1 = "0"}
    };

    IRProgram program = crear_programa(instructions, 5);
    BasicBlockCFG *cfg = construir_cfg(&program);

    bb_print(cfg);
    bb_print_cfg(cfg);

    printf("OK: funciones de impresion ejecutadas\n");

    bb_free(cfg);
}

// Funcion principal. Ejecuta todas las pruebas
int main(void) {

    printf("========================================\n");
    printf(" PRUEBAS DE BLOQUES BASICOS Y CFG\n");
    printf("========================================\n");

    test_sin_saltos();
    test_condicional();
    test_goto();
    test_ciclo();
    test_return();
    test_programa_vacio();
    test_etiqueta_inexistente();
    test_sin_conexion_duplicada();
    test_dos_funciones();
    test_impresion();

    printf("\n========================================\n");
    printf(" PRUEBAS COMPLETADAS: 10 / 10\n");
    printf("========================================\n");

    return 0;
}