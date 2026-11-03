#include "memoria.h"

#include <stdint.h>

/*
 * [0, limite_globales)             : datos globales estaticos
 * [limite_globales, SP_actual)     : memoria libre compartida
 * [SP_actual, MEMORIA_TOTAL_BYTES) : frames activos, pila descendente
 *
 * El compilador verifica los tamanos estaticos; el codigo generado
 * comprueba el limite en cada reserva de frame de ejecucion.
 */
int pila_hay_espacio(uint32_t sp_actual,
                     uint32_t tamano_marco,
                     uint32_t espacio_cabecera,
                     uint32_t limite_globales)
{
    uint64_t reserva = (uint64_t)tamano_marco + espacio_cabecera;
    if (limite_globales > MEMORIA_TOTAL_BYTES ||
        sp_actual > MEMORIA_TOTAL_BYTES || sp_actual < limite_globales)
        return 0;

    return reserva <= (uint64_t)sp_actual - limite_globales;
}
