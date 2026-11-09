#include "memoria.h"
#include <stdio.h>

#define CHEQUEAR(x) do { if (!(x)) { \
    fprintf(stderr, "FALLO en linea %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

int main(void)
{
    /* Globales de 48 KiB: quedan 16 KiB disponibles para pila. */
    CHEQUEAR(pila_hay_espacio(65536, 16000, 8, 49152));
    CHEQUEAR(!pila_hay_espacio(65536, 16380, 8, 49152));
    /* Globales de 4 KiB: pila puede superar sin problema 32 KiB. */
    CHEQUEAR(pila_hay_espacio(65536, 40000, 8, 4096));
    /* La comprobacion tambien funciona con la pila ya ocupada. */
    CHEQUEAR(pila_hay_espacio(50000, 10000, 8, 4096));
    CHEQUEAR(!pila_hay_espacio(4200, 128, 8, 4096));
    CHEQUEAR(!pila_hay_espacio(4092, 0, 0, 4096));
    CHEQUEAR(!pila_hay_espacio(70000, 0, 0, 4096));
    CHEQUEAR(!pila_hay_espacio(65536, 4, 0, 65536));
    CHEQUEAR(pila_hay_espacio(65536, 0, 0, 65536));
    puts("PASS: memoria global/stack dinamica, limites y colisiones.");
    return 0;
}
