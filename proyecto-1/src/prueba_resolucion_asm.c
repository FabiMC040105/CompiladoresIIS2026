#include "resolucion_etiquetas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORIGEN "prueba_etiquetas_entrada.tmp"
#define RESUELTA "prueba_etiquetas_resuelta.tmp"
static int pruebas = 0;
static int fallos = 0;

#define CHEQUEAR(c, msg) do { \
    pruebas++; \
    if (!(c)) { fprintf(stderr, "FALLO: %s\n", msg); fallos++; } \
} while (0)

static char *leer(const char *ruta)
{
    FILE *f = fopen(ruta, "rb");
    char *buf;
    long n;
    if (f == NULL) return NULL;
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) || (buf = malloc((size_t)n+1)) == NULL) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf); fclose(f); return NULL;
    }
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void ejemplo_completo(void)
{
    FILE *f = fopen(ORIGEN,"w");
    char *r;
    if (!f) exit(2);
    fputs("# salida calendarizada\n"
          "INICIO:\n"
          "ALU: ADDI x3, x1, RET\n"
          "BRU: BNE x4, x0, FIN\n"
          "# NOP por dependencia RAW\n"
          "ALU: NOP\n"
          "BRU: JMP INICIO\n"
          "RET:\n"
          "BRU: JMP FIN\n"
          "FIN:\n"
          "BRU: JMP FIN\n", f);
    fclose(f);
    CHEQUEAR(resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL), "resolucion completa");
    r = leer(RESUELTA);
    CHEQUEAR(r != NULL, "archivo resuelto existe");
    if (r != NULL) {
        CHEQUEAR(strstr(r, "ALU: ADDI x3, x1, 64\n") != NULL, "ADDI PC relativo");
        CHEQUEAR(strstr(r, "BRU: BNE x4, x0, 64\n") != NULL, "salto condicional");
        CHEQUEAR(strstr(r, "BRU: JMP -48\n") != NULL, "salto hacia atras");
        CHEQUEAR(strstr(r, "BRU: JMP 16\n") != NULL, "salto hacia adelante");
        CHEQUEAR(strstr(r, "BRU: JMP 0\n") != NULL, "salto a si mismo");
        CHEQUEAR(strstr(r, "ALU: NOP\n") != NULL, "NOP conservado");
        CHEQUEAR(strstr(r, "RET:\n") != NULL, "etiqueta de depuracion conservada");
        free(r);
    }
}

static void etiqueta_inexistente(void)
{
    FILE *f = fopen(ORIGEN,"w");
    if (!f) exit(2);
    fputs("BRU: JMP NO_EXISTE\nALU: NOP\n", f);
    fclose(f);
    CHEQUEAR(!resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL), "etiqueta inexistente rechazada");
    f = fopen(RESUELTA,"r");
    CHEQUEAR(f == NULL, "no queda salida obsoleta tras error");
    if (f) fclose(f);
}

static void etiqueta_duplicada(void)
{
    FILE *f = fopen(ORIGEN,"w");
    if (!f) exit(2);
    fputs("A:\nALU: NOP\nA:\nBRU: JMP A\n", f);
    fclose(f);
    CHEQUEAR(!resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL), "etiqueta duplicada rechazada");
}

static void salto_condicional_largo(const char *instruccion,
                                   const char *salto_corto)
{
    FILE *f = fopen(ORIGEN, "w");
    char *r;
    int i;
    if (!f) exit(2);
    fprintf(f, "%s\n", instruccion);
    for (i = 0; i < 130; ++i) fputs("ALU: NOP\n", f);
    fputs("LEJOS:\nALU: NOP\n", f);
    fclose(f);
    CHEQUEAR(resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL),
             "branch largo relajado automaticamente");
    r = leer(RESUELTA);
    CHEQUEAR(r != NULL, "salida generada para branch largo");
    if (r != NULL) {
        CHEQUEAR(strstr(r, salto_corto) != NULL,
                 "salto condicional corto correcto");
        CHEQUEAR(strstr(r, "BRU: JMP 2096\n") != NULL,
                 "JMP llega al destino lejano");
        free(r);
    }
}

static void salto_condicional_hacia_atras(void)
{
    FILE *f = fopen(ORIGEN, "w");
    char *r;
    int i;
    if (!f) exit(2);
    fputs("INICIO:\n", f);
    for (i = 0; i < 130; ++i) fputs("ALU: NOP\n", f);
    fputs("BRU: BNE x2, x0, INICIO\nALU: NOP\n", f);
    fclose(f);
    CHEQUEAR(resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL),
             "branch lejano hacia atras convertido a JMP");
    r = leer(RESUELTA);
    CHEQUEAR(r != NULL, "salida salto hacia atras existe");
    if (r != NULL) {
        CHEQUEAR(strstr(r, "BRU: BEQ x2, x0, 32\n") != NULL,
                 "branch invertido no tomado evita el salto largo");
        CHEQUEAR(strstr(r, "BRU: JMP -2096\n") != NULL,
                 "JMP largo hacia atras apunta al inicio");
        free(r);
    }
}

static void relajacion_en_cascada(void)
{
    FILE *f = fopen(ORIGEN, "w");
    char *r;
    int i;
    if (!f) exit(2);
    /* El primer branch inicialmente alcanza MEDIO (128 bundles).
     * El segundo necesita expandirse y desplaza MEDIO un bundle,
     * forzando otra expansion en una pasada posterior. */
    fputs("BRU: BEQ x2, x0, MEDIO\n", f); /* 0 */
    for (i = 1; i < 20; ++i) fputs("ALU: NOP\n", f);
    fputs("BRU: BNE x3, x0, FIN\n", f);  /* 20 */
    for (i = 21; i < 128; ++i) fputs("ALU: NOP\n", f);
    fputs("MEDIO:\nALU: NOP\n", f);     /* 128 */
    for (i = 129; i < 180; ++i) fputs("ALU: NOP\n", f);
    fputs("FIN:\nALU: NOP\n", f);       /* 180 */
    fclose(f);
    CHEQUEAR(resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL),
             "branches que se desbordan en cascada son resueltos");
    r = leer(RESUELTA);
    CHEQUEAR(r != NULL, "salida para relajacion en cascada existe");
    if (r != NULL) {
        CHEQUEAR(strstr(r, "__branch_largo_fin_0:\n") != NULL &&
                 strstr(r, "__branch_largo_fin_1:\n") != NULL,
                 "dos iteraciones de relajacion realizaron ambas expansiones");
        free(r);
    }
}

static void pc_relativo_fuera_rango(void)
{
    FILE *f = fopen(ORIGEN, "w");
    int i;
    if (!f) exit(2);
    fputs("ALU: ADDI x3, x1, LEJOS\n", f);
    for (i = 0; i < 130; ++i) fputs("ALU: NOP\n", f);
    fputs("LEJOS:\nALU: NOP\n", f);
    fclose(f);
    CHEQUEAR(!resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL),
             "ADDI PC relativo fuera de rango sigue siendo error");
}

int main(void)
{
    ejemplo_completo();
    etiqueta_inexistente();
    etiqueta_duplicada();
    salto_condicional_largo("BRU: BEQ x2, x0, LEJOS",
                            "BRU: BNE x2, x0, 32\n");
    salto_condicional_largo("BRU: BNE x2, x0, LEJOS",
                            "BRU: BEQ x2, x0, 32\n");
    salto_condicional_largo("BRU: BGE x2, x0, LEJOS",
                            "BRU: BGE x2, x0, 32\n");
    salto_condicional_largo("BRU: BLE x2, x0, LEJOS",
                            "BRU: BLE x2, x0, 32\n");
    salto_condicional_hacia_atras();
    relajacion_en_cascada();
    pc_relativo_fuera_rango();
    remove(ORIGEN);
    remove(RESUELTA);
    printf("Pruebas de integracion ASM: %d comprobaciones, %d fallos.\n", pruebas, fallos);
    return fallos == 0 ? 0 : 1;
}
