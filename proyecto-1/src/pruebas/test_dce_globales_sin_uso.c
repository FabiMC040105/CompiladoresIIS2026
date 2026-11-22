#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Revisa si todavia existe una asignacion a la variable indicada
static int existe_asignacion(
    const IRProgram *programa,
    const char *nombre)
{
    // Recorremos las instrucciones que quedaron despues de DCE
    for (size_t i = 0; i < programa->count; i++) {

        const IRInstruction *instruccion =
            &programa->instructions[i];

        // Buscamos una copia que guarde un valor en esta variable
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

    // Esta variable si se utiliza desde otra funcion
    assert(ir_emit(&programa, IR_COPY, "0", NULL, "contador", 1));

    // Esta variable no se utiliza en ninguna funcion
    assert(ir_emit(&programa, IR_COPY, "100", NULL, "sin_uso", 2));

    // La funcion obtener necesita el valor de contador
    assert(ir_emit(&programa, IR_FUNC_BEGIN, NULL, NULL, "obtener", 3));
    assert(ir_emit(&programa, IR_RETURN, "contador", NULL, NULL, 4));
    assert(ir_emit(&programa, IR_FUNC_END, NULL, NULL, "obtener", 5));

    // La funcion principal no utiliza ninguna de las dos directamente
    assert(ir_emit(&programa, IR_FUNC_BEGIN, NULL, NULL, "principal", 6));
    assert(ir_emit(&programa, IR_RETURN, "0", NULL, NULL, 7));
    assert(ir_emit(&programa, IR_FUNC_END, NULL, NULL, "principal", 8));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    // Contador debe conservarse porque otra funcion lo necesita
    assert(existe_asignacion(&programa, "contador"));

    // Sin_uso debe eliminarse porque ninguna funcion lo necesita
    assert(!existe_asignacion(&programa, "sin_uso"));

    printf("\nOK: DCE conserva la variable utilizada y elimina la innecesaria.\n");

    ir_free(&programa);

    return 0;
}