#include "binario.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;

#define CHEQUEAR(cond, msg) \
    do { \
        if (cond) printf("PASS: %s\n", msg); \
        else { printf("FAIL: %s\n", msg); fallos++; } \
    } while (0)

int main(void)
{
    const char *entrada = "prueba_binario_entrada.tmp";
    const char *salida = "prueba_binario_salida.tmp";
    const char *esperado[] = {
        "00000000000000000000000000005029\n",
        "00000000000000000000038A00000241\n",
        "00000000007F80250000000000000000\n",
        "00452394000000000000000000000000\n"
    };
    FILE *f;
    char linea[128];
    size_t i;

    f = fopen(entrada, "w");
    if (f == NULL) return EXIT_FAILURE;
    fputs("ALU: ADDI x2, x0, 5\n", f);
    fputs("LSU: STORE x2, 0(x14) | ALU: ADD x4, x2, x0\n", f);
    fputs("BRU: BNE x4, x0, -16\n", f);
    fputs("FEIST: FROUND x4, x5, x2, x3, 2, 1, 0\n", f);
    fclose(f);

    CHEQUEAR(binario_ensamblar_archivo(entrada, salida, 0, NULL),
             "ensamblado VLIW genera archivo de codigo maquina");

    f = fopen(salida, "r");
    CHEQUEAR(f != NULL, "archivo binario puede abrirse");
    if (f != NULL) {
        for (i = 0; i < sizeof(esperado) / sizeof(esperado[0]); ++i) {
            if (fgets(linea, sizeof(linea), f) == NULL) {
                CHEQUEAR(0, "bundle esperado presente");
                break;
            }
            CHEQUEAR(strcmp(linea, esperado[i]) == 0,
                     "bundle coincide con assembler.py de Arquitectura");
        }
        CHEQUEAR(fgets(linea, sizeof(linea), f) == NULL,
                 "no se generaron bundles extra");
        fclose(f);
    }

    remove(entrada);
    remove(salida);

    if (fallos == 0)
        printf("PASS: codificacion compatible con la arquitectura final.\n");

    return fallos == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
