
#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    IRProgram programa;
    ResultadoDCE resultado = {0};

    // Inicializamos el programa donde guardaremos las instrucciones
    ir_init(&programa);

    // Empezamos el contador en cero y definimos el limite del ciclo
    assert(ir_emit(&programa, IR_COPY, "0", NULL, "contador", 1));
    assert(ir_emit(&programa, IR_COPY, "3", NULL, "limite", 2));

    // Esta etiqueta marca el inicio del ciclo
    assert(ir_emit(&programa, IR_LABEL, NULL, NULL, "@INICIO", 3));

    // Revisamos si el contador todavia es menor que el limite
    assert(ir_emit(&programa, IR_LT,
                   "contador", "limite", "condicion", 4));

    // Cuando la condicion es falsa, salimos del ciclo
    assert(ir_emit(&programa, IR_IF_FALSE,
                   "condicion", NULL, "@FIN", 5));

    // Esta asignacion no se utiliza y queremos que DCE la elimine
    assert(ir_emit(&programa, IR_COPY,
                   "999", NULL, "sin_uso", 6));

    // Aumentamos el contador para avanzar en el ciclo
    assert(ir_emit(&programa, IR_ADD,
                   "contador", "1", "contador", 7));

    // Volvemos al inicio para comprobar la condicion otra vez
    assert(ir_emit(&programa, IR_GOTO,
                   NULL, NULL, "@INICIO", 8));

    // Aqui continua el programa cuando termina el ciclo
    assert(ir_emit(&programa, IR_LABEL,
                   NULL, NULL, "@FIN", 9));

    // Devolvemos el valor final del contador
    assert(ir_emit(&programa, IR_RETURN,
                   "contador", NULL, NULL, 10));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    // Ejecutamos la eliminacion de codigo muerto
    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    mostrar_estadisticas_dce(&resultado);

    // Esperamos que solamente se elimine sin_uso = 999
    assert(resultado.originales == 10);
    assert(resultado.eliminadas == 1);
    assert(resultado.restantes == 9);

    /*
    Revisamos que la suma que actualiza el contador
    siga existiendo, porque el ciclo la necesita
    */
    int encontro_incremento = 0;
    int encontro_codigo_muerto = 0;
    int encontro_salto = 0;

    for (size_t i = 0; i < programa.count; i++) {

        const IRInstruction *instruccion =
            &programa.instructions[i];

        // Buscamos la suma que modifica el contador
        if (instruccion->op == IR_ADD &&
            instruccion->result != NULL &&
            strcmp(instruccion->result, "contador") == 0) {
            encontro_incremento = 1;
        }

        // Comprobamos que la asignacion innecesaria desaparecio
        if (instruccion->result != NULL &&
            strcmp(instruccion->result, "sin_uso") == 0) {
            encontro_codigo_muerto = 1;
        }

        // El salto hacia el inicio del ciclo debe conservarse
        if (instruccion->op == IR_GOTO) {
            encontro_salto = 1;
        }
    }

    assert(encontro_incremento == 1);
    assert(encontro_codigo_muerto == 0);
    assert(encontro_salto == 1);

    // Liberamos la memoria del programa
    ir_free(&programa);

    printf("\nOK: DCE conserva el ciclo y elimina codigo muerto.\n");

    return 0;
}
