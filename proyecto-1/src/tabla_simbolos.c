/*
    Incluye las estructuras y funciones de la tabla de simbolos, junto con las bibliotecas
    necesarias para reservar memoria, trabajar con cadenas y controlar los limites numericos.
    memoria.h contiene los limites usados al asignar direcciones de memoria.
*/
#include "tabla_simbolos.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include "memoria.h"

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

/*
    Comprueba que una cadena exista y que no este vacia.
    Se utiliza para validar nombres de simbolos, ambitos y dimensiones.
*/
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

    /* Asigna un identificador unico y guarda la relacion con el ambito padre. */
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

/*
    Crea la tabla de simbolos e inicializa el ambito Global.
    Reserva memoria para la tabla y establece Global como ambito actual,
    de forma que las primeras declaraciones se registren en ese contexto.
*/
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

    /* El ambito Global sera el punto de partida para las declaraciones. */
    tabla->ambito_actual = tabla->ambito_global;
    return tabla;
}

/*
    Libera toda la memoria utilizada por la tabla de simbolos.
    Recorre las listas enlazadas de simbolos y ambitos, libera sus nombres,
    dimensiones y listas de parametros, y finalmente libera la tabla principal.
*/
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

/*
    Abre un nuevo ambito utilizando el ambito actual como padre.
    Verifica que el nombre y el tipo sean validos, crea el ambito y
    actualiza ambito_actual para registrar las siguientes declaraciones.
*/
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

/*
    Cierra el ambito activo regresando a su ambito padre.
    Impide salir cuando no existe un ambito superior, como sucede en Global.
*/
ResultadoTS ts_salir_ambito(TablaSimbolos *tabla)
{
    if (tabla == NULL || tabla->ambito_actual == NULL ||
        tabla->ambito_actual->padre == NULL)
        return TS_ERROR_AMBITO;

    tabla->ambito_actual = tabla->ambito_actual->padre;
    return TS_OK;
}

/*
    Busca un simbolo por nombre solamente dentro del ambito indicado.
    Recorre la lista general de simbolos y compara el nombre y el ambito,
    retornando NULL si no encuentra una coincidencia.
*/
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

/*
    Busca un simbolo visible desde el ambito actual.
    Primero revisa el ambito donde se encuentra el parser y luego recorre
    los ambitos padres hasta Global, permitiendo resolver variables externas.
*/
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

/*
    Inserta una nueva variable, parametro, funcion o etiqueta en la tabla de simbolos.
    Valida los argumentos y comprueba que el nombre no este repetido en el mismo ambito.
    Reserva memoria, registra el tipo, forma, linea y ambito, y enlaza el nuevo simbolo.
    La direccion de memoria se asigna posteriormente durante la distribucion de memoria.
*/
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

    /* Un nombre puede repetirse en otro ambito, pero no dentro del mismo. */
    if (ts_buscar_local(tabla, nombre, tabla->ambito_actual) != NULL)
        return TS_ERROR_DUPLICADO;

    simbolo = calloc(1, sizeof(*simbolo));
    if (simbolo == NULL)
        return TS_ERROR_MEMORIA;
    
    simbolo->tamano_bytes = 0;
    simbolo->tamano_parametros = 0;
    simbolo->tamano_locales = 0;
    simbolo->tamano_marco = 0;

    simbolo->nombre = ts_copiar_cadena(nombre);
    if (simbolo->nombre == NULL)
    {
        free(simbolo);
        return TS_ERROR_MEMORIA;
    }

    /* Guarda las propiedades semanticas del simbolo y el ambito de declaracion. */
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

/*
    Almacena las dimensiones de un simbolo que representa una lista o matriz.
    Verifica la cantidad de dimensiones segun la forma del simbolo y copia
    sus valores para conservarlos independientemente de las cadenas originales.
*/
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

    /* Se preparan copias nuevas antes de reemplazar las dimensiones existentes. */
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
/*
 * Convierte una dimension escrita como texto a un entero positivo.
 * Ejemplos validos: "3", "10", "25".
 */
static int ts_leer_dimension_positiva(
    const char *texto,
    unsigned long long *valor
)
{
    char *fin;
    unsigned long long numero;

    if (texto == NULL || texto[0] == '\0' || valor == NULL)
        return 0;

    /* Las dimensiones no pueden ser negativas ni comenzar con espacios. */
    if (texto[0] == '-' ||
        texto[0] == ' ' ||
        texto[0] == '\t' ||
        texto[0] == '\n')
        return 0;

    /* Reinicia errno para detectar correctamente un desbordamiento de strtoull. */
    errno = 0;
    numero = strtoull(texto, &fin, 10);

    /* Rechaza texto no numerico, cero y valores fuera de rango. */
    if (errno == ERANGE ||
        fin == texto ||
        *fin != '\0' ||
        numero == 0)
        return 0;

    *valor = numero;
    return 1;
}


/*
 * Calcula cuantos bytes necesita un simbolo global.
 * Cada elemento ocupa 4 bytes.
 */
static int ts_calcular_bytes_simbolo(
    const Simbolo *simbolo,
    unsigned long long *bytes
)
{
    unsigned long long dimension1;
    unsigned long long dimension2;
    unsigned long long elementos;

    if (simbolo == NULL || bytes == NULL)
        return 0;

    /* Determina cuantos elementos ocupa el simbolo segun su forma. */
    switch (simbolo->forma)
    {
        case FORMA_ESCALAR:
            elementos = 1;
            break;

        case FORMA_LISTA:
            if (simbolo->cantidad_dimensiones != 1 ||
                !ts_leer_dimension_positiva(
                    simbolo->dimensiones[0], &dimension1))
                return 0;

            elementos = dimension1;
            break;

        case FORMA_MATRIZ:
            if (simbolo->cantidad_dimensiones != 2 ||
                !ts_leer_dimension_positiva(
                    simbolo->dimensiones[0], &dimension1) ||
                !ts_leer_dimension_positiva(
                    simbolo->dimensiones[1], &dimension2))
                return 0;

            /* Evita desbordamiento al multiplicar las dimensiones. */
            if (dimension1 > ULLONG_MAX / dimension2)
                return 0;

            elementos = dimension1 * dimension2;
            break;

        default:
            return 0;
    }

    /* Cada elemento ocupa 4 bytes. */
    if (elementos > ULLONG_MAX / 4ULL)
        return 0;

    *bytes = elementos * 4ULL;
    return 1;
}

/* Encuentra la funcion a la que pertenece un ambito.
   Funciona tambien para ambitos anidados dentro de si, mientras y para.

    Recorre los ambitos padres para localizar la funcion propietaria de un bloque.
    Permite asociar variables locales de si, mientras y para al marco de
    la funcion correspondiente, aunque se encuentren en ambitos anidados.
*/
static Ambito *ts_funcion_propietaria(Ambito *ambito)
{
    while (ambito != NULL && ambito->tipo != AMBITO_FUNCION)
        ambito = ambito->padre;

    return ambito;
}

/*
 * Asigna direcciones globales y desplazamientos locales.
 *
 * Globales:
 *   0, 4, 8, ...
 *
 * Parametros:
 *   8, 12, 16, ...
 *
 * Variables locales:
 *   -4, -8, -12, ...
 *
 * Los desplazamientos locales son relativos al marco de la funcion.
 * El convenio de parametros es provisional y debe coincidir con
 * el que utilice posteriormente el generador de codigo.
 */

ResultadoTS ts_asignar_direcciones_globales(TablaSimbolos *tabla)
{
    Simbolo *actual;
    Ambito *ambito_funcion;
    unsigned long long siguiente_global = 0;

    /* Calcula el espacio de pila disponible y descuenta su cabecera reservada. */
    const unsigned long long CAPACIDAD_PILA =
        (unsigned long long)MEMORIA_TOTAL_BYTES - LIMITE_PILA;

    const unsigned long long MAXIMO_LOCALES =
        CAPACIDAD_PILA - TAMANO_CABECERA;

    if (tabla == NULL || tabla->ambito_global == NULL)
        return TS_ERROR_ARGUMENTO;

    /*
     * PASO 1.
     * Reiniciar toda la informacion de memoria.
     */
    for (actual = tabla->primero;
         actual != NULL;
         actual = actual->siguiente)
    {
        actual->direccion = 0;
        actual->direccion_asignada = 0;
        actual->tamano_bytes = 0;
        actual->tamano_parametros = 0;
        actual->tamano_locales = 0;
        actual->tamano_marco = 0;
    }

    /*
     * PASO 2.
     * Validar el tamano de las variables globales.
     *
     * Las direcciones globales comienzan en 0x0000.
     * Cada simbolo ocupa 4 bytes por elemento.
     */
    for (actual = tabla->primero;
         actual != NULL;
         actual = actual->siguiente)
    {
        unsigned long long bytes;

        if (actual->categoria != SIM_VARIABLE ||
            actual->ambito != tabla->ambito_global)
        {
            continue;
        }

        if (!ts_calcular_bytes_simbolo(actual, &bytes))
            return TS_ERROR_ARGUMENTO;

        if (siguiente_global > LIMITE_GLOBALES ||
            bytes > LIMITE_GLOBALES - siguiente_global)
        {
            return TS_ERROR_MEMORIA;
        }

        actual->tamano_bytes = (long)bytes;
        siguiente_global += bytes;
    }

    /*
     * PASO 3.
     * Asignar direcciones absolutas a las variables globales.
     */
    siguiente_global = 0;

    for (actual = tabla->primero;
         actual != NULL;
         actual = actual->siguiente)
    {
        unsigned long long bytes;

        if (actual->categoria != SIM_VARIABLE ||
            actual->ambito != tabla->ambito_global)
        {
            continue;
        }

        if (!ts_calcular_bytes_simbolo(actual, &bytes))
            return TS_ERROR_ARGUMENTO;

        actual->direccion = (long)siguiente_global;
        actual->direccion_asignada = 1;
        actual->tamano_bytes = (long)bytes;

        siguiente_global += bytes;
    }

    tabla->siguiente_direccion_global = (long)siguiente_global;

    /*
     * PASO 4.
     * Procesar cada ambito de funcion por separado.
     *
     * Los parametros se ubican en desplazamientos positivos:
     * 8(FP), 12(FP), 16(FP), ...
     *
     * Las variables locales se ubican en desplazamientos negativos:
     * -4(FP), -8(FP), -12(FP), ...
     */
    for (ambito_funcion = tabla->ambitos;
         ambito_funcion != NULL;
         ambito_funcion = ambito_funcion->siguiente)
    {
        Simbolo *simbolo;
        Simbolo *simbolo_funcion = NULL;

        /* Los parametros empiezan despues de los 8 bytes de cabecera del marco. */
        unsigned long long siguiente_parametro = 8ULL;
        unsigned long long espacio_parametros = 0;
        unsigned long long espacio_local = 0;

        if (ambito_funcion->tipo != AMBITO_FUNCION)
            continue;

        /*
         * Encontrar el simbolo que representa esta funcion.
         */
        for (simbolo = tabla->primero;
             simbolo != NULL;
             simbolo = simbolo->siguiente)
        {
            if (simbolo->categoria == SIM_FUNCION &&
                simbolo->ambito_funcion == ambito_funcion)
            {
                simbolo_funcion = simbolo;
                break;
            }
        }

        /*
         * PASO 4A.
         * Asignar desplazamientos a los parametros.
         *
         * Escalares: almacenan su valor (4 bytes).
         * Listas y matrices: almacenan una direccion (4 bytes).
         */
        for (simbolo = tabla->primero;
             simbolo != NULL;
             simbolo = simbolo->siguiente)
        {
            unsigned long long bytes;

            if (simbolo->categoria != SIM_PARAMETRO ||
                ts_funcion_propietaria(simbolo->ambito) !=
                    ambito_funcion)
            {
                continue;
            }

            if (simbolo->forma == FORMA_ESCALAR)
            {
                bytes = 4ULL;
            }
            else if (simbolo->forma == FORMA_LISTA ||
                     simbolo->forma == FORMA_MATRIZ)
            {
                bytes = 4ULL;
            }
            else
            {
                return TS_ERROR_ARGUMENTO;
            }

            if (siguiente_parametro > (unsigned long long)LONG_MAX ||
                bytes > (unsigned long long)LONG_MAX -
                            siguiente_parametro ||
                bytes > (unsigned long long)LONG_MAX -
                            espacio_parametros)
            {
                return TS_ERROR_MEMORIA;
            }

            simbolo->direccion = (long)siguiente_parametro;
            simbolo->direccion_asignada = 1;
            simbolo->tamano_bytes = (long)bytes;

            siguiente_parametro += bytes;
            espacio_parametros += bytes;
        }

        /*
         * PASO 4B.
         * Calcular el espacio total de variables locales.
         *
         * Incluye las variables de todos los ambitos anidados
         * que pertenecen a esta funcion.
         *
         * Se reserva espacio independiente para cada variable.
         * No se reutiliza espacio entre bloques.
         */
        for (simbolo = tabla->primero;
             simbolo != NULL;
             simbolo = simbolo->siguiente)
        {
            unsigned long long bytes;

            if (simbolo->categoria != SIM_VARIABLE ||
                simbolo->ambito == tabla->ambito_global ||
                ts_funcion_propietaria(simbolo->ambito) !=
                    ambito_funcion)
            {
                continue;
            }

            if (!ts_calcular_bytes_simbolo(simbolo, &bytes))
                return TS_ERROR_ARGUMENTO;
            /*
            * La pila ocupa desde LIMITE_PILA hasta MEMORIA_TOTAL_BYTES.
            * Su capacidad es de 32768 bytes.
            *
            * El marco incluye:
            * - Variables locales.
            * - 8 bytes para FP anterior y direccion de retorno.
            *
            * tamano_marco guarda solo el espacio de variables locales.
            */

            if (bytes > MAXIMO_LOCALES ||
                espacio_local > MAXIMO_LOCALES - bytes)
            {
                return TS_ERROR_MEMORIA;
            }

            espacio_local += bytes;

            if (espacio_local > (unsigned long long)LONG_MAX)
                return TS_ERROR_MEMORIA;

            /* Las variables locales crecen hacia direcciones negativas respecto al FP. */
            simbolo->direccion = -(long)espacio_local;
            simbolo->direccion_asignada = 1;
            simbolo->tamano_bytes = (long)bytes;
        }

        /*
         * PASO 4C.
         * Guardar los tamanos calculados en el simbolo de funcion.
         */
        if (simbolo_funcion != NULL)
        {
            if (espacio_parametros > (unsigned long long)LONG_MAX ||
                espacio_local > (unsigned long long)LONG_MAX)
            {
                return TS_ERROR_MEMORIA;
            }

            simbolo_funcion->tamano_parametros =
                (long)espacio_parametros;

            simbolo_funcion->tamano_locales =
                (long)espacio_local;

            /*
             * tamano_marco representa la reserva de locales
             * que debe realizar el generador de codigo.
             *
             * No incluye parametros ni los 8 bytes de cabecera.
             */
            simbolo_funcion->tamano_marco =
                (long)espacio_local;
        }
    }

    return TS_OK;
}
/*
    Agrega un parametro a la firma de una funcion.
    Verifica las categorias recibidas, evita repetir el mismo parametro
    y lo enlaza al final para conservar el orden de declaracion.
*/
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

    /* Actualiza la cantidad de elementos presentes en la firma de la funcion. */
    funcion->cantidad_parametros++;
    return TS_OK;
}

/*
    Convierte el tipo de dato registrado en la tabla a su nombre de texto.
    Se utiliza al imprimir los simbolos para mostrar entero, vof o vacio.
*/
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

/*
    Obtiene el nombre de la categoria de un simbolo para mostrarlo en la tabla.
    Distingue entre variables, parametros, funciones y etiquetas.
*/
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

/*
    Convierte la forma de almacenamiento del simbolo a texto.
    Permite distinguir un escalar, una lista y una matriz al imprimir la tabla.
*/
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

/*
    Devuelve el nombre asociado al tipo de ambito.
    Se utiliza para visualizar si un ambito es Global, funcion, bloque o for.
*/
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

/*
    Imprime la informacion de la tabla de simbolos en el flujo indicado.
    Primero muestra el resumen de memoria de cada funcion y despues la lista
    general con tipos, formas, ambitos, dimensiones y direcciones asignadas.
*/
void ts_imprimir(const TablaSimbolos *tabla, FILE *salida)
{
    const Simbolo *actual;
    int i;
    
    if (salida == NULL)
        return;

    if (tabla == NULL)
    {
        fprintf(salida, "Error: tabla de simbolos no inicializada.\n");
        return;
    }

    /* Encabezado general: 119 caracteres. */
    fprintf(salida, "\n");
    for (i = 0; i < 119; i++)
        fputc('=', salida);
    fprintf(salida, "\n");

    fprintf(salida, "%59s\n", "TABLA DE SIMBOLOS");

    for (i = 0; i < 119; i++)
        fputc('=', salida);
    fprintf(salida, "\n");

    /*
     * RESUMEN DE MEMORIA POR FUNCION
     *
     * Ancho total: 82 caracteres.
     */
    fprintf(salida, "\nRESUMEN DE MEMORIA POR FUNCION\n");

    for (i = 0; i < 82; i++)
        fputc('-', salida);
    fprintf(salida, "\n");

    fprintf(salida,
            "%-22s | %12s | %12s | %12s | %12s\n",
            "Funcion",
            "Parametros",
            "Locales",
            "Marco",
            "Total marco");

    for (i = 0; i < 82; i++)
        fputc('-', salida);
    fprintf(salida, "\n");

    for (actual = tabla->primero;
         actual != NULL;
         actual = actual->siguiente)
    {
        if (actual->categoria == SIM_FUNCION)
        {
            long total_marco = actual->tamano_marco;

            /*
             * tamano_marco representa los bytes de variables
             * locales. La cabecera ocupa 8 bytes.
             */
            if (total_marco >= 0 &&
                total_marco <= LONG_MAX - (long)TAMANO_CABECERA)
            {
                total_marco += (long)TAMANO_CABECERA;
            }

            fprintf(salida,
                    "%-22.22s | %12ld | %12ld | %12ld | %12ld\n",
                    actual->nombre,
                    actual->tamano_parametros,
                    actual->tamano_locales,
                    actual->tamano_marco,
                    total_marco);
        }
    }

    for (i = 0; i < 82; i++)
        fputc('-', salida);
    fprintf(salida, "\n");

    /*
     * TABLA GENERAL DE SIMBOLOS
     *
     * Ancho total: 119 caracteres.
     */
    fprintf(salida, "\nTABLA GENERAL DE SIMBOLOS\n");

    for (i = 0; i < 119; i++)
        fputc('-', salida);
    fprintf(salida, "\n");

    fprintf(salida,
            "%-4s | %-18s | %-10s | %-11s | %-8s | %-6s | %-5s | %-15s | %-18s\n",
            "ID",
            "Identificador",
            "Categoria",
            "Tipo",
            "Forma",
            "Ambito",
            "Linea",
            "Dimensiones",
            "Ubicacion");

    /*
     * Separador de columnas.
     * Se usan los mismos anchos que en las filas.
     */
    for (i = 0; i < 119; i++)
        fputc('-', salida);
    fprintf(salida, "\n");

    if (tabla->primero == NULL)
        fprintf(salida, "%-119s\n", "(sin simbolos)");

    for (actual = tabla->primero;
         actual != NULL;
         actual = actual->siguiente)
    {
        /* Prepara los textos que se imprimiran para dimensiones y ubicacion en memoria. */
        char dimensiones[96] = "-";
        char ubicacion[96] = "sin asignar";

        /* Dimensiones de listas y matrices. */
        if (actual->cantidad_dimensiones == 1 &&
            actual->dimensiones[0] != NULL)
        {
            snprintf(dimensiones,
                     sizeof(dimensiones),
                     "[%.70s]",
                     actual->dimensiones[0]);
        }
        else if (actual->cantidad_dimensiones == 2 &&
                 actual->dimensiones[0] != NULL &&
                 actual->dimensiones[1] != NULL)
        {
            snprintf(dimensiones,
                     sizeof(dimensiones),
                     "[%.35s][%.35s]",
                     actual->dimensiones[0],
                     actual->dimensiones[1]);
        }

        /*
         * Las funciones usan una etiqueta simbolica.
         * Su direccion numerica de codigo se resolvera
         * durante la generacion de codigo.
         */
        if (actual->categoria == SIM_FUNCION)
        {
            snprintf(ubicacion,
                     sizeof(ubicacion),
                     "%.95s",
                     actual->nombre);
        }
        else if (actual->direccion_asignada)
        {
            if (actual->ambito == tabla->ambito_global)
            {
                snprintf(ubicacion,
                         sizeof(ubicacion),
                         "0x%04lX",
                         actual->direccion);
            }
            else
            {
                snprintf(ubicacion,
                         sizeof(ubicacion),
                         "FP%+ld",
                         actual->direccion);
            }
        }

        fprintf(salida,
                "%-4d | %-18.18s | %-10s | %-11s | %-8s | %-6d | %-5d | %-15.15s | %-18.18s\n",
                actual->id,
                actual->nombre,
                ts_nombre_categoria(actual->categoria),
                ts_nombre_tipo(actual->tipo),
                ts_nombre_forma(actual->forma),
                actual->ambito != NULL ? actual->ambito->id : -1,
                actual->linea,
                dimensiones,
                ubicacion);
    }

    for (i = 0; i < 119; i++)
        fputc('-', salida);
    fprintf(salida, "\n\n");
}

/*
    Muestra los ambitos registrados y su relacion de jerarquia.
    Recorre la lista de ambitos e imprime su identificador, nombre, tipo
    y el identificador del ambito padre cuando existe.
*/
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
