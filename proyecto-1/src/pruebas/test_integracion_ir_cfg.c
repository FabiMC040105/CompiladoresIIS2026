
#include "../ir.h"
#include "../basic_blocks.h"

#include <assert.h>
#include <stdio.h>

/*
PRUEBA DE INTEGRACION ENTRE IR Y BLOQUES BASICOS

Objetivos:
1. Generar instrucciones IR
2. Construir bloques basicos
3. Construir el CFG
4. Comprobar las conexiones
5. Imprimir los resultados
*/

/*
Comprueba si existe una conexion entre dos bloques del CFG
Retorna 1 si existe y 0 si no existe
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
Prueba principal de integracion.
Representamos este ciclo:
L0:
     if_false cond goto L1
     i = i + 1
     goto L0
     L1:
     return i
*/
static void test_integracion_ir_cfg(void)
{
    IRProgram program;
    BasicBlockCFG *cfg = NULL;

    // Inicializamos el programa IR.
    ir_init(&program);

    // Generamos instrucciones usando ir_emit()
    int ok =
        ir_emit(&program, IR_LABEL,
                NULL, NULL, "L0", 1) &&

        ir_emit(&program, IR_IF_FALSE,
                "cond", NULL, "L1", 2) &&

        ir_emit(&program, IR_ADD,
                "i", "1", "i", 3) &&

        ir_emit(&program, IR_GOTO,
                NULL, NULL, "L0", 4) &&

        ir_emit(&program, IR_LABEL,
                NULL, NULL, "L1", 5) &&

        ir_emit(&program, IR_RETURN,
                "i", NULL, NULL, 6);

    // Comprobamos que las instrucciones se crearon
    assert(ok == 1);
    assert(program.count == 6);

    printf("\n========================================\n");
    printf(" REPRESENTACION INTERMEDIA REAL\n");
    printf("========================================\n");

    // Imprimimos el IR generado por ir.c
    ir_print(&program, stdout);

    // Construimos los bloques a partir del IR
    cfg = bb_build(&program);

    assert(cfg != NULL);

    // Construimos las conexiones del CFG
    assert(bb_build_cfg(cfg) == 1);

    // Comprobamos la cantidad de bloques
    assert(cfg->block_count == 3);

    // Comprobamos la cantidad de conexiones
    assert(cfg->edge_count == 3);

    // B0 puede continuar hacia B1
    assert(existe_conexion(cfg, 0, 1));

    // B0 puede saltar hacia B2
    assert(existe_conexion(cfg, 0, 2));

    // B1 regresa a B0 para repetir el ciclo
    assert(existe_conexion(cfg, 1, 0));

    // Mostramos los bloques basicos
    bb_print(cfg);

    // Mostramos las conexiones del CFG
    bb_print_cfg(cfg);

    printf("\nOK: integracion IR y bloques basicos\n");

    // Liberamos primero el CFG
    bb_free(cfg);

    // Liberamos las instrucciones del IR
    ir_free(&program);
}

//Ejecuta todas las pruebas de integracion
int main(void)
{
    printf("========================================\n");
    printf(" PRUEBAS DE INTEGRACION IR - CFG\n");
    printf("========================================\n");

    test_integracion_ir_cfg();

    printf("\n========================================\n");
    printf(" PRUEBA DE INTEGRACION COMPLETADA\n");
    printf("========================================\n");

    return 0;
}