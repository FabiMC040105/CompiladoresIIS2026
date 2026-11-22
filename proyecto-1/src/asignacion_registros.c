#include "asignacion_registros.h"

#include <stdlib.h>
#include <string.h>

static char *duplicar_cadena(const char *s)
{
    size_t n;
    char *copia;

    if (s == NULL)
        return NULL;

    n = strlen(s) + 1;

    copia = (char *)malloc(n);
    if (copia != NULL)
        memcpy(copia, s, n);

    return copia;
}

void ar_init(AsignacionRegistros *asignacion)
{
    if (asignacion != NULL)
        *asignacion = (AsignacionRegistros){0};
}

void ar_free(AsignacionRegistros *asignacion)
{
    size_t i;

    if (asignacion == NULL)
        return;

    for (i = 0; i < asignacion->cantidad; i++)
        free(asignacion->entradas[i].temporal);

    free(asignacion->entradas);

    *asignacion = (AsignacionRegistros){0};
}

int ar_construir(
    const GrafoInterferencia *grafo,
    int registro_base,
    AsignacionRegistros *asignacion
)
{
    size_t i;
    size_t siguiente_slot = 0;

    if (grafo == NULL ||
        asignacion == NULL ||
        registro_base < 0)
        return 0;

    ar_free(asignacion);
    ar_init(asignacion);

    if (grafo->cantidad == 0)
        return 1;

    asignacion->entradas =
        (AsignacionTemporal *)calloc(
            grafo->cantidad,
            sizeof(AsignacionTemporal)
        );

    if (asignacion->entradas == NULL)
        return 0;

    asignacion->cantidad = grafo->cantidad;

    for (i = 0; i < grafo->cantidad; i++) {
        AsignacionTemporal *entrada =
            &asignacion->entradas[i];

        const NodoInterferencia *nodo =
            &grafo->nodos[i];

        entrada->temporal =
            duplicar_cadena(nodo->nombre);

        if (entrada->temporal == NULL) {
            ar_free(asignacion);
            return 0;
        }

        if (nodo->spill || nodo->color < 0) {
            entrada->registro = -1;
            entrada->es_spill = 1;
            entrada->slot_spill = siguiente_slot++;

        } else {
            entrada->registro =
                registro_base + nodo->color;

            entrada->es_spill = 0;
            entrada->slot_spill = 0;
        }
    }

    asignacion->cantidad_spills =
        siguiente_slot;

    return 1;
}

const AsignacionTemporal *ar_buscar(
    const AsignacionRegistros *asignacion,
    const char *temporal
)
{
    size_t i;

    if (asignacion == NULL ||
        temporal == NULL)
        return NULL;

    for (i = 0; i < asignacion->cantidad; i++) {
        if (strcmp(
                asignacion->entradas[i].temporal,
                temporal
            ) == 0)
            return &asignacion->entradas[i];
    }

    return NULL;
}

void ar_imprimir(
    const AsignacionRegistros *asignacion,
    FILE *salida
)
{
    size_t i;

    if (asignacion == NULL ||
        salida == NULL)
        return;

    fprintf(
        salida,
        "\n===== ASIGNACION DE REGISTROS Y SPILLS =====\n"
    );

    for (i = 0; i < asignacion->cantidad; i++) {

        const AsignacionTemporal *entrada =
            &asignacion->entradas[i];

        if (entrada->es_spill) {
            fprintf(
                salida,
                "%s -> spill slot %zu\n",
                entrada->temporal,
                entrada->slot_spill
            );

        } else {
            fprintf(
                salida,
                "%s -> x%d\n",
                entrada->temporal,
                entrada->registro
            );
        }
    }
}
