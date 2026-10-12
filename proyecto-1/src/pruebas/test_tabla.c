
#include <stdio.h>
#include "tabla_simbolos.h"

int main(void)
{
    TablaSimbolos *tabla = ts_crear();
    Simbolo *a = NULL;
    Simbolo *b = NULL;
    Simbolo *lista = NULL;
    Simbolo *matriz = NULL;
    Simbolo *funcion = NULL;
    Simbolo *local = NULL;

    if (tabla == NULL) {
        printf("ERROR: no se pudo crear la tabla.\n");
        return 1;
    }

    /* Dos variables globales escalares: 4 bytes cada una. */
    if (ts_insertar(tabla, "a", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_ESCALAR, 1, &a) != TS_OK ||
        ts_insertar(tabla, "b", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_ESCALAR, 2, &b) != TS_OK) {
        printf("ERROR al insertar variables escalares.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* Lista global de 3 enteros: 12 bytes. */
    if (ts_insertar(tabla, "lista", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_LISTA, 3, &lista) != TS_OK ||
        ts_definir_dimensiones(lista, "3", NULL) != TS_OK) {
        printf("ERROR al crear la lista.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* Matriz global de 2 x 3 enteros: 24 bytes. */
    if (ts_insertar(tabla, "matriz", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_MATRIZ, 4, &matriz) != TS_OK ||
        ts_definir_dimensiones(matriz, "2", "3") != TS_OK) {
        printf("ERROR al crear la matriz.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* Las funciones no reciben direcciones de datos globales. */
    if (ts_insertar(tabla, "mi_funcion", SIM_FUNCION,
                    TIPO_VACIO, FORMA_ESCALAR, 5, &funcion) != TS_OK) {
        printf("ERROR al insertar la funcion.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* Crear un marco de funcion y una variable local. */
    if (ts_entrar_ambito(tabla, "mi_funcion",
                         AMBITO_FUNCION) == NULL ||
        ts_insertar(tabla, "local", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_ESCALAR, 6, &local) != TS_OK) {
        printf("ERROR al crear la variable local.\n");
        ts_destruir(tabla);
        return 1;
    }

    if (ts_salir_ambito(tabla) != TS_OK) {
        printf("ERROR al salir del ambito.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* Asignar globales y desplazamientos de las funciones. */
    if (ts_asignar_direcciones_globales(tabla) != TS_OK) {
        printf("ERROR al asignar direcciones y desplazamientos.\n");
        ts_destruir(tabla);
        return 1;
    }

    ts_imprimir(tabla, stdout);

    printf("\nComprobando resultados...\n");

    /* Direcciones absolutas globales esperadas. */
    if (a->direccion != 0 ||
        b->direccion != 4 ||
        lista->direccion != 8 ||
        matriz->direccion != 20) {
        printf("ERROR: las direcciones globales son incorrectas.\n");
        ts_destruir(tabla);
        return 1;
    }

    if (!a->direccion_asignada ||
        !b->direccion_asignada ||
        !lista->direccion_asignada ||
        !matriz->direccion_asignada) {
        printf("ERROR: hay variables globales sin direccion.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* La funcion no es una variable de datos. */
    if (funcion->direccion_asignada) {
        printf("ERROR: se asigno una direccion de datos a la funcion.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* La variable local ocupa 4 bytes debajo del marco x15. */
    if (!local->direccion_asignada || local->direccion != -4) {
        printf("ERROR: el desplazamiento local esperado es -4.\n");
        ts_destruir(tabla);
        return 1;
    }

    /* 4 + 4 + 12 + 24 = 44 bytes globales. */
    if (tabla->siguiente_direccion_global != 44) {
        printf("ERROR: el espacio global reservado no es correcto.\n");
        ts_destruir(tabla);
        return 1;
    }

    printf("PRUEBA 1 CORRECTA\n");

    ts_destruir(tabla);
    return 0;
}
