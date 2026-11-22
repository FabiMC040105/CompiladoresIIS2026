
#include "../ir.h"
#include "../basic_blocks.h"
#include "../liveness.h"
#include "../dead_code_elimination.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    IRProgram programa;
    BasicBlockCFG *cfg;
    AnalisisBloques analisis = {0};
    DatosInstruccion *resultados;
    EstadisticasDCE estadisticas = {0}; // Aqui guardaremos las estadisticas del analisis DCE

    // Preparamos el programa IR
    ir_init(&programa);

    /*
    Creamos un programa sencillo:
    a = 10
    b = 20
    c = a + b
    d = 50
    return c
    Queremos comprobar que d no esta viva
    despues de recibir el valor 50
    */

    assert(ir_emit(&programa, IR_COPY, "10", NULL, "a", 1));
    assert(ir_emit(&programa, IR_COPY, "20", NULL, "b", 2));
    assert(ir_emit(&programa, IR_ADD, "a", "b", "c", 3));
    assert(ir_emit(&programa, IR_COPY, "50", NULL, "d", 4));
    assert(ir_emit(&programa, IR_RETURN, "c", NULL, NULL, 5));

    // Construimos los bloques y sus conexiones
    cfg = bb_build(&programa);
    assert(cfg != NULL);
    assert(bb_build_cfg(cfg));

    // Calculamos USE y DEF por bloque
    assert(analizar_use_def_bloques(cfg, &analisis));

    // Calculamos IN y OUT por bloque
    assert(calcular_in_out(cfg, &analisis));

    /*
    Reservamos un espacio por cada instruccion.
    calloc deja los conjuntos vacios para que podamos
    agregar variables sin utilizar datos anteriores
    */
    resultados = calloc(programa.count, sizeof(DatosInstruccion));
    assert(resultados != NULL);

    // Calculamos IN y OUT por instruccion
    assert(calcular_in_out_instrucciones(
        cfg, &analisis, resultados));

    // Mostramos los resultados
    mostrar_in_out_instrucciones(&programa, resultados);

    /*
    Ahora que conocemos las variables vivas de cada instruccion,
    revisamos cuales asignaciones tienen resultados que no se usan
    */
    assert(buscar_codigo_muerto(
        &programa,
        resultados,
        &estadisticas
    ));

    // En este ejemplo esperamos encontrar d = 50
    assert(estadisticas.posibles_eliminaciones == 1);

    /*
    Comprobamos que a y b se necesitan antes
    de ejecutar la suma
    */
    assert(contiene_variable(&resultados[2].in, "a"));
    assert(contiene_variable(&resultados[2].in, "b"));

    /*
    Comprobamos que c se necesita despues
    de la suma porque luego hacemos return c
    */
    assert(contiene_variable(&resultados[2].out, "c"));

    /*
    Comprobamos que d no se necesita despues
    de su asignacion
    */
    assert(!contiene_variable(&resultados[3].out, "d"));

    // Liberamos los conjuntos de cada instruccion
    liberar_analisis_instrucciones(resultados, programa.count);

    // Liberamos el arreglo que reservamos con calloc
    free(resultados);

    // Liberamos el resto de las estructuras
    liberar_analisis_bloques(&analisis);
    bb_free(cfg);
    ir_free(&programa);

    printf("\nOK: IN y OUT por instruccion funcionan.\n");

    return 0;
}