#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Revisamos si una asignacion sigue existiendo despues de optimizar
static int existe_asignacion(
    const IRProgram *programa,
    const char *nombre)
{
    // Recorremos las instrucciones que quedaron en el IR
    for (size_t i = 0; i < programa->count; i++) {

        const IRInstruction *instruccion =
            &programa->instructions[i];

        // Buscamos una copia que guarde un valor en la variable indicada
        if (instruccion->op == IR_COPY &&
            instruccion->result != NULL &&
            strcmp(instruccion->result, nombre) == 0) {
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

    // Esta variable se inicializa fuera de las funciones
    assert(ir_emit(&programa, IR_COPY, "0", NULL, "contador", 1));

    // Esta funcion necesita leer el valor de contador
    assert(ir_emit(&programa, IR_FUNC_BEGIN, NULL, NULL, "obtener", 2));
    assert(ir_emit(&programa, IR_RETURN, "contador", NULL, NULL, 3));
    assert(ir_emit(&programa, IR_FUNC_END, NULL, NULL, "obtener", 4));

    // La funcion principal no utiliza contador directamente
    assert(ir_emit(&programa, IR_FUNC_BEGIN, NULL, NULL, "principal", 5));
    assert(ir_emit(&programa, IR_RETURN, "0", NULL, NULL, 6));
    assert(ir_emit(&programa, IR_FUNC_END, NULL, NULL, "principal", 7));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    // Esta prueba debe fallar si DCE elimina contador incorrectamente
    assert(existe_asignacion(&programa, "contador"));

    printf("\nOK: se conserva la asignacion utilizada por otra funcion.\n");

    ir_free(&programa);
    return 0;
}