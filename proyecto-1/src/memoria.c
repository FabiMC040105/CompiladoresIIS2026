#include "memoria.h"

#include <stdint.h>

/*
 * La memoria se divide en dos regiones:
 *
 * Globales: 0x0000 - 0x7FFF
 * Pila:     0x8000 - 0xFFFF
 *
 * La pila crece hacia direcciones menores.
 * Un SP inicial de 65536 representa una posicion
 * justo despues del ultimo byte de memoria.
 */

int pila_hay_espacio(uint32_t sp_actual,
                     uint32_t tamano_marco,
                     uint32_t espacio_cabecera)
{
    uint64_t reserva;
    uint64_t nuevo_sp;

    /*
     * No se acepta un SP fuera de la memoria.
     * 65536 es valido como posicion inicial de la pila.
     */
    if (sp_actual > MEMORIA_TOTAL_BYTES)
        return 0;

    /*
     * El SP no puede estar por debajo de la region
     * reservada para la pila.
     */
    if (sp_actual < LIMITE_PILA)
        return 0;

    /*
     * Calculamos en 64 bits para evitar desbordamientos.
     */
    reserva = (uint64_t)tamano_marco + espacio_cabecera;

    /*
     * La reserva no puede cruzar la frontera de la pila.
     */
    if (reserva > (uint64_t)sp_actual - LIMITE_PILA)
        return 0;

    nuevo_sp = (uint64_t)sp_actual - reserva;

    if (nuevo_sp < LIMITE_PILA)
        return 0;

    return 1;
}