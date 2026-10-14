/*
Incluye las estructuras y funciones necesarias para conectar la tabla
de simbolos con la generacion de ensamblador, junto con las bibliotecas
utilizadas para buscar simbolos y convertir dimensiones numericas.
*/
#include "codegen_tabla.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*
Convierte el tipo de dato registrado en la tabla de simbolos a texto.
Este valor se copia despues al simbolo que utiliza codegen.c.
*/
static const char *tipo_texto(TipoDato tipo)
{
    switch (tipo) {
    case TIPO_ENTERO: return "entero";
    case TIPO_VOF: return "vof";
    case TIPO_VACIO: return "vacio";
    default: return "desconocido";
    }
}

/*
Busca un identificador visible desde el ambito donde fue reconocido.
Recorre el ambito actual y sus padres hasta encontrar el simbolo, por lo
que se respeta el alcance de las variables y la prioridad del ambito local.
Retorna NULL cuando no existe un simbolo visible con ese nombre.
*/
static Simbolo *buscar_visible_desde(const TablaSimbolos *tabla,
                                     const char *nombre,
                                     const Ambito *ambito)
{
    const Ambito *actual;

    if (tabla == NULL || nombre == NULL)
        return NULL;

    /* Se revisa primero el ambito mas cercano y despues los superiores. */
    for (actual = ambito; actual != NULL; actual = actual->padre) {
        Simbolo *simbolo = ts_buscar_local(tabla, nombre, actual);
        if (simbolo != NULL)
            return simbolo;
    }

    return NULL;
}

/*
Busca una funcion por nombre dentro de la tabla de simbolos.
Solamente acepta simbolos de categoria SIM_FUNCION declarados en el
ambito global, ya que las funciones se registran en ese nivel.
*/
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

/*
Convierte las dimensiones de listas y matrices almacenadas como texto
a un entero sin signo de 32 bits. Valida que el contenido sea numerico,
positivo y que no sobrepase el limite permitido. Retorna 0 si es invalido.
*/
static uint32_t dimension_numerica(const char *texto)
{
    char *fin;
    unsigned long valor;

    if (texto == NULL || *texto == '\0' || texto[0] == '-')
        return 0;

    /* Reinicia errno para detectar errores de conversion de strtoul. */
    errno = 0;
    valor = strtoul(texto, &fin, 10);
    if (errno != 0 || fin == texto || *fin != '\0' ||
        valor == 0 || valor > UINT32_MAX)
        return 0;

    return (uint32_t)valor;
}

/*
Funcion que conecta la Fase 3 (tabla de simbolos) con la Fase 4
(generacion de ensamblador). Recibe el nodo del AST y el nombre que
necesita codegen, busca su simbolo respetando los ambitos y convierte
su informacion al formato CodegenSimbolo.

Devuelve 1 cuando encuentra y prepara el simbolo correctamente y 0
cuando no existe o le falta informacion necesaria para generar codigo.
*/
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

    /* Recupera el ambito original asociado al identificador en el AST. */
    if (simbolo == NULL && ctx->obtener_ambito_nodo != NULL && nodo != NULL)
        ambito = ctx->obtener_ambito_nodo(nodo);

    /* Intenta resolver primero el nombre dentro de los ambitos visibles. */
    if (simbolo == NULL && ambito != NULL)
        simbolo = buscar_visible_desde(ctx->tabla, nombre, ambito);

    /* Fallback seguro para nombres de funcion. */
    if (simbolo == NULL)
        simbolo = buscar_funcion_global(ctx->tabla, nombre);

    if (simbolo == NULL)
        return 0;

    /*
    Limpia el simbolo de salida y copia sus propiedades generales.
    El generador usa estos datos para decidir el tipo de operacion,
    la direccion de memoria y el acceso correspondiente.
    */
    memset(salida, 0, sizeof(*salida));
    salida->nombre = simbolo->nombre;
    salida->direccion = simbolo->direccion;
    salida->tipo = tipo_texto(simbolo->tipo);
    salida->cantidad_dimensiones =
        simbolo->cantidad_dimensiones >= 0 ?
        (unsigned)simbolo->cantidad_dimensiones : 0u;

    /* Convierte la forma del simbolo en escalar, lista o matriz. */
    switch (simbolo->forma) {
    case FORMA_LISTA: salida->forma = CODEGEN_FORMA_LISTA; break;
    case FORMA_MATRIZ: salida->forma = CODEGEN_FORMA_MATRIZ; break;
    default: salida->forma = CODEGEN_FORMA_ESCALAR; break;
    }

    /* Obtiene las dimensiones numericas para calcular accesos a arreglos. */
    if (simbolo->cantidad_dimensiones >= 1)
        salida->dimensiones[0] = dimension_numerica(simbolo->dimensiones[0]);
    if (simbolo->cantidad_dimensiones >= 2)
        salida->dimensiones[1] = dimension_numerica(simbolo->dimensiones[1]);

    /*
    Copia los tamanos calculados en la etapa de asignacion de memoria.
    Incluye almacenamiento del simbolo, parametros, locales y marco
    de funcion, necesarios para manejar la pila y las llamadas.
    */
    if (simbolo->tamano_bytes >= 0)
        salida->tamano_bytes = (uint32_t)simbolo->tamano_bytes;
    if (simbolo->tamano_parametros >= 0)
        salida->tamano_parametros = (uint32_t)simbolo->tamano_parametros;
    if (simbolo->tamano_locales >= 0)
        salida->tamano_locales = (uint32_t)simbolo->tamano_locales;
    if (simbolo->tamano_marco >= 0)
        salida->tamano_marco = (uint32_t)simbolo->tamano_marco;

    /*
    Determina la categoria y ubicacion real del simbolo:
    - Funcion: etiqueta de codigo.
    - Parametro: posicion relativa al FP, por referencia si es arreglo.
    - Variable: direccion global o espacio local dentro del marco.
    Rechaza variables y parametros que aun no tengan direccion asignada.
    */
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
