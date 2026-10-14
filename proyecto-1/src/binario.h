#ifndef BINARIO_H
#define BINARIO_H

#include <stdio.h>

/*
 * Convierte ensamblador VLIW ya resuelto a la representacion hexadecimal
 * utilizada por la memoria de instrucciones de la arquitectura.
 *
 * Cada bundle de 128 bits se escribe como 32 digitos hexadecimales:
 * [127:96] FEIST | [95:64] BRU | [63:32] LSU | [31:0] ALU.
 *
 * Si mostrar_paquetes != 0, imprime para cada bundle su ensamblador,
 * representacion hexadecimal y representacion binaria de 128 bits.
 */
int binario_ensamblar_archivo(const char *entrada_asm,
                              const char *salida_bin,
                              int mostrar_paquetes,
                              FILE *reporte);

#endif
