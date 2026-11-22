#include "spill_transform.h"

#include "asignacion_registros.h"
#include "basic_blocks.h"
#include "dependencias.h"
#include "grafo_interferencia.h"
#include "liveness.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define SPILL_BASE "cdff_spill_area"
#define MAX_RONDAS_SPILL 128

static char **campo_mutable(IRInstruction *inst, unsigned campo)
{
    switch (campo) {
        case DEP_ARG1:
            return &inst->arg1;

        case DEP_ARG2:
            return &inst->arg2;

        case DEP_RESULT:
            return &inst->result;

        case DEP_EXTRA:
            return &inst->extra;

        default:
            return NULL;
    }
}

/*
 * Encuentra un numero de temporal que no colisione con los %tN
 * que ya existen en la IR.
 *
 * Esto permite que la transformacion funcione incluso en pruebas
 * construidas manualmente donde next_temp no fue actualizado.
 */
static size_t siguiente_temp_seguro(const IRProgram *programa)
{
    size_t siguiente;
    unsigned campos[] = {
        DEP_ARG1,
        DEP_ARG2,
        DEP_RESULT,
        DEP_EXTRA
    };

    if (programa == NULL)
        return 0;

    siguiente = programa->next_temp;

    for (size_t i = 0; i < programa->count; i++) {
        for (size_t k = 0; k < 4; k++) {
            const char *texto =
                dep_campo(&programa->instructions[i], campos[k]);

            const unsigned char *p;
            size_t numero = 0;

            if (texto == NULL ||
                texto[0] != '%' ||
                texto[1] != 't' ||
                !isdigit((unsigned char)texto[2]))
                continue;

            p = (const unsigned char *)(texto + 2);

            while (isdigit(*p)) {
                numero = numero * 10u +
                         (size_t)(*p - '0');
                p++;
            }

            if (*p == '\0' &&
                numero + 1 > siguiente)
                siguiente = numero + 1;
        }
    }

    return siguiente;
}

static int analizar_grafo(
    const IRProgram *programa,
    GrafoInterferencia *grafo
)
{
    BasicBlockCFG *cfg = NULL;
    AnalisisBloques bloques = {0};
    DatosInstruccion *instrucciones = NULL;
    int ok = 0;

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
        (DatosInstruccion *)calloc(
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
            grafo
        ))
        goto fin;

    ok = 1;

fin:

    if (instrucciones != NULL) {
        liberar_analisis_instrucciones(
            instrucciones,
            programa->count
        );

        free(instrucciones);
    }

    liberar_analisis_bloques(&bloques);
    bb_free(cfg);

    return ok;
}

static void liberar_reloads(char **reloads)
{
    size_t i;

    for (i = 0; i < 4; i++) {
        free(reloads[i]);
        reloads[i] = NULL;
    }
}

/*
 * Reescribe una ronda de spilling.
 *
 * Para cada lectura de un valor spilled:
 *
 *     LOAD nuevo_temp, spill[slot]
 *
 * antes de utilizarlo.
 *
 * Para cada definicion de un valor spilled:
 *
 *     STORE spill[slot], temporal
 *
 * inmediatamente despues de producirlo.
 *
 * En IR se reutilizan IR_ARRAY_LOAD e IR_ARRAY_STORE para no agregar
 * nuevos IROp incompatibles con las otras optimizaciones.
 */
static int reescribir_spills(
    IRProgram *programa,
    const AsignacionRegistros *asignacion,
    size_t slot_base,
    ResultadoSpill *resultado
)
{
    IRProgram nuevo;

    unsigned campos[] = {
        DEP_ARG1,
        DEP_ARG2,
        DEP_RESULT,
        DEP_EXTRA
    };

    ir_init(&nuevo);

    nuevo.next_temp =
        siguiente_temp_seguro(programa);

    nuevo.next_label =
        programa->next_label;

    for (size_t i = 0;
         i < programa->count;
         i++) {

        const IRInstruction *original =
            &programa->instructions[i];

        IRInstruction modificada =
            *original;

        DepAccesos accesos;

        char *reloads[4] = {
            NULL,
            NULL,
            NULL,
            NULL
        };

        if (!dep_accesos(
                original,
                &accesos
            ))
            goto error;

        /*
         * LOAD antes de cada lectura de un temporal
         * que fue seleccionado para spill.
         */
        for (size_t k = 0; k < 4; k++) {

            unsigned campo = campos[k];

            const char *nombre;

            const AsignacionTemporal *entrada;

            char slot[32];

            char *temp_reload;

            char **destino;

            if (!(accesos.lecturas & campo))
                continue;

            nombre =
                dep_campo(original, campo);

            entrada =
                ar_buscar(
                    asignacion,
                    nombre
                );

            if (entrada == NULL ||
                !entrada->es_spill)
                continue;

            temp_reload =
                ir_new_temp(&nuevo);

            if (temp_reload == NULL) {
                liberar_reloads(reloads);
                goto error;
            }

            snprintf(
                slot,
                sizeof(slot),
                "%zu",
                slot_base +
                entrada->slot_spill
            );

            /*
             * IR_ARRAY_LOAD:
             *
             * result = arg1[arg2]
             *
             * %tN = cdff_spill_area[slot]
             */
            if (!ir_emit(
                    &nuevo,
                    IR_ARRAY_LOAD,
                    SPILL_BASE,
                    slot,
                    temp_reload,
                    original->source_line
                )) {

                free(temp_reload);
                liberar_reloads(reloads);
                goto error;
            }

            destino =
                campo_mutable(
                    &modificada,
                    campo
                );

            if (destino == NULL) {
                free(temp_reload);
                liberar_reloads(reloads);
                goto error;
            }

            *destino = temp_reload;

            reloads[k] =
                temp_reload;

            resultado->loads++;
        }

        /*
         * Emitir la instruccion original utilizando
         * los reloads que se hayan insertado.
         */
        if (!ir_emit_ex(
                &nuevo,
                modificada.op,
                modificada.arg1,
                modificada.arg2,
                modificada.result,
                modificada.extra,
                modificada.source_line
            )) {

            liberar_reloads(reloads);
            goto error;
        }

        liberar_reloads(reloads);

        /*
         * STORE inmediatamente despues de producir
         * un temporal seleccionado para spill.
         */
        for (size_t k = 0; k < 4; k++) {

            unsigned campo = campos[k];

            const char *nombre;

            const AsignacionTemporal *entrada;

            char slot[32];

            if (!(accesos.escrituras & campo))
                continue;

            nombre =
                dep_campo(original, campo);

            entrada =
                ar_buscar(
                    asignacion,
                    nombre
                );

            if (entrada == NULL ||
                !entrada->es_spill)
                continue;

            snprintf(
                slot,
                sizeof(slot),
                "%zu",
                slot_base +
                entrada->slot_spill
            );

            /*
             * IR_ARRAY_STORE:
             *
             * result[arg1] = arg2
             *
             * cdff_spill_area[slot] = %tN
             */
            if (!ir_emit(
                    &nuevo,
                    IR_ARRAY_STORE,
                    slot,
                    nombre,
                    SPILL_BASE,
                    original->source_line
                ))
                goto error;

            resultado->stores++;
        }
    }

    ir_free(programa);
    *programa = nuevo;

    return 1;

error:

    ir_free(&nuevo);
    return 0;
}

int spill_resolver_programa(
    IRProgram *programa,
    int cantidad_colores,
    int registro_base,
    ResultadoSpill *resultado
)
{
    GrafoInterferencia grafo;
    AsignacionRegistros asignacion;

    ResultadoSpill total = {0};

    size_t slot_base = 0;

    if (programa == NULL ||
        resultado == NULL ||
        cantidad_colores <= 0)
        return 0;

    gi_init(&grafo);
    ar_init(&asignacion);

    /*
     * Una sola ronda no siempre basta.
     *
     * Al introducir un reload aparece un temporal nuevo,
     * por lo que se vuelve a calcular liveness y se colorea
     * nuevamente hasta eliminar toda presion superior a K.
     */
    for (size_t ronda = 0;
         ronda < MAX_RONDAS_SPILL;
         ronda++) {

        size_t cantidad_spills;

        ResultadoSpill paso = {0};

        gi_free(&grafo);
        gi_init(&grafo);

        ar_free(&asignacion);
        ar_init(&asignacion);

        if (!analizar_grafo(
                programa,
                &grafo
            ) ||
            !gi_colorear(
                &grafo,
                cantidad_colores
            ))
            goto error;

        cantidad_spills =
            gi_contar_spills(&grafo);

        /*
         * El proceso termina solamente cuando
         * todo el grafo puede recibir un color.
         */
        if (cantidad_spills == 0) {

            total.slots =
                slot_base;

            *resultado =
                total;

            gi_free(&grafo);
            ar_free(&asignacion);

            return 1;
        }

        if (!ar_construir(
                &grafo,
                registro_base,
                &asignacion
            ))
            goto error;

        if (!reescribir_spills(
                programa,
                &asignacion,
                slot_base,
                &paso
            ))
            goto error;

        /*
         * Cada nueva ronda utiliza slots diferentes
         * para evitar sobrescribir spills anteriores.
         */
        slot_base +=
            asignacion.cantidad_spills;

        total.rondas++;

        total.stores +=
            paso.stores;

        total.loads +=
            paso.loads;
    }

error:

    gi_free(&grafo);
    ar_free(&asignacion);

    return 0;
}
