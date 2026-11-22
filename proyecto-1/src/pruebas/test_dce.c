
#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    IRProgram programa;
    ResultadoDCE resultado = {0};

    ir_init(&programa);

    /*
    Creamos un programa donde dos instrucciones
    pueden eliminarse en diferentes pasadas
    a = 10
    b = a + 5
    c = 30
    return c
    Primero se elimina b = a + 5.
    Despues a = 10 tambien queda innecesaria
    */
    assert(ir_emit(&programa, IR_COPY, "10", NULL, "a", 1));
    assert(ir_emit(&programa, IR_ADD, "a", "5", "b", 2));
    assert(ir_emit(&programa, IR_COPY, "30", NULL, "c", 3));
    assert(ir_emit(&programa, IR_RETURN, "c", NULL, NULL, 4));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    // Ejecutamos la eliminacion real.
    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    mostrar_estadisticas_dce(&resultado);

    // Comprobamos que se eliminaron dos instrucciones
    assert(resultado.originales == 4);
    assert(resultado.eliminadas == 2);
    assert(resultado.restantes == 2);

    // El programa debe conservar solamente c = 30 y return c
    assert(programa.count == 2);
    assert(programa.instructions[0].op == IR_COPY);
    assert(programa.instructions[1].op == IR_RETURN);

    ir_free(&programa);

    printf("\nOK: DCE elimino instrucciones innecesarias.\n");

    return 0;
}
