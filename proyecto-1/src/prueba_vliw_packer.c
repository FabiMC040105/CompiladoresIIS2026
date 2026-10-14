#include "vliw_packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int escribir(const char *ruta, const char *texto)
{
    FILE *f = fopen(ruta, "w");
    if (!f) return 0;
    fputs(texto, f);
    return fclose(f) == 0;
}

static char *leer(const char *ruta)
{
    FILE *f = fopen(ruta, "rb");
    long n;
    char *s;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    rewind(f);
    s = malloc((size_t)n + 1u);
    if (!s) { fclose(f); return NULL; }
    if (fread(s, 1, (size_t)n, f) != (size_t)n) { free(s); fclose(f); return NULL; }
    s[n] = '\0';
    fclose(f);
    return s;
}

static int contar_bundles(const char *s)
{
    int n = 0;
    const char *p = s;
    while (*p) {
        const char *fin = strchr(p, '\n');
        size_t len = fin ? (size_t)(fin - p) : strlen(p);
        if (len && p[len - 1] != ':') n++;
        if (!fin) break;
        p = fin + 1;
    }
    return n;
}

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); fallos++; } \
    else printf("PASS: %s\n", msg); \
} while (0)

int main(void)
{
    const char *in = "prueba_vliw_entrada.tmp";
    const char *out = "prueba_vliw_salida.tmp";
    char *r;
    int fallos = 0;

    /* Prueba combinada: ALU+LSU, ALU+BRU con ajuste, FEIST+ALU. */
    CHECK(escribir(in,
        "inicio:\n"
        "LSU: LOAD x4, 0(x15)\n"
        "ALU: ADDI x5, x0, 7\n"
        "ALU: NOP\n"
        "ALU: ADDI x6, x0, 1\n"
        "BRU: JMP 32\n"
        "ALU: NOP\n"
        "destino:\n"
        "ALU: NOP\n"
        "FEIST: AUTH x8, x9\n"
        "ALU: ADDI x10, x0, 3\n"
        "ALU: NOP\n"),
        "se crea ASM de prueba");

    CHECK(vliw_empaquetar_archivo(in, out, NULL),
          "empaquetador VLIW procesa el archivo");
    r = leer(out);
    CHECK(r != NULL, "se genera salida VLIW");
    if (r != NULL) {
        CHECK(strstr(r, "ALU: ADDI x5, x0, 7 | LSU: LOAD x4, 0(x15)") != NULL,
              "fusion ALU + LSU");
        CHECK(strstr(r, "ALU: ADDI x6, x0, 1 | BRU: JMP 48") != NULL,
              "fusion ALU + BRU ajusta offset +16");
        CHECK(strstr(r, "ALU: ADDI x10, x0, 3 | FEIST: AUTH x8, x9") != NULL,
              "fusion FEIST con otra unidad");
        CHECK(strstr(r, "inicio:\n") != NULL && strstr(r, "destino:\n") != NULL,
              "se conservan etiquetas de depuracion");
        CHECK(contar_bundles(r) == 10,
              "se conserva exactamente la cantidad de bundles");
        free(r);
    }

    remove(in);
    remove(out);

    /* Verificar que FEIST puede completar un bundle con las cuatro unidades. */
    CHECK(escribir(in,
        "ALU: NOP\n"
        "FEIST: FROUND x13, x14, x5, x6, 0, 0, 0\n"
        "ALU: ADDI x11, x0, 9 | LSU: LOAD x12, 0(x0) | BRU: BEQ x0, x2, 16\n"
        "ALU: NOP\n"
        "ALU: NOP\n"),
        "se crea caso de cuatro slots");
    CHECK(vliw_empaquetar_archivo(in, out, NULL),
          "empaquetador procesa caso de cuatro slots");
    r = leer(out);
    CHECK(r != NULL, "se genera salida de cuatro slots");
    if (r != NULL) {
        CHECK(strstr(r,
              "ALU: ADDI x11, x0, 9 | LSU: LOAD x12, 0(x0) | BRU: BEQ x0, x2, 16 | FEIST: FROUND x13, x14, x5, x6, 0, 0, 0") != NULL,
              "se genera un bundle con ALU + LSU + BRU + FEIST");
        CHECK(contar_bundles(r) == 5,
              "cuatro slots no alteran la cantidad de bundles");
        free(r);
    }

    remove(in);
    remove(out);
    if (fallos == 0)
        printf("Todas las pruebas del empaquetador VLIW pasaron.\n");
    return fallos ? EXIT_FAILURE : EXIT_SUCCESS;
}
