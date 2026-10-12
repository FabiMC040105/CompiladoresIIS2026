#include <stdio.h>
#include "tabla_simbolos.h"

#define EXIGIR(condicion, mensaje) \
    do { \
        if (!(condicion)) { \
            printf("ERROR: %s\n", mensaje); \
            ts_destruir(tabla); \
            return 1; \
        } \
    } while (0)

int main(void)
{
    TablaSimbolos *tabla = ts_crear();
    Simbolo *global = NULL;
    Simbolo *funcion = NULL;
    Simbolo *a = NULL;
    Simbolo *b = NULL;
    Simbolo *resultado = NULL;
    Simbolo *temporal = NULL;
    Ambito *ambito_funcion;

    EXIGIR(tabla != NULL, "no se pudo crear la tabla");

    /* Crear una variable global. */
    EXIGIR(
        ts_insertar(tabla, "global", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_ESCALAR, 1, &global) == TS_OK,
        "no se pudo crear la variable global"
    );

    /* Registrar la funcion en el ambito global. */
    EXIGIR(
        ts_insertar(tabla, "sumar", SIM_FUNCION,
                    TIPO_ENTERO, FORMA_ESCALAR, 2, &funcion) == TS_OK,
        "no se pudo crear la funcion"
    );

    /* Crear el ambito de la funcion y vincularlo con su simbolo. */
    ambito_funcion = ts_entrar_ambito(
        tabla, "sumar", AMBITO_FUNCION
    );

    EXIGIR(
        ambito_funcion != NULL,
        "no se pudo crear el ambito de la funcion"
    );

    funcion->ambito_funcion = ambito_funcion;

    /* Crear y registrar los parametros en orden. */
    EXIGIR(
        ts_insertar(tabla, "a", SIM_PARAMETRO,
                    TIPO_ENTERO, FORMA_ESCALAR, 2, &a) == TS_OK,
        "no se pudo crear el parametro a"
    );

    EXIGIR(
        ts_insertar(tabla, "b", SIM_PARAMETRO,
                    TIPO_ENTERO, FORMA_ESCALAR, 2, &b) == TS_OK,
        "no se pudo crear el parametro b"
    );

    EXIGIR(
        ts_agregar_parametro(funcion, a) == TS_OK,
        "no se pudo registrar el parametro a en la firma"
    );

    EXIGIR(
        ts_agregar_parametro(funcion, b) == TS_OK,
        "no se pudo registrar el parametro b en la firma"
    );

    /* Crear dos variables locales. */
    EXIGIR(
        ts_insertar(tabla, "resultado", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_ESCALAR, 3,
                    &resultado) == TS_OK,
        "no se pudo crear la variable resultado"
    );

    EXIGIR(
        ts_insertar(tabla, "temporal", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_ESCALAR, 4,
                    &temporal) == TS_OK,
        "no se pudo crear la variable temporal"
    );

    /* Asignar direcciones globales y desplazamientos de la funcion. */
    EXIGIR(
        ts_asignar_direcciones_globales(tabla) == TS_OK,
        "fallo la asignacion de direcciones"
    );

    /* Comprobar la variable global. */
    EXIGIR(
        global->direccion_asignada &&
        global->direccion == 0 &&
        global->tamano_bytes == 4,
        "la variable global no tiene la direccion o el tamano esperado"
    );

    /* Comprobar los parametros segun la convencion actual. */
    EXIGIR(
        a->direccion_asignada &&
        a->direccion == 8 &&
        a->tamano_bytes == 4,
        "el parametro a no esta en FP+8"
    );

    EXIGIR(
        b->direccion_asignada &&
        b->direccion == 12 &&
        b->tamano_bytes == 4,
        "el parametro b no esta en FP+12"
    );

    /* Comprobar las variables locales. */
    EXIGIR(
        resultado->direccion_asignada &&
        resultado->direccion == -4 &&
        resultado->tamano_bytes == 4,
        "resultado no esta en FP-4"
    );

    EXIGIR(
        temporal->direccion_asignada &&
        temporal->direccion == -8 &&
        temporal->tamano_bytes == 4,
        "temporal no esta en FP-8"
    );

    /* Comprobar los tamanos registrados para la funcion. */
    EXIGIR(
        funcion->cantidad_parametros == 2 &&
        funcion->tamano_parametros == 8,
        "el tamano de los parametros no es 8 bytes"
    );

    EXIGIR(
        funcion->tamano_locales == 8 &&
        funcion->tamano_marco == 8,
        "el espacio reservado para las variables locales no es 8 bytes"
    );

    /* Mostrar el resultado para inspeccion. */
    ts_imprimir(tabla, stdout);

    printf("\nPRUEBA CORRECTA: parametros y variables locales.\n");
    printf("Parametros: FP+8, FP+12.\n");
    printf("Locales: FP-4, FP-8.\n");
    printf("Parametros: %ld bytes.\n", funcion->tamano_parametros);
    printf("Locales: %ld bytes.\n", funcion->tamano_locales);
    printf("Reserva local del marco: %ld bytes.\n",
           funcion->tamano_marco);

    ts_destruir(tabla);
    return 0;
}
