#include <stdio.h>
#include "tabla_simbolos.h"

int main(void)
{
    TablaSimbolos *tabla = ts_crear();
    Simbolo *lista = NULL;

    if (tabla == NULL)
        return 1;

    if (ts_insertar(tabla, "lista_maxima", SIM_VARIABLE,
                    TIPO_ENTERO, FORMA_LISTA, 1, &lista) != TS_OK ||
        ts_definir_dimensiones(lista, "8192", NULL) != TS_OK)
    {
        printf("ERROR: no se pudo preparar la prueba.\n");
        ts_destruir(tabla);
        return 1;
    }

    ResultadoTS resultado = ts_asignar_direcciones_globales(tabla);

    if (resultado != TS_OK ||
        lista->direccion != 0 ||
        lista->tamano_bytes != 32768 ||
        !lista->direccion_asignada)
    {
        printf("ERROR: la lista de 32768 bytes no se asigno correctamente.\n");
        ts_destruir(tabla);
        return 1;
    }

    printf("PRUEBA CORRECTA: se aceptaron exactamente 32768 bytes.\n");

    ts_destruir(tabla);
    return 0;
}
