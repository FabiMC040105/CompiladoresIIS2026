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
     * Supera el limite global de 32768 bytes.
     */
    if (ts_insertar(tabla, "lista_grande", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_LISTA, 1, &lista) != TS_OK ||
        ts_definir_dimensiones(lista, "8193", NULL) != TS_OK) {
        printf("ERROR al preparar la prueba.\n");
        ts_destruir(tabla);
        return 1;
    }

    ResultadoTS resultado = ts_asignar_direcciones_globales(tabla);

    if (resultado != TS_ERROR_MEMORIA) {
        printf("ERROR: se esperaba TS_ERROR_MEMORIA, se obtuvo %d.\n",
               (int)resultado);
        ts_destruir(tabla);
        return 1;
    }

    printf("PRUEBA 2 CORRECTA: se rechazo la lista que excede "
           "32768 bytes de memoria global.\n");

    ts_destruir(tabla);
    return 0;
}
