#ifndef ETIQUETAS_H
#define ETIQUETAS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Restricciones de la ISA VLIW: direcciones expresadas en bytes. */
#define ETQ_BYTES_POR_BUNDLE 16u
#define ETQ_MEMORIA_CODIGO_BYTES 65536u
#define ETQ_MAX_BUNDLES (ETQ_MEMORIA_CODIGO_BYTES / ETQ_BYTES_POR_BUNDLE)

/* Instrucciones de salto del slot BRU. */
typedef enum {
    ETQ_JMP,
    ETQ_BEQ,
    ETQ_BNE,
    ETQ_BGE,
    ETQ_BLE,
    ETQ_ADDI_PC /* ADDI rd, x1, etiqueta: referencia PC relativa de 12 bits */
} TipoSalto;

/* Resultado de las operaciones de este modulo. */
typedef enum {
    ETQ_OK,
    ETQ_ERROR_ARGUMENTO,
    ETQ_ERROR_MEMORIA,
    ETQ_ERROR_ETIQUETA_DUPLICADA,
    ETQ_ERROR_ETIQUETA_NO_DEFINIDA,
    ETQ_ERROR_REFERENCIA_DUPLICADA,
    ETQ_ERROR_BUNDLE_INVALIDO,
    ETQ_ERROR_OFFSET_FUERA_RANGO
} ResultadoEtiquetas;

typedef struct Etiqueta Etiqueta;
typedef struct ReferenciaSalto ReferenciaSalto;

/* Una etiqueta marca el comienzo de un bundle de codigo. */
struct Etiqueta {
    char *nombre;               /* Cadena propia, liberada por etq_destruir. */
    size_t indice_bundle;       /* Indice definitivo: 0, 1, 2, ... */
    uint32_t direccion_bytes;   /* indice_bundle * 16, tras resolver. */
    int definida;               /* La etiqueta tiene una posicion conocida. */
    Etiqueta *siguiente;
};

/* Una referencia representa un salto BRU con destino simbolico. */
struct ReferenciaSalto {
    char *destino;              /* Nombre de la etiqueta de destino. */
    size_t bundle_origen;       /* Bundle que contiene la instruccion BRU. */
    TipoSalto tipo;             /* JMP, BEQ, BNE, BGE o BLE. */
    int32_t offset_bytes;       /* Destino - PC del bundle de origen. */
    int resuelta;               /* El offset fue validado y calculado. */
    ReferenciaSalto *siguiente;
};

/* Dos listas planas: definiciones de etiquetas y referencias de saltos. */
typedef struct {
    Etiqueta *primera_etiqueta;
    Etiqueta *ultima_etiqueta;
    ReferenciaSalto *primera_referencia;
    ReferenciaSalto *ultima_referencia;
    unsigned int siguiente_id;  /* Para generar nombres unicos. */
} TablaEtiquetas;

/* Ciclo de vida. */
TablaEtiquetas *etq_crear(void);
void etq_destruir(TablaEtiquetas *tabla);

/* Produce un nombre como L_WHILE_0; no define la etiqueta. */
ResultadoEtiquetas etq_generar_nombre(
    TablaEtiquetas *tabla,
    const char *prefijo,
    char *salida,
    size_t capacidad
);

/* Registra una etiqueta en la posicion DEFINITIVA de su bundle. */
ResultadoEtiquetas etq_definir(
    TablaEtiquetas *tabla,
    const char *nombre,
    size_t indice_bundle
);

/* Registra un salto cuyo destino puede estar definido mas adelante. */
ResultadoEtiquetas etq_agregar_salto(
    TablaEtiquetas *tabla,
    size_t bundle_origen,
    TipoSalto tipo,
    const char *etiqueta_destino
);

/* Dos pasadas conceptuales: ubicar etiquetas y resolver saltos.
   Llamar despues de calendarizar y fijar el orden de los bundles. */
ResultadoEtiquetas etq_resolver(
    TablaEtiquetas *tabla,
    size_t cantidad_bundles
);

/* Consultas para el generador y para depuracion. Punteros prestados:
   no deben liberarse ni usarse despues de etq_destruir. */
const Etiqueta *etq_buscar_etiqueta(
    const TablaEtiquetas *tabla,
    const char *nombre
);

const ReferenciaSalto *etq_buscar_salto(
    const TablaEtiquetas *tabla,
    size_t bundle_origen
);

/* Visualizacion para pruebas y defensa. */
void etq_imprimir(const TablaEtiquetas *tabla, FILE *salida);

#endif /* ETIQUETAS_H */
