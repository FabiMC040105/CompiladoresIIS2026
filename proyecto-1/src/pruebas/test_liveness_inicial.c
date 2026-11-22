#include "../ir.h"
#include "../basic_blocks.h"
#include "../liveness.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    IRProgram p;
    BasicBlockCFG *cfg;
    AnalisisBloques analisis = {0};
    ir_init(&p);

    /*
    El temporal %t0 debe reconocerse como variable, no como etiqueta
    */
    assert(ir_emit(&p, IR_COPY, "10", NULL, "a", 1));
    assert(ir_emit(&p, IR_ADD, "a", "5", "%t0", 2));
    assert(ir_emit(&p, IR_COPY, "%t0", NULL, "resultado", 3));
    assert(ir_emit(&p, IR_COPY, "50", NULL, "sin_uso", 4));
    assert(ir_emit(&p, IR_RETURN, "resultado", NULL, NULL, 5));

    cfg = bb_build(&p);
    assert(cfg != NULL);
    assert(bb_build_cfg(cfg));
    assert(analizar_use_def_bloques(cfg, &analisis));
    assert(analisis.cantidad_bloques == 1);
    assert(analisis.bloques[0].use.cantidad == 0);
    assert(contiene_variable(&analisis.bloques[0].def, "%t0"));
    assert(contiene_variable(&analisis.bloques[0].def, "sin_uso"));
    mostrar_use_def_instrucciones(&p);
    mostrar_use_def_bloques(cfg);
    /*
    Probamos el calculo de IN y OUT.
    Primero obtenemos USE y DEF de todos los bloques.
    Despues calculamos las variables vivas usando el CFG.
    */

    assert(analizar_use_def_bloques(cfg, &analisis));

    assert(calcular_in_out(cfg, &analisis));

    mostrar_in_out_bloques(cfg, &analisis);

    // Liberamos la memoria que utilizamos para guardar los conjuntos
    liberar_analisis_bloques(&analisis);
    liberar_analisis_bloques(&analisis);
    bb_free(cfg);
    ir_free(&p);
    printf("\nOK: USE y DEF reconocen temporales y conjuntos por bloque.\n");
    return 0;
}