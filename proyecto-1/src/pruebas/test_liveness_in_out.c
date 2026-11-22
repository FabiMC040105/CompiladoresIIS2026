
#include "../ir.h"
#include "../basic_blocks.h"
#include "../liveness.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    // Creamos el programa IR que vamos a utilizar en la prueba
    IRProgram programa;

    // Aqui guardaremos los bloques y sus conexiones
    BasicBlockCFG *cfg;

    // Aqui guardaremos USE, DEF, IN y OUT de cada bloque
    AnalisisBloques analisis = {0};

    // Inicializamos el programa antes de agregar instrucciones
    ir_init(&programa);

    /*
    Creamos este programa:
    a = 10
    b = 20
    goto L1
    L1:
    c = a + b
    return c
    La idea es que a y b se definan en el primer bloque,
    pero se necesiten en el segundo bloque
    */

    assert(ir_emit(&programa, IR_COPY, "10", NULL, "a", 1));

    assert(ir_emit(&programa, IR_COPY, "20", NULL, "b", 2));

    // Saltamos a la etiqueta L1
    assert(ir_emit(&programa, IR_GOTO, NULL, NULL, "L1", 3));

    // Esta etiqueta marca el inicio del siguiente bloque
    assert(ir_emit(&programa, IR_LABEL, NULL, NULL, "L1", 4));

    // El segundo bloque utiliza a y b, que vienen del primero
    assert(ir_emit(&programa, IR_ADD, "a", "b", "c", 5));

    // Devolvemos el resultado de la suma
    assert(ir_emit(&programa, IR_RETURN, "c", NULL, NULL, 6));

    // Construimos los bloques basicos a partir del IR
    cfg = bb_build(&programa);
    assert(cfg != NULL);

    // Construimos las conexiones entre los bloques
    assert(bb_build_cfg(cfg));

    // Mostramos los bloques y las conexiones para revisarlos
    bb_print(cfg);
    bb_print_cfg(cfg);

    // Calculamos USE y DEF de cada bloque
    assert(analizar_use_def_bloques(cfg, &analisis));

    // Ahora calculamos IN y OUT utilizando las conexiones
    assert(calcular_in_out(cfg, &analisis));

    // Mostramos los cuatro conjuntos de cada bloque
    mostrar_in_out_bloques(cfg, &analisis);

    /*
    Comprobamos que el programa se haya dividido en dos bloques
    y que exista la conexion entre ellos
    */
    assert(analisis.cantidad_bloques == 2);
    assert(cfg->edge_count == 1);

    /*
    B1 necesita a y b desde antes de comenzar
    Por eso ambas variables deben estar en su IN
    */
    assert(contiene_variable(&analisis.bloques[1].in, "a"));
    assert(contiene_variable(&analisis.bloques[1].in, "b"));

    /*
    B0 debe conservar a y b al terminar porque B1 las necesita.
    Por eso ambas variables deben estar en el OUT de B0
    */
    assert(contiene_variable(&analisis.bloques[0].out, "a"));
    assert(contiene_variable(&analisis.bloques[0].out, "b"));

    // Tambien comprobamos que los conjuntos tengan el tamaño esperado
    assert(analisis.bloques[0].in.cantidad == 0);
    assert(analisis.bloques[0].out.cantidad == 2);
    assert(analisis.bloques[1].in.cantidad == 2);
    assert(analisis.bloques[1].out.cantidad == 0);

    // Liberamos la memoria utilizada por el analisis
    liberar_analisis_bloques(&analisis);

    // Liberamos los bloques y el programa IR
    bb_free(cfg);
    ir_free(&programa);

    printf("\nOK: IN y OUT funcionan con dos bloques conectados.\n");

    return 0;
}