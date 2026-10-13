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

static void salto_fuera_rango(const char *inicio)
{
    FILE *f = fopen(ORIGEN,"w");
    int i;
    if (!f) exit(2);
    fprintf(f, "%s\n", inicio);
    for (i=0; i<130; ++i) fputs("ALU: NOP\n", f);
    fputs("LEJOS:\nALU: NOP\n", f);
    fclose(f);
    CHEQUEAR(!resolver_etiquetas_asm(ORIGEN, RESUELTA, NULL),
             "referencia PC relativa fuera de rango rechazada");
}

int main(void)
{
    ejemplo_completo();
    etiqueta_inexistente();
    etiqueta_duplicada();
    salto_fuera_rango("BRU: BEQ x2, x0, LEJOS");
    salto_fuera_rango("ALU: ADDI x3, x1, LEJOS");
    remove(ORIGEN);
    remove(RESUELTA);
    printf("Pruebas de integracion ASM: %d comprobaciones, %d fallos.\n", pruebas, fallos);
    return fallos == 0 ? 0 : 1;
}
