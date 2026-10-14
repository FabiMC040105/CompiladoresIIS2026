#ifndef VLIW_PACKER_H
#define VLIW_PACKER_H

#include <stdio.h>

/*
 * Empaquetado VLIW conservador posterior a Fase 5.
 *
 * La entrada debe tener todos los saltos resueltos a offsets numericos. Las
 * etiquetas pueden permanecer para depuracion. El empaquetador conserva la
 * cantidad total de bundles, por lo que las direcciones y offsets calculados
 * por Fase 5 siguen siendo validos. Cuando una operacion se mueve a otro slot
 * del bundle vecino, su posicion original se reemplaza por un NOP.
 *
 * Devuelve 1 en exito y 0 en error.
 */
int vliw_empaquetar_archivo(const char *entrada,
                            const char *salida,
                            FILE *reporte);

#endif
