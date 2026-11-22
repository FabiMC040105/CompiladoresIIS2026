#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "basic_blocks.h"
#include "liveness.h"
#include "grafo_interferencia.h"
#include "spill_transform.h"

static int verificar_sin_spills(
    const IRProgram *programa
)
{
    BasicBlockCFG *cfg = NULL;

    AnalisisBloques bloques = {0};

    DatosInstruccion *instrucciones =
        NULL;

    GrafoInterferencia grafo;

    int ok = 0;

    gi_init(&grafo);

    cfg = bb_build(programa);

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

    instrucciones =
        calloc(
            programa->count,
            sizeof(*instrucciones)
        );

    if (programa->count > 0 &&
        instrucciones == NULL)
        goto fin;

    if (!calcular_in_out_instrucciones(
            cfg,
            &bloques,
            instrucciones
        ))
        goto fin;

    if (!gi_construir(
            programa,
            instrucciones,
            &grafo
        ))
        goto fin;

    if (!gi_colorear(
            &grafo,
            9
        ))
        goto fin;

    ok =
        gi_contar_spills(&grafo) == 0;

fin:

    if (instrucciones != NULL) {

        liberar_analisis_instrucciones(
            instrucciones,
            programa->count
        );

        free(instrucciones);
    }

    liberar_analisis_bloques(
        &bloques
    );

    bb_free(cfg);

    gi_free(&grafo);

    return ok;
}

int main(void)
{
    IRProgram programa;

    ResultadoSpill resultado;

    char temp[32];

    ir_init(&programa);

    /*
     * Crear 10 temporales que permanecen vivos
     * simultaneamente.
     *
     * Solo existen 9 registros fisicos:
     * x4 ... x12.
     */
    for (int i = 0; i < 10; i++) {

        snprintf(
            temp,
            sizeof(temp),
            "%%t%d",
            i
        );

        assert(
            ir_emit(
                &programa,
                IR_COPY,
                "1",
                NULL,
                temp,
                1
            )
        );
    }

    /*
     * Utilizarlos despues de haber definido todos
     * fuerza la interferencia entre ellos.
     */
    for (int i = 0; i < 10; i++) {

        snprintf(
            temp,
            sizeof(temp),
            "%%t%d",
            i
        );

        assert(
            ir_emit(
                &programa,
                IR_PARAM,
                temp,
                NULL,
                NULL,
                2
            )
        );
    }

    assert(
        spill_resolver_programa(
            &programa,
            9,
            4,
            &resultado
        )
    );

    assert(resultado.rondas > 0);
    assert(resultado.stores > 0);
    assert(resultado.loads > 0);
    assert(resultado.slots > 0);

    /*
     * Despues de insertar los spills y recalcular
     * liveness, todo debe ser coloreable.
     */
    assert(
        verificar_sin_spills(
            &programa
        )
    );

    printf(
        "PASS: spilling resolvio la presion de registros\n"
    );

    printf(
        "Rondas de spilling: %zu\n",
        resultado.rondas
    );

    printf(
        "STORE insertados: %zu\n",
        resultado.stores
    );

    printf(
        "LOAD insertados: %zu\n",
        resultado.loads
    );

    printf(
        "Slots utilizados: %zu\n",
        resultado.slots
    );

    printf(
        "\n===== IR DESPUES DE SPILLING =====\n"
    );

    ir_print(
        &programa,
        stdout
    );

    ir_free(&programa);

    return 0;
}
