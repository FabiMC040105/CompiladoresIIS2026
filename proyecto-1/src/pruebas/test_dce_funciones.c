
#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    IRProgram programa;
    ResultadoDCE resultado = {0};

    // Preparamos el programa IR
    ir_init(&programa);

    /*
    Creamos un programa con una llamada a una funcion.
    Aunque nadie utilice el resultado de la llamada,
    debemos conservarla porque podria tener efectos
    secundarios, como imprimir algo en pantalla
    */

    assert(ir_emit(
        &programa, IR_FUNC_BEGIN,
        NULL, NULL, "principal", 1
    ));

    // Esta asignacion no se utiliza y podria eliminarse
    assert(ir_emit(
        &programa, IR_COPY,
        "100", NULL, "%t0", 2
    ));

    // La llamada debe conservarse
    assert(ir_emit(
        &programa, IR_CALL,
        "imprimir", "0", NULL, 3
    ));

    assert(ir_emit(
        &programa, IR_RETURN,
        "0", NULL, NULL, 4
    ));

    assert(ir_emit(
        &programa, IR_FUNC_END,
        NULL, NULL, "principal", 5
    ));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    // Ejecutamos la eliminacion
    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    mostrar_estadisticas_dce(&resultado);

    /*
    Esperamos eliminar solamente la asignacion %t0 = 100
    La llamada, el retorno y los limites de la funcion
    deben permanecer en el programa
    */
    assert(programa.count == 4);
    assert(resultado.eliminadas == 1);

    // Comprobamos que la llamada siga existiendo
    assert(programa.instructions[1].op == IR_CALL);

    // Comprobamos que el retorno siga existiendo
    assert(programa.instructions[2].op == IR_RETURN);

    ir_free(&programa);

    printf("\nOK: DCE conserva las llamadas a funciones.\n");

    return 0;
}
