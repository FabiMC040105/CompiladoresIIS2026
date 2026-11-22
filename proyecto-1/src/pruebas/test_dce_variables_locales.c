#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Revisamos si una funcion conserva una asignacion a la variable indicada
static int existe_asignacion_en_funcion(
    const IRProgram *programa,
    const char *nombre_funcion,
    const char *nombre_variable)
{
    int dentro_funcion = 0;

    // Recorremos todas las instrucciones del programa
    for (size_t i = 0; i < programa->count; i++) {

        const IRInstruction *instruccion =
            &programa->instructions[i];

        // Comprobamos si encontramos el inicio de la funcion buscada
        if (instruccion->op == IR_FUNC_BEGIN) {
            dentro_funcion =
                instruccion->result != NULL &&
                strcmp(instruccion->result, nombre_funcion) == 0;

            continue;
        }

        // Al terminar una funcion dejamos de revisar su contenido
        if (instruccion->op == IR_FUNC_END) {
            dentro_funcion = 0;
            continue;
        }

        // Solo buscamos asignaciones dentro de la funcion indicada
        if (!dentro_funcion) {
            continue;
        }

        // Comprobamos si existe una asignacion a la variable buscada
        if (instruccion->op == IR_COPY &&
            instruccion->result != NULL &&
            strcmp(instruccion->result, nombre_variable) == 0) {
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    IRProgram programa;
    ResultadoDCE resultado = {0};

    ir_init(&programa);

    // En primera, dato recibe un valor que nunca se utiliza
    assert(ir_emit(&programa, IR_FUNC_BEGIN, NULL, NULL, "primera", 1));
    // Declaramos dato dentro de primera
    assert(ir_emit(&programa, IR_DECL, NULL, NULL, "dato", 1));
    assert(ir_emit(&programa, IR_COPY, "50", NULL, "dato", 2));
    assert(ir_emit(&programa, IR_RETURN, "0", NULL, NULL, 3));
    assert(ir_emit(&programa, IR_FUNC_END, NULL, NULL, "primera", 4));

    // En segunda, dato si se utiliza para devolver un resultado
    assert(ir_emit(&programa, IR_FUNC_BEGIN, NULL, NULL, "segunda", 5));
    // Declaramos otra variable dato, pero dentro de segunda
    assert(ir_emit(&programa, IR_DECL, NULL, NULL, "dato", 5));
    assert(ir_emit(&programa, IR_COPY, "20", NULL, "dato", 6));
    assert(ir_emit(&programa, IR_RETURN, "dato", NULL, NULL, 7));
    assert(ir_emit(&programa, IR_FUNC_END, NULL, NULL, "segunda", 8));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    // La asignacion de primera debe eliminarse
    assert(!existe_asignacion_en_funcion(&programa, "primera", "dato"));

    // La asignacion de segunda debe conservarse
    assert(existe_asignacion_en_funcion(&programa, "segunda", "dato"));

    printf("\nOK: DCE distingue los usos de variables locales entre funciones.\n");

    ir_free(&programa);

    return 0;
}