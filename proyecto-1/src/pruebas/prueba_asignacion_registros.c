#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "basic_blocks.h"
#include "liveness.h"
#include "grafo_interferencia.h"
#include "asignacion_registros.h"

static int analizar(
    const IRProgram *p,
    GrafoInterferencia *g
)
{
    BasicBlockCFG *cfg = NULL;
    AnalisisBloques bloques = {0};
    DatosInstruccion *inst = NULL;
    int ok = 0;

    cfg = bb_build(p);

    if (cfg == NULL ||
        !bb_build_cfg(cfg))
        goto fin;

    if (!analizar_use_def_bloques(
            cfg,
            &bloques
        ) ||
        !calcular_in_out(
            cfg,
            &bloques
        ))
        goto fin;

    inst = (DatosInstruccion *)calloc(
        p->count,
        sizeof(DatosInstruccion)
    );

    if (inst == NULL)
        goto fin;

    if (!calcular_in_out_instrucciones(
            cfg,
            &bloques,
            inst
        ))
        goto fin;

    if (!gi_construir(
            p,
            inst,
            g
        ))
        goto fin;

    ok = 1;

fin:

    if (inst != NULL) {
        liberar_analisis_instrucciones(
            inst,
            p->count
        );

        free(inst);
    }

    liberar_analisis_bloques(&bloques);
    bb_free(cfg);

    return ok;
}

static void crear_presion(
    IRProgram *p,
    int cantidad
)
{
    char temp[32];
    int i;

    ir_init(p);

    for (i = 0; i < cantidad; i++) {

        snprintf(
            temp,
            sizeof(temp),
            "%%t%d",
            i
        );

        assert(
            ir_emit(
                p,
                IR_COPY,
                "1",
                NULL,
                temp,
                1
            )
        );
    }

    /*
     * Los temporales se usan despues de haber sido
     * definidos todos, obligando a mantenerlos vivos.
     */
    for (i = 0; i < cantidad; i++) {

        snprintf(
            temp,
            sizeof(temp),
            "%%t%d",
            i
        );

        assert(
            ir_emit(
                p,
                IR_PARAM,
                temp,
                NULL,
                NULL,
                2
            )
        );
    }
}

int main(void)
{
    IRProgram p;
    GrafoInterferencia g;
    AsignacionRegistros asignacion;

    size_t i;
    int spills_encontrados = 0;

    gi_init(&g);
    ar_init(&asignacion);

    crear_presion(&p, 10);

    assert(
        analizar(&p, &g)
    );

    assert(
        gi_colorear(&g, 9)
    );

    assert(
        ar_construir(
            &g,
            4,
            &asignacion
        )
    );

    assert(
        asignacion.cantidad_spills >= 1
    );

    for (i = 0;
         i < asignacion.cantidad;
         i++) {

        if (asignacion.entradas[i].es_spill) {

            spills_encontrados++;

            assert(
                asignacion.entradas[i].registro == -1
            );

        } else {

            assert(
                asignacion.entradas[i].registro >= 4
            );

            assert(
                asignacion.entradas[i].registro <= 12
            );
        }
    }

    assert(
        spills_encontrados ==
        (int)asignacion.cantidad_spills
    );

    printf(
        "PASS: el coloreo se traduce a registros x4-x12 y slots de spill\n"
    );

    ar_imprimir(
        &asignacion,
        stdout
    );

    ar_free(&asignacion);
    gi_free(&g);
    ir_free(&p);

    return 0;
}
