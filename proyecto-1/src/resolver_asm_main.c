#include "resolucion_etiquetas.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s entrada.asm salida_resuelta.asm\n", argv[0]);
        return 2;
    }
    return resolver_etiquetas_asm(argv[1], argv[2], stdout) ? 0 : 1;
}
