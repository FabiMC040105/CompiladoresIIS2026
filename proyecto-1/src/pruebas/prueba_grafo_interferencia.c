#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "basic_blocks.h"
#include "liveness.h"
#include "grafo_interferencia.h"

static int analizar(const IRProgram *p, GrafoInterferencia *g)
{
    BasicBlockCFG *cfg = NULL;
    AnalisisBloques bloques = {0};
    DatosInstruccion *inst = NULL;
    int ok = 0;

    cfg = bb_build(p);
    if (cfg == NULL || !bb_build_cfg(cfg))
        goto fin;

    if (!analizar_use_def_bloques(cfg, &bloques) ||
        !calcular_in_out(cfg, &bloques))
        goto fin;

    inst = (DatosInstruccion *)calloc(p->count, sizeof(DatosInstruccion));
    if (inst == NULL)
        goto fin;

    if (!calcular_in_out_instrucciones(cfg, &bloques, inst))
        goto fin;

    if (!gi_construir(p, inst, g))
        goto fin;

    ok = 1;

fin:
    if (inst != NULL) {
        liberar_analisis_instrucciones(inst, p->count);
        free(inst);
    }
    liberar_analisis_bloques(&bloques);
    bb_free(cfg);
    return ok;
}

static void crear_presion(IRProgram *p, int cantidad)
{
    char temp[32];
    int i;

    ir_init(p);

    for (i = 0; i < cantidad; i++) {
        snprintf(temp, sizeof(temp), "%%t%d", i);
        assert(ir_emit(p, IR_COPY, "1", NULL, temp, 1));
    }

    /* Cada temporal se usa despues de haber definido a todos. Por eso,
       inmediatamente despues de la ultima definicion, los "cantidad"
       temporales estan vivos al mismo tiempo y forman un clique. */
    for (i = 0; i < cantidad; i++) {
        snprintf(temp, sizeof(temp), "%%t%d", i);
        assert(ir_emit(p, IR_PARAM, temp, NULL, NULL, 2));
    }
}

int main(void)
{
    IRProgram p;
    GrafoInterferencia g;

    gi_init(&g);

    crear_presion(&p, 9);
    assert(analizar(&p, &g));

    printf("\nCASO 1: 9 temporales, 9 registros\n");
    gi_imprimir_grafo(&g, stdout);

    assert(gi_colorear(&g, 9));
    assert(gi_contar_spills(&g) == 0);
    printf("PASS: 9 temporales se colorean con 9 registros\n");
    gi_imprimir(&g, 4, stdout);
    assert(gi_exportar_dot(
        &g,
        4,
        "evidencia-vliw/grafo_9_temporales.dot"
    ));
    gi_free(&g);
    ir_free(&p);

    gi_init(&g);
    crear_presion(&p, 10);
    assert(analizar(&p, &g));

    printf("\nCASO 2: 10 temporales, 9 registros\n");
    gi_imprimir_grafo(&g, stdout);

    assert(gi_colorear(&g, 9));
    assert(gi_contar_spills(&g) >= 1);
    printf("PASS: 10 temporales simultaneamente vivos fuerzan spill\n");
    gi_imprimir(&g, 4, stdout);

    assert(gi_exportar_dot(
        &g,
        4,
        "evidencia-vliw/grafo_10_temporales_spill.dot"
    ));
    gi_free(&g);
    ir_free(&p);

    return 0;
}
