#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdint.h>

/* Una unica memoria de datos: globales desde cero y pila desde el final. */
#define MEMORIA_TOTAL_BYTES 65536U
#define TAMANO_PALABRA 4U
#define TAMANO_CABECERA 8U

/* limite_globales es el primer byte libre despues de todas las globales. */
int pila_hay_espacio(uint32_t sp_actual,
                     uint32_t tamano_marco,
                     uint32_t espacio_cabecera,
                     uint32_t limite_globales);

#endif
