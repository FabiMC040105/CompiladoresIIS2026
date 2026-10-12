#include "codegen_tabla.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const char *tipo_texto(TipoDato tipo)
{
    switch (tipo) {
    case TIPO_ENTERO: return "entero";
    case TIPO_VOF: return "vof";
    case TIPO_VACIO: return "vacio";
    default: return "desconocido";
    }
}

static Simbolo *buscar_visible_desde(const TablaSimbolos *tabla,
                                     const char *nombre,
                                     const Ambito *ambito)
{
    const Ambito *actual;

    if (tabla == NULL || nombre == NULL)
        return NULL;

    for (actual = ambito; actual != NULL; actual = actual->padre) {
        Simbolo *simbolo = ts_buscar_local(tabla, nombre, actual);
        if (simbolo != NULL)
            return simbolo;
    }

    return NULL;
}

static Simbolo *buscar_funcion_global(const TablaSimbolos *tabla,
                                      const char *nombre)
{
    Simbolo *s;

    if (tabla == NULL || nombre == NULL)
        return NULL;

    for (s = tabla->primero; s != NULL; s = s->siguiente)
        if (s->categoria == SIM_FUNCION &&
            s->ambito == tabla->ambito_global &&
            strcmp(s->nombre, nombre) == 0)
            return s;

    return NULL;
}

static uint32_t dimension_numerica(const char *texto)
{
    char *fin;
    unsigned long valor;

    if (texto == NULL || *texto == '\0' || texto[0] == '-')
        return 0;

    errno = 0;
    valor = strtoul(texto, &fin, 10);
    if (errno != 0 || fin == texto || *fin != '\0' ||
        valor == 0 || valor > UINT32_MAX)
        return 0;

    return (uint32_t)valor;
}

int codegen_resolver_desde_tabla(void *contexto,
                                 const NodoAST *nodo,
                                 const char *nombre,
                                 CodegenSimbolo *salida)
{
    CodegenTablaContexto *ctx = (CodegenTablaContexto *)contexto;
    const Ambito *ambito = NULL;
    Simbolo *simbolo = NULL;

    if (ctx == NULL || ctx->tabla == NULL || nombre == NULL || salida == NULL)
        return 0;

    /* Las llamadas y nodos AST_FUNCION/AST_PRINCIPAL no son identificadores
       marcados por Flex: las funciones siempre se buscan en Global. */
    if (nodo != NULL &&
        (nodo->tipo == AST_LLAMADA ||
         nodo->tipo == AST_FUNCION ||
         nodo->tipo == AST_PRINCIPAL))
        simbolo = buscar_funcion_global(ctx->tabla, nombre);

    if (simbolo == NULL && ctx->obtener_ambito_nodo != NULL && nodo != NULL)
        ambito = ctx->obtener_ambito_nodo(nodo);

    if (simbolo == NULL && ambito != NULL)
        simbolo = buscar_visible_desde(ctx->tabla, nombre, ambito);

    /* Fallback seguro para nombres de funcion. */
    if (simbolo == NULL)
        simbolo = buscar_funcion_global(ctx->tabla, nombre);

    if (simbolo == NULL)
        return 0;

    memset(salida, 0, sizeof(*salida));
    salida->nombre = simbolo->nombre;
    salida->direccion = simbolo->direccion;
    salida->tipo = tipo_texto(simbolo->tipo);
    salida->cantidad_dimensiones =
        simbolo->cantidad_dimensiones >= 0 ?
        (unsigned)simbolo->cantidad_dimensiones : 0u;

    switch (simbolo->forma) {
    case FORMA_LISTA: salida->forma = CODEGEN_FORMA_LISTA; break;
    case FORMA_MATRIZ: salida->forma = CODEGEN_FORMA_MATRIZ; break;
    default: salida->forma = CODEGEN_FORMA_ESCALAR; break;
    }

    if (simbolo->cantidad_dimensiones >= 1)
        salida->dimensiones[0] = dimension_numerica(simbolo->dimensiones[0]);
    if (simbolo->cantidad_dimensiones >= 2)
        salida->dimensiones[1] = dimension_numerica(simbolo->dimensiones[1]);

    if (simbolo->tamano_bytes >= 0)
        salida->tamano_bytes = (uint32_t)simbolo->tamano_bytes;
    if (simbolo->tamano_parametros >= 0)
        salida->tamano_parametros = (uint32_t)simbolo->tamano_parametros;
    if (simbolo->tamano_locales >= 0)
        salida->tamano_locales = (uint32_t)simbolo->tamano_locales;
    if (simbolo->tamano_marco >= 0)
        salida->tamano_marco = (uint32_t)simbolo->tamano_marco;

    switch (simbolo->categoria) {
    case SIM_FUNCION:
        salida->categoria = CODEGEN_CAT_FUNCION;
        salida->ubicacion = CODEGEN_UBICACION_FUNCION;
        break;

    case SIM_PARAMETRO:
        if (!simbolo->direccion_asignada)
            return 0;
        salida->categoria = CODEGEN_CAT_PARAMETRO;
        salida->ubicacion = CODEGEN_UBICACION_PARAMETRO;
        salida->parametro_por_referencia =
            simbolo->forma == FORMA_LISTA || simbolo->forma == FORMA_MATRIZ;
        break;

    case SIM_VARIABLE:
        if (!simbolo->direccion_asignada)
            return 0;
        salida->categoria = CODEGEN_CAT_VARIABLE;
        salida->ubicacion =
            simbolo->ambito == ctx->tabla->ambito_global ?
            CODEGEN_UBICACION_GLOBAL : CODEGEN_UBICACION_LOCAL;
        break;

    default:
        return 0;
    }

    return 1;
}
