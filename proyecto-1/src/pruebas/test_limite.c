#include <stdio.h>
#include "tabla_simbolos.h"

int main(void)
{
    TablaSimbolos *tabla = ts_crear();
    Simbolo *lista = NULL;

    if (tabla == NULL) {
        printf("ERROR: no se pudo crear la tabla.\n");
        return 1;
    }

    /*
     * Una lista de 8193 enteros necesita 32772 bytes.
     * Supera los 32 KiB anteriores, pero cabe en los 64 KiB.
     */
    if (ts_insertar(tabla, "lista_grande", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_LISTA, 1, &lista) != TS_OK ||
        ts_definir_dimensiones(lista, "8193", NULL) != TS_OK) {
        printf("ERROR al preparar la prueba.\n");
        ts_destruir(tabla);
        return 1;
    }

    ResultadoTS resultado = ts_asignar_direcciones_globales(tabla);

    if (resultado != TS_OK || tabla->siguiente_direccion_global != 32772) {
        printf("ERROR: se esperaba TS_OK y 32772 bytes, se obtuvo %d.\n",
               (int)resultado);
        ts_destruir(tabla);
        return 1;
    }

    printf("PRUEBA CORRECTA: se aceptaron 32772 bytes de globales.\n");

    ts_destruir(tabla);

    tabla = ts_crear();
    lista = NULL;
    if (tabla == NULL ||
        ts_insertar(tabla, "exceso_memoria", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_LISTA, 1, &lista) != TS_OK ||
        ts_definir_dimensiones(lista, "16385", NULL) != TS_OK) {
        printf("ERROR al preparar la prueba de exceso de memoria.\n");
        ts_destruir(tabla);
        return 1;
    }
    resultado = ts_asignar_direcciones_globales(tabla);
    if (resultado != TS_ERROR_MEMORIA) {
        printf("ERROR: se aceptaron mas de 65536 bytes (%d).\n", (int)resultado);
        ts_destruir(tabla);
        return 1;
    }
    puts("PRUEBA CORRECTA: 65540 bytes globales se rechazan.");
    ts_destruir(tabla);
    return 0;
}
