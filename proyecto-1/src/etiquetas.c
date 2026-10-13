#include "etiquetas.h"

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Copia portable: no requiere strdup (que no forma parte de C11). */
static char *etq_copiar_cadena(const char *origen)
{
    size_t longitud;
    char *copia;

    if (origen == NULL) return NULL;
    longitud = strlen(origen) + 1u;
    copia = (char *)malloc(longitud);
    if (copia != NULL) memcpy(copia, origen, longitud);
    return copia;
}

static int etq_tipo_valido(TipoSalto tipo)
{
    return tipo == ETQ_JMP || tipo == ETQ_BEQ || tipo == ETQ_BNE ||
           tipo == ETQ_BGE || tipo == ETQ_BLE || tipo == ETQ_ADDI_PC;
}

static const char *etq_nombre_tipo(TipoSalto tipo)
{
    switch (tipo) {
        case ETQ_JMP: return "JMP";
        case ETQ_BEQ: return "BEQ";
        case ETQ_BNE: return "BNE";
        case ETQ_BGE: return "BGE";
        case ETQ_BLE: return "BLE";
        case ETQ_ADDI_PC: return "ADDI_PC";
        default: return "DESCONOCIDO";
    }
}

TablaEtiquetas *etq_crear(void)
{
    return (TablaEtiquetas *)calloc(1u, sizeof(TablaEtiquetas));
}

void etq_destruir(TablaEtiquetas *tabla)
{
    Etiqueta *etiqueta;
    ReferenciaSalto *referencia;
    if (tabla == NULL) return;

    etiqueta = tabla->primera_etiqueta;
    while (etiqueta != NULL) {
        Etiqueta *siguiente = etiqueta->siguiente;
        free(etiqueta->nombre);
        free(etiqueta);
        etiqueta = siguiente;
    }

    referencia = tabla->primera_referencia;
    while (referencia != NULL) {
        ReferenciaSalto *siguiente = referencia->siguiente;
        free(referencia->destino);
        free(referencia);
        referencia = siguiente;
    }
    free(tabla);
}

const Etiqueta *etq_buscar_etiqueta(const TablaEtiquetas *tabla,
                                   const char *nombre)
{
    const Etiqueta *actual;
    if (tabla == NULL || nombre == NULL) return NULL;
    for (actual = tabla->primera_etiqueta; actual != NULL;
         actual = actual->siguiente) {
        if (strcmp(actual->nombre, nombre) == 0) return actual;
    }
    return NULL;
}

const ReferenciaSalto *etq_buscar_salto(const TablaEtiquetas *tabla,
                                       size_t bundle_origen)
{
    const ReferenciaSalto *actual;
    if (tabla == NULL) return NULL;
    for (actual = tabla->primera_referencia; actual != NULL;
         actual = actual->siguiente) {
        if (actual->bundle_origen == bundle_origen) return actual;
    }
    return NULL;
}

ResultadoEtiquetas etq_generar_nombre(TablaEtiquetas *tabla,
                                      const char *prefijo,
                                      char *salida,
                                      size_t capacidad)
{
    int escritos;
    if (tabla == NULL || prefijo == NULL || prefijo[0] == '\0' ||
        salida == NULL || capacidad == 0u) return ETQ_ERROR_ARGUMENTO;

    if (tabla->siguiente_id == UINT_MAX) return ETQ_ERROR_ARGUMENTO;

    /* Si un usuario definio manualmente ese nombre, probar el siguiente. */
    while (tabla->siguiente_id < UINT_MAX) {
        escritos = snprintf(salida, capacidad, "L_%s_%u",
                            prefijo, tabla->siguiente_id);
        if (escritos < 0 || (size_t)escritos >= capacidad) {
            salida[0] = '\0';
            return ETQ_ERROR_ARGUMENTO;
        }
        tabla->siguiente_id++;
        if (etq_buscar_etiqueta(tabla, salida) == NULL) return ETQ_OK;
    }
    salida[0] = '\0';
    return ETQ_ERROR_ARGUMENTO;
}

ResultadoEtiquetas etq_definir(TablaEtiquetas *tabla, const char *nombre,
                               size_t indice_bundle)
{
    Etiqueta *nueva;
    if (tabla == NULL || nombre == NULL || nombre[0] == '\0')
        return ETQ_ERROR_ARGUMENTO;
    if (indice_bundle >= ETQ_MAX_BUNDLES) return ETQ_ERROR_BUNDLE_INVALIDO;
    if (etq_buscar_etiqueta(tabla, nombre) != NULL)
        return ETQ_ERROR_ETIQUETA_DUPLICADA;

    nueva = (Etiqueta *)calloc(1u, sizeof(Etiqueta));
    if (nueva == NULL) return ETQ_ERROR_MEMORIA;
    nueva->nombre = etq_copiar_cadena(nombre);
    if (nueva->nombre == NULL) {
        free(nueva);
        return ETQ_ERROR_MEMORIA;
    }
    nueva->indice_bundle = indice_bundle;
    nueva->direccion_bytes = (uint32_t)(indice_bundle * ETQ_BYTES_POR_BUNDLE);
    nueva->definida = 1;

    if (tabla->ultima_etiqueta != NULL)
        tabla->ultima_etiqueta->siguiente = nueva;
    else
        tabla->primera_etiqueta = nueva;
    tabla->ultima_etiqueta = nueva;
    return ETQ_OK;
}

ResultadoEtiquetas etq_agregar_salto(TablaEtiquetas *tabla,
                                     size_t bundle_origen,
                                     TipoSalto tipo,
                                     const char *etiqueta_destino)
{
    ReferenciaSalto *nueva;
    if (tabla == NULL || etiqueta_destino == NULL ||
        etiqueta_destino[0] == '\0' || !etq_tipo_valido(tipo))
        return ETQ_ERROR_ARGUMENTO;
    if (bundle_origen >= ETQ_MAX_BUNDLES) return ETQ_ERROR_BUNDLE_INVALIDO;
    /* Existe un unico slot BRU por bundle: un salto por indice. */
    if (etq_buscar_salto(tabla, bundle_origen) != NULL)
        return ETQ_ERROR_REFERENCIA_DUPLICADA;

    nueva = (ReferenciaSalto *)calloc(1u, sizeof(ReferenciaSalto));
    if (nueva == NULL) return ETQ_ERROR_MEMORIA;
    nueva->destino = etq_copiar_cadena(etiqueta_destino);
    if (nueva->destino == NULL) {
        free(nueva);
        return ETQ_ERROR_MEMORIA;
    }
    nueva->bundle_origen = bundle_origen;
    nueva->tipo = tipo;
    /* Offset y resuelta se mantienen en cero hasta etq_resolver. */

    if (tabla->ultima_referencia != NULL)
        tabla->ultima_referencia->siguiente = nueva;
    else
        tabla->primera_referencia = nueva;
    tabla->ultima_referencia = nueva;
    return ETQ_OK;
}

static int64_t etq_calcular_offset(size_t origen, size_t destino)
{
    return ((int64_t)destino - (int64_t)origen) *
           (int64_t)ETQ_BYTES_POR_BUNDLE;
}

ResultadoEtiquetas etq_resolver(TablaEtiquetas *tabla, size_t cantidad_bundles)
{
    const Etiqueta *etiqueta;
    const ReferenciaSalto *referencia;
    ReferenciaSalto *editable;

    if (tabla == NULL) return ETQ_ERROR_ARGUMENTO;

    /* Invalidar offsets anteriores: si cambian los bundles hay que recalcular. */
    for (editable = tabla->primera_referencia; editable != NULL;
         editable = editable->siguiente) {
        editable->resuelta = 0;
        editable->offset_bytes = 0;
    }

    if (cantidad_bundles == 0u || cantidad_bundles > ETQ_MAX_BUNDLES)
        return ETQ_ERROR_BUNDLE_INVALIDO;

    for (etiqueta = tabla->primera_etiqueta; etiqueta != NULL;
         etiqueta = etiqueta->siguiente) {
        if (!etiqueta->definida || etiqueta->indice_bundle >= cantidad_bundles)
            return ETQ_ERROR_BUNDLE_INVALIDO;
    }

    /* Comprobar todo antes de dar como resuelta ninguna referencia. */
    for (referencia = tabla->primera_referencia; referencia != NULL;
         referencia = referencia->siguiente) {
        const Etiqueta *destino;
        int64_t offset;
        int64_t minimo, maximo;

        if (referencia->bundle_origen >= cantidad_bundles)
            return ETQ_ERROR_BUNDLE_INVALIDO;
        destino = etq_buscar_etiqueta(tabla, referencia->destino);
        if (destino == NULL || !destino->definida)
            return ETQ_ERROR_ETIQUETA_NO_DEFINIDA;

        offset = etq_calcular_offset(referencia->bundle_origen,
                                    destino->indice_bundle);
        minimo = referencia->tipo == ETQ_JMP ? -524288LL : -2048LL;
        maximo = referencia->tipo == ETQ_JMP ? 524287LL : 2047LL;
        if (offset < minimo || offset > maximo)
            return ETQ_ERROR_OFFSET_FUERA_RANGO;
        /* Todos los indices enteros dan destinos alineados a 16 bytes. */
    }

    for (editable = tabla->primera_referencia; editable != NULL;
         editable = editable->siguiente) {
        const Etiqueta *destino =
            etq_buscar_etiqueta(tabla, editable->destino);
        editable->offset_bytes = (int32_t)etq_calcular_offset(
            editable->bundle_origen, destino->indice_bundle);
        editable->resuelta = 1;
    }
    return ETQ_OK;
}

void etq_imprimir(const TablaEtiquetas *tabla, FILE *salida)
{
    const Etiqueta *etiqueta;
    const ReferenciaSalto *referencia;
    if (salida == NULL) return;
    if (tabla == NULL) {
        fprintf(salida, "(tabla de etiquetas inexistente)\n");
        return;
    }
    fprintf(salida, "\nTABLA DE ETIQUETAS\n");
    fprintf(salida, "%-24s | %-8s | %-10s\n", "Nombre", "Bundle", "PC (bytes)");
    fprintf(salida, "-------------------------+----------+-----------\n");
    for (etiqueta = tabla->primera_etiqueta; etiqueta != NULL;
         etiqueta = etiqueta->siguiente) {
        fprintf(salida, "%-24s | %-8zu | 0x%04" PRIX32 "\n",
                etiqueta->nombre, etiqueta->indice_bundle,
                etiqueta->direccion_bytes);
    }

    fprintf(salida, "\nREFERENCIAS DE SALTO\n");
    fprintf(salida, "%-8s | %-6s | %-24s | %-13s\n",
            "Bundle", "Salto", "Destino", "Offset (bytes)");
    fprintf(salida, "---------+--------+--------------------------+--------------\n");
    for (referencia = tabla->primera_referencia; referencia != NULL;
         referencia = referencia->siguiente) {
        if (referencia->resuelta)
            fprintf(salida, "%-8zu | %-6s | %-24s | %" PRId32 "\n",
                    referencia->bundle_origen, etq_nombre_tipo(referencia->tipo),
                    referencia->destino, referencia->offset_bytes);
        else
            fprintf(salida, "%-8zu | %-6s | %-24s | pendiente\n",
                    referencia->bundle_origen, etq_nombre_tipo(referencia->tipo),
                    referencia->destino);
    }
}
