#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdint.h>

#define MEMORIA_TOTAL_BYTES 65536U
#define LIMITE_PILA 32768U
#define LIMITE_GLOBALES 32768U
#define TAMANO_PALABRA 4U
#define TAMANO_CABECERA 8U

int pila_hay_espacio(uint32_t sp_actual,
                     uint32_t tamano_marco,
                     uint32_t espacio_cabecera);

#endif