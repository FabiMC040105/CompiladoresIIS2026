#include "etiquetas.h"

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*
    FUNCIONES AUXILIARES PARA ETIQUETAS Y SALTOS
    Esta seccion realiza las validaciones y conversiones necesarias para
    administrar los nombres de etiquetas y los tipos de instrucciones de salto.
*/
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

/*
    Verifica que el tipo de salto recibido pertenezca a los tipos reconocidos
    por la tabla de etiquetas. Retorna verdadero si es valido y falso en caso contrario.
*/
static int etq_tipo_valido(TipoSalto tipo)
{
    return tipo == ETQ_JMP || tipo == ETQ_BEQ || tipo == ETQ_BNE ||
           tipo == ETQ_BGE || tipo == ETQ_BLE || tipo == ETQ_ADDI_PC;
}

/*
    Convierte el tipo de salto a su nombre en texto para poder mostrarlo
    cuando se imprimen las referencias registradas en la tabla.
*/
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

/*
    Crea la tabla que almacenara las etiquetas y referencias a saltos.
    Reserva memoria e inicializa sus campos en cero para comenzar el registro.
*/
TablaEtiquetas *etq_crear(void)
{
    return (TablaEtiquetas *)calloc(1u, sizeof(TablaEtiquetas));
}

/*
    Libera la memoria utilizada por la tabla de etiquetas.
    Recorre por separado la lista de etiquetas y la lista de referencias,
    libera los nombres y destinos almacenados, y finalmente libera la tabla.
*/
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

/*
    Busca una etiqueta registrada a partir de su nombre.
    Recorre la lista enlazada y retorna la etiqueta si encuentra coincidencia,
    o NULL si no existe dentro de la tabla.
*/
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

/*
    Busca si ya existe una referencia de salto para el bundle indicado.
    Se utiliza para evitar registrar mas de un salto en un mismo slot BRU.
*/
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

/*
    Genera nombres unicos para las etiquetas internas del compilador.
    Combina un prefijo con el contador de la tabla y comprueba que el nombre
    no exista antes de devolverlo, evitando conflictos con etiquetas previas.
*/
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

/*
    Registra la definicion de una etiqueta y su posicion dentro del programa.
    Valida el nombre y el indice, evita etiquetas duplicadas y calcula su
    direccion en bytes a partir del numero de bundle donde fue definida.
*/
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
    /* Guarda la posicion del bundle y su direccion correspondiente en memoria. */
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

/*
    Agrega una referencia de salto que aun tiene pendiente su destino numerico.
    Guarda el bundle de origen, tipo de salto y nombre de la etiqueta destino;
    el offset se calculara posteriormente durante la resolucion de etiquetas.
*/
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

    /* Inserta la nueva referencia al final de la lista enlazada de saltos. */
    if (tabla->ultima_referencia != NULL)
        tabla->ultima_referencia->siguiente = nueva;
    else
        tabla->primera_referencia = nueva;
    tabla->ultima_referencia = nueva;
    return ETQ_OK;
}

/*
    Calcula la distancia relativa entre el bundle del salto y el bundle destino.
    Resta los indices y multiplica por los bytes de cada bundle VLIW,
    porque los desplazamientos de la ISA se expresan en bytes.
*/
static int64_t etq_calcular_offset(size_t origen, size_t destino)
{
    return ((int64_t)destino - (int64_t)origen) *
           (int64_t)ETQ_BYTES_POR_BUNDLE;
}

/*
    Resuelve las referencias a etiquetas una vez conocidas sus posiciones.
    Primero invalida resultados anteriores y verifica que los destinos existan,
    que los indices sean validos y que los offsets quepan en cada tipo de salto.
    Si todas las verificaciones pasan, asigna los desplazamientos definitivos.
*/
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

    /* Verifica que el programa tenga una cantidad permitida de bundles. */
    if (cantidad_bundles == 0u || cantidad_bundles > ETQ_MAX_BUNDLES)
        return ETQ_ERROR_BUNDLE_INVALIDO;

    /* Comprueba que las etiquetas definidas pertenezcan al programa generado. */
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
        /* JMP dispone de un rango de desplazamiento distinto al de los demas saltos. */
        minimo = referencia->tipo == ETQ_JMP ? -524288LL : -2048LL;
        maximo = referencia->tipo == ETQ_JMP ? 524287LL : 2047LL;
        if (offset < minimo || offset > maximo)
            return ETQ_ERROR_OFFSET_FUERA_RANGO;
        /* Todos los indices enteros dan destinos alineados a 16 bytes. */
    }

    /* Segunda pasada: guarda el offset definitivo y marca los saltos como resueltos. */
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

/*
    Imprime la informacion registrada en la tabla de etiquetas.
    Muestra cada etiqueta junto a su posicion y direccion en bytes, y luego
    presenta las referencias de salto con sus offsets o como pendientes.
*/
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
