#include "tabla_simbolos.h"

#include <stdlib.h>
#include <string.h>

/*
 * La tabla contiene dos listas planas: una para simbolos y otra para ambitos.
 * El campo padre de cada ambito permite resolver nombres lexicalmente.
 * Los simbolos y los ambitos se conservan hasta llamar a ts_destruir().
 */

/* Crea una copia propia de la cadena; evita depender del AST o del lexer. */
static char *ts_copiar_cadena(const char *texto)
{
    size_t longitud;
    char *copia;

    if (texto == NULL)//revisa si texto si existe si no error
        return NULL;

    longitud = strlen(texto) + 1;// el +1 permite copiar \0 que es el fin de la cadena
    copia = malloc(longitud);
    if (copia != NULL)
        memcpy(copia, texto, longitud);

    return copia;
}

static int ts_cadena_valida(const char *texto)
{
    return texto != NULL && texto[0] != '\0';
}

/* Crea y enlaza un ambito; no cambia el ambito activo. */
static Ambito *ts_crear_ambito_interno(
    TablaSimbolos *tabla,
    const char *nombre,
    TipoAmbito tipo,
    Ambito *padre)
{
    Ambito *ambito = calloc(1, sizeof(*ambito));

    if (ambito == NULL)//revisa que el calloc no haya fallado
        return NULL;

    ambito->nombre = ts_copiar_cadena(nombre);
    if (ambito->nombre == NULL)//revisa que el copiar cadena no haya fallado y si fallo libera el ambito
    {
        free(ambito);
        return NULL;
    }

    ambito->id = tabla->siguiente_id_ambito++;
    ambito->tipo = tipo;
    ambito->padre = padre;

    if (tabla->ultimo_ambito != NULL)// si es no 
        tabla->ultimo_ambito->siguiente = ambito;
    else
        tabla->ambitos = ambito;

    tabla->ultimo_ambito = ambito;
    return ambito;
}

TablaSimbolos *ts_crear(void)
{
    TablaSimbolos *tabla = calloc(1, sizeof(*tabla));

    if (tabla == NULL)
        return NULL;

    tabla->ambito_global = ts_crear_ambito_interno(
        tabla, "Global", AMBITO_GLOBAL, NULL);

    if (tabla->ambito_global == NULL)
    {
        free(tabla);
        return NULL;
    }

    tabla->ambito_actual = tabla->ambito_global;
    return tabla;
}

void ts_destruir(TablaSimbolos *tabla)
{
    Simbolo *simbolo;
    Ambito *ambito;

    if (tabla == NULL)
        return;

    /* Los parametros de firma apuntan a simbolos: no son sus propietarios. */
    simbolo = tabla->primero;
    while (simbolo != NULL)
    {
        Simbolo *siguiente_simbolo = simbolo->siguiente;
        ParametroFirma *parametro = simbolo->parametros;

        while (parametro != NULL)
        {
            ParametroFirma *siguiente_parametro = parametro->siguiente;
            free(parametro);
            parametro = siguiente_parametro;
        }

        free(simbolo->nombre);
        free(simbolo->dimensiones[0]);
        free(simbolo->dimensiones[1]);
        free(simbolo);
        simbolo = siguiente_simbolo;
    }

    ambito = tabla->ambitos;
    while (ambito != NULL)
    {
        Ambito *siguiente_ambito = ambito->siguiente;
        free(ambito->nombre);
        free(ambito);
        ambito = siguiente_ambito;
    }

    free(tabla);
}

Ambito *ts_entrar_ambito(TablaSimbolos *tabla,
                         const char *nombre,
                         TipoAmbito tipo)
{
    Ambito *nuevo;

    if (tabla == NULL || tabla->ambito_actual == NULL ||
        !ts_cadena_valida(nombre) || tipo == AMBITO_GLOBAL ||
        tipo < AMBITO_GLOBAL || tipo > AMBITO_FOR)
        return NULL;

    nuevo = ts_crear_ambito_interno(tabla, nombre, tipo,
                                   tabla->ambito_actual);
    if (nuevo != NULL)
        tabla->ambito_actual = nuevo;

    return nuevo;
}

ResultadoTS ts_salir_ambito(TablaSimbolos *tabla)
{
    if (tabla == NULL || tabla->ambito_actual == NULL ||
        tabla->ambito_actual->padre == NULL)
        return TS_ERROR_AMBITO;

    tabla->ambito_actual = tabla->ambito_actual->padre;
    return TS_OK;
}

Simbolo *ts_buscar_local(const TablaSimbolos *tabla,
                         const char *nombre,
                         const Ambito *ambito)
{
    Simbolo *actual;

    if (tabla == NULL || !ts_cadena_valida(nombre) || ambito == NULL)
        return NULL;

    for (actual = tabla->primero; actual != NULL; actual = actual->siguiente)
    {
        if (actual->ambito == ambito && strcmp(actual->nombre, nombre) == 0)
            return actual;
    }

    return NULL;
}

Simbolo *ts_buscar_visible(const TablaSimbolos *tabla, const char *nombre)
{
    const Ambito *ambito;

    if (tabla == NULL || !ts_cadena_valida(nombre))
        return NULL;

    for (ambito = tabla->ambito_actual; ambito != NULL; ambito = ambito->padre)
    {
        Simbolo *simbolo = ts_buscar_local(tabla, nombre, ambito);
        if (simbolo != NULL)
            return simbolo;
    }

    return NULL;
}

ResultadoTS ts_insertar(TablaSimbolos *tabla,
                        const char *nombre,
                        CategoriaSimbolo categoria,
                        TipoDato tipo,
                        FormaSimbolo forma,
                        int linea,
                        Simbolo **nuevo)
{
    Simbolo *simbolo;

    if (nuevo != NULL)
        *nuevo = NULL;

    if (tabla == NULL || tabla->ambito_actual == NULL ||
        !ts_cadena_valida(nombre) ||
        categoria < SIM_VARIABLE || categoria > SIM_ETIQUETA ||
        tipo < TIPO_ENTERO || tipo > TIPO_DESCONOCIDO ||
        forma < FORMA_ESCALAR || forma > FORMA_MATRIZ || linea < 0)
        return TS_ERROR_ARGUMENTO;

    if (ts_buscar_local(tabla, nombre, tabla->ambito_actual) != NULL)
        return TS_ERROR_DUPLICADO;

    simbolo = calloc(1, sizeof(*simbolo));
    if (simbolo == NULL)
        return TS_ERROR_MEMORIA;

    simbolo->nombre = ts_copiar_cadena(nombre);
    if (simbolo->nombre == NULL)
    {
        free(simbolo);
        return TS_ERROR_MEMORIA;
    }

    simbolo->id = tabla->siguiente_id_simbolo++;
    simbolo->categoria = categoria;
    simbolo->tipo = tipo;
    simbolo->forma = forma;
    simbolo->linea = linea;
    simbolo->ambito = tabla->ambito_actual;
    /* direccion_asignada = 0: una direccion cero podria ser valida. */

    if (tabla->ultimo != NULL)
        tabla->ultimo->siguiente = simbolo;
    else
        tabla->primero = simbolo;

    tabla->ultimo = simbolo;

    if (nuevo != NULL)
        *nuevo = simbolo;

    return TS_OK;
}

ResultadoTS ts_definir_dimensiones(Simbolo *simbolo,
                                  const char *dimension1,
                                  const char *dimension2)
{
    char *primera;
    char *segunda = NULL;

    if (simbolo == NULL ||
        (simbolo->forma != FORMA_LISTA && simbolo->forma != FORMA_MATRIZ) ||
        !ts_cadena_valida(dimension1))
        return TS_ERROR_ARGUMENTO;

    if (simbolo->forma == FORMA_LISTA && dimension2 != NULL)
        return TS_ERROR_ARGUMENTO;

    if (simbolo->forma == FORMA_MATRIZ && !ts_cadena_valida(dimension2))
        return TS_ERROR_ARGUMENTO;

    primera = ts_copiar_cadena(dimension1);
    if (primera == NULL)
        return TS_ERROR_MEMORIA;

    if (simbolo->forma == FORMA_MATRIZ)
    {
        segunda = ts_copiar_cadena(dimension2);
        if (segunda == NULL)
        {
            free(primera);
            return TS_ERROR_MEMORIA;
        }
    }

    /* Solo reemplazamos las dimensiones si ambas copias tuvieron exito. */
    free(simbolo->dimensiones[0]);
    free(simbolo->dimensiones[1]);
    simbolo->dimensiones[0] = primera;
    simbolo->dimensiones[1] = segunda;
    simbolo->cantidad_dimensiones = (segunda == NULL) ? 1 : 2;

    return TS_OK;
}

ResultadoTS ts_agregar_parametro(Simbolo *funcion, Simbolo *parametro)
{
    ParametroFirma *nuevo;
    ParametroFirma *actual;

    if (funcion == NULL || parametro == NULL ||
        funcion->categoria != SIM_FUNCION ||
        parametro->categoria != SIM_PARAMETRO ||
        funcion == parametro)
        return TS_ERROR_ARGUMENTO;

    for (actual = funcion->parametros; actual != NULL; actual = actual->siguiente)
    {
        if (actual->simbolo == parametro)
            return TS_ERROR_DUPLICADO;
    }

    nuevo = malloc(sizeof(*nuevo));
    if (nuevo == NULL)
        return TS_ERROR_MEMORIA;

    nuevo->simbolo = parametro;
    nuevo->siguiente = NULL;

    if (funcion->parametros == NULL)
        funcion->parametros = nuevo;
    else
    {
        actual = funcion->parametros;
        while (actual->siguiente != NULL)
            actual = actual->siguiente;
        actual->siguiente = nuevo;
    }

    funcion->cantidad_parametros++;
    return TS_OK;
}

static const char *ts_nombre_tipo(TipoDato tipo)
{
    switch (tipo)
    {
    case TIPO_ENTERO: return "entero";
    case TIPO_VOF: return "vof";
    case TIPO_VACIO: return "vacio";
    default: return "desconocido";
    }
}

static const char *ts_nombre_categoria(CategoriaSimbolo categoria)
{
    switch (categoria)
    {
    case SIM_VARIABLE: return "variable";
    case SIM_PARAMETRO: return "parametro";
    case SIM_FUNCION: return "funcion";
    case SIM_ETIQUETA: return "etiqueta";
    default: return "?";
    }
}

static const char *ts_nombre_forma(FormaSimbolo forma)
{
    switch (forma)
    {
    case FORMA_ESCALAR: return "escalar";
    case FORMA_LISTA: return "lista";
    case FORMA_MATRIZ: return "matriz";
    default: return "?";
    }
}

static const char *ts_nombre_ambito(TipoAmbito tipo)
{
    switch (tipo)
    {
    case AMBITO_GLOBAL: return "global";
    case AMBITO_FUNCION: return "funcion";
    case AMBITO_BLOQUE: return "bloque";
    case AMBITO_FOR: return "for";
    default: return "?";
    }
}

void ts_imprimir(const TablaSimbolos *tabla, FILE *salida)
{
    const Simbolo *actual;

    if (salida == NULL)
        return;

    fprintf(salida, "\nTABLA DE SIMBOLOS\n");
    if (tabla == NULL)
    {
        fprintf(salida, "(tabla no inicializada)\n");
        return;
    }

    fprintf(salida,
            "%-4s | %-20s | %-10s | %-11s | %-8s | %-6s | %-5s | %-16s | %-12s\n",
            "ID", "Identificador", "Categoria", "Tipo", "Forma", "Ambito", "Linea",
            "Dimensiones", "Direccion");
    fprintf(salida,
            "-----+----------------------+------------+-------------+----------+--------+-------+------------------+-------------\n");

    if (tabla->primero == NULL)
        fprintf(salida, "(sin simbolos)\n");

    for (actual = tabla->primero; actual != NULL; actual = actual->siguiente)
    {
        char dimensiones[96] = "-";
        char direccion[32] = "pendiente";

        if (actual->cantidad_dimensiones == 1 && actual->dimensiones[0] != NULL)
            snprintf(dimensiones, sizeof(dimensiones), "[%.70s]", actual->dimensiones[0]);
        else if (actual->cantidad_dimensiones == 2 &&
                 actual->dimensiones[0] != NULL && actual->dimensiones[1] != NULL)
            snprintf(dimensiones, sizeof(dimensiones), "[%.35s][%.35s]",
                     actual->dimensiones[0], actual->dimensiones[1]);

        if (actual->direccion_asignada)
            snprintf(direccion, sizeof(direccion), "%ld", actual->direccion);

        fprintf(salida,
                "%-4d | %-20.20s | %-10s | %-11s | %-8s | %-6d | %-5d | %-16.16s | %-12s\n",
                actual->id, actual->nombre, ts_nombre_categoria(actual->categoria),
                ts_nombre_tipo(actual->tipo), ts_nombre_forma(actual->forma),
                actual->ambito != NULL ? actual->ambito->id : -1, actual->linea,
                dimensiones, direccion);
    }
}

void ts_imprimir_ambitos(const TablaSimbolos *tabla, FILE *salida)
{
    const Ambito *actual;

    if (salida == NULL)
        return;

    fprintf(salida, "\nTABLA DE AMBITOS\n");
    if (tabla == NULL)
    {
        fprintf(salida, "(tabla no inicializada)\n");
        return;
    }

    fprintf(salida, "%-4s | %-24s | %-10s | %-6s\n",
            "ID", "Nombre", "Tipo", "Padre");
    fprintf(salida, "-----+--------------------------+------------+-------\n");

    for (actual = tabla->ambitos; actual != NULL; actual = actual->siguiente)
    {
        char padre[16] = "-";
        if (actual->padre != NULL)
            snprintf(padre, sizeof(padre), "%d", actual->padre->id);

        fprintf(salida, "%-4d | %-24.24s | %-10s | %-6s\n",
                actual->id, actual->nombre, ts_nombre_ambito(actual->tipo), padre);
    }
}
