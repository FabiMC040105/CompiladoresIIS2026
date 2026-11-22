
#include "../ir.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    IRProgram programa;
    ResultadoDCE resultado = {0};

    // Inicializamos el programa IR
    ir_init(&programa);

    /*
    Creamos un programa con escrituras y lecturas
    de arreglos y matrices. Las escrituras deben conservarse porque modifican
    la memoria del programa
    Tambien incluimos una asignacion innecesaria
    para comprobar que DCE pueda eliminarla
    */

    // Declaramos un arreglo de cinco posiciones
    assert(ir_emit(
        &programa, IR_ARRAY_DECL,
        "5", NULL, "arreglo", 1
    ));

    // Guardamos el valor 25 en la primera posicion
    assert(ir_emit(
        &programa, IR_ARRAY_STORE,
        "0", "25", "arreglo", 2
    ));

    // Leemos el valor que guardamos en el arreglo
    assert(ir_emit(
        &programa, IR_ARRAY_LOAD,
        "arreglo", "0", "valor", 3
    ));

    // Declaramos una matriz de dos filas y dos columnas
    assert(ir_emit(
        &programa, IR_MATRIX_DECL,
        "2", "2", "matriz", 4
    ));

    // Guardamos el valor 40 en la posicion [0][1]
    assert(ir_emit_ex(
        &programa, IR_MATRIX_STORE,
        "0", "1", "matriz", "40", 5
    ));

    // Leemos el valor que guardamos en la matriz
    assert(ir_emit_ex(
        &programa, IR_MATRIX_LOAD,
        "matriz", "0", "otro_valor", "1", 6
    ));

    // Esta asignacion no se necesita
    assert(ir_emit(
        &programa, IR_COPY,
        "999", NULL, "sin_uso", 7
    ));

    // Sumamos los valores leidos
    assert(ir_emit(
        &programa, IR_ADD,
        "valor", "otro_valor", "resultado", 8
    ));

    // Devolvemos el resultado
    assert(ir_emit(
        &programa, IR_RETURN,
        "resultado", NULL, NULL, 9
    ));

    printf("\n===== IR ANTES DE DCE =====\n");
    ir_print(&programa, stdout);

    // Ejecutamos la eliminacion de codigo muerto
    assert(eliminar_codigo_muerto(&programa, &resultado));

    printf("\n===== IR DESPUES DE DCE =====\n");
    ir_print(&programa, stdout);

    mostrar_estadisticas_dce(&resultado);

    // Esperamos eliminar solamente sin_uso = 999
    assert(resultado.originales == 9);
    assert(resultado.eliminadas == 1);
    assert(resultado.restantes == 8);

    int encontro_escritura_arreglo = 0;
    int encontro_escritura_matriz = 0;
    int encontro_lectura_arreglo = 0;
    int encontro_lectura_matriz = 0;
    int encontro_codigo_muerto = 0;

    // Revisamos cuales operaciones quedaron despues de DCE
    for (size_t i = 0; i < programa.count; i++) {

        const IRInstruction *instruccion =
            &programa.instructions[i];

        if (instruccion->op == IR_ARRAY_STORE) {
            encontro_escritura_arreglo = 1;
        }

        if (instruccion->op == IR_MATRIX_STORE) {
            encontro_escritura_matriz = 1;
        }

        if (instruccion->op == IR_ARRAY_LOAD) {
            encontro_lectura_arreglo = 1;
        }

        if (instruccion->op == IR_MATRIX_LOAD) {
            encontro_lectura_matriz = 1;
        }

        // Buscamos si quedo alguna asignacion a sin_uso
        if (instruccion->result != NULL &&
            strcmp(instruccion->result, "sin_uso") == 0) {
            encontro_codigo_muerto = 1;
        }
    }

    // Las operaciones de memoria deben conservarse
    assert(encontro_escritura_arreglo == 1);
    assert(encontro_escritura_matriz == 1);
    assert(encontro_lectura_arreglo == 1);
    assert(encontro_lectura_matriz == 1);

    // La asignacion innecesaria debe desaparecer
    assert(encontro_codigo_muerto == 0);

    ir_free(&programa);

    printf("\nOK: DCE conserva operaciones de memoria.\n");

    return 0;
}
