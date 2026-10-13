#include "binario.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIN_MAX_LINEA 2048
#define BIN_MAX_OPS 8

typedef struct {
    uint32_t alu;
    uint32_t lsu;
    uint32_t bru;
    uint32_t feist;
} BundleCodificado;

static char *trim(char *s)
{
    char *fin;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    fin = s + strlen(s) - 1;
    while (fin >= s && isspace((unsigned char)*fin)) *fin-- = '\0';
    return s;
}

static int solo_decimal(const char *s)
{
    if (*s == '+' || *s == '-') s++;
    if (*s == '\0') return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

static int leer_numero(const char *texto, int64_t *valor)
{
    char *fin;
    long long v;
    const char *p = texto;

    while (isspace((unsigned char)*p)) p++;
    errno = 0;
    v = strtoll(p, &fin, 0);
    while (isspace((unsigned char)*fin)) fin++;

    if (errno != ERANGE && *fin == '\0') {
        *valor = (int64_t)v;
        return 1;
    }

    /* Igual que assembler.py: aceptar, por ejemplo, 05 como decimal. */
    if (solo_decimal(p)) {
        errno = 0;
        v = strtoll(p, &fin, 10);
        while (isspace((unsigned char)*fin)) fin++;
        if (errno != ERANGE && *fin == '\0') {
            *valor = (int64_t)v;
            return 1;
        }
    }

    return 0;
}

static int leer_registro(const char *texto, unsigned *registro)
{
    char *fin;
    long n;
    const char *p = texto;

    while (isspace((unsigned char)*p)) p++;
    if (*p != 'x' && *p != 'X') return 0;
    p++;
    if (!isdigit((unsigned char)*p)) return 0;

    errno = 0;
    n = strtol(p, &fin, 10);
    while (isspace((unsigned char)*fin)) fin++;
    if (errno == ERANGE || *fin != '\0' || n < 0 || n > 15) return 0;

    *registro = (unsigned)n;
    return 1;
}

static int inmediato(const char *texto, unsigned bits, uint32_t *resultado)
{
    int64_t valor;
    int64_t minimo = -((int64_t)1 << (bits - 1));
    int64_t maximo = ((int64_t)1 << (bits - 1)) - 1;
    uint64_t mascara = ((uint64_t)1 << bits) - 1u;

    if (!leer_numero(texto, &valor)) return 0;
    if (valor < minimo || valor > maximo) return 0;
    *resultado = (uint32_t)((uint64_t)valor & mascara);
    return 1;
}

static int separar_operandos(char *texto, char *ops[], int max_ops)
{
    int n = 0;
    char *inicio = texto;
    char *p = texto;

    if (*trim(texto) == '\0') return 0;

    while (1) {
        if (*p == ',' || *p == '\0') {
            char separador = *p;
            if (n >= max_ops) return -1;
            if (separador == ',') *p = '\0';
            ops[n++] = trim(inicio);
            if (separador == '\0') break;
            inicio = p + 1;
        }
        p++;
    }
    return n;
}

static int extraer_mnemonico(char *texto, char **mnemonico, char **resto)
{
    char *p = trim(texto);
    char *q;
    if (*p == '\0') return 0;
    q = p;
    while (*q && !isspace((unsigned char)*q)) q++;
    if (*q) {
        *q++ = '\0';
        q = trim(q);
    }
    *mnemonico = p;
    *resto = *q ? q : q;
    return 1;
}

static void mayusculas(char *s)
{
    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

static int codificar_alu(char *texto, uint32_t *valor, char *error, size_t nerr)
{
    char *mn, *resto, *ops[BIN_MAX_OPS];
    int n;
    unsigned rd, rs1, rs2;
    uint32_t imm, opcode;

    if (!extraer_mnemonico(texto, &mn, &resto)) goto invalida;
    mayusculas(mn);
    n = separar_operandos(resto, ops, BIN_MAX_OPS);
    if (n < 0) goto invalida;

    if (strcmp(mn, "NOP") == 0) {
        if (n != 0) goto cantidad;
        *valor = 0;
        return 1;
    }

    if (strcmp(mn, "ADD") == 0) opcode = 0b001;
    else if (strcmp(mn, "SUB") == 0) opcode = 0b010;
    else if (strcmp(mn, "OR") == 0) opcode = 0b011;
    else if (strcmp(mn, "XOR") == 0) opcode = 0b100;
    else if (strcmp(mn, "AND") == 0) opcode = 0b101;
    else if (strcmp(mn, "SLT") == 0) opcode = 0b110;
    else opcode = 0;

    if (opcode != 0) {
        if (n != 3) goto cantidad;
        if (!leer_registro(ops[0], &rd) || !leer_registro(ops[1], &rs1) ||
            !leer_registro(ops[2], &rs2)) goto invalida;
        *valor = (rs2 << 12) | (rs1 << 8) | (rd << 4) | opcode;
        return 1;
    }

    if (strcmp(mn, "ADDI") == 0) opcode = 0b001;
    else if (strcmp(mn, "SRLI") == 0) opcode = 0b010;
    else if (strcmp(mn, "SLLI") == 0) opcode = 0b011;
    else goto desconocida;

    if (n != 3) goto cantidad;
    if (!leer_registro(ops[0], &rd) || !leer_registro(ops[1], &rs1) ||
        !inmediato(ops[2], 12, &imm)) goto invalida;
    *valor = (imm << 12) | (rs1 << 8) | (rd << 4) | (1u << 3) | opcode;
    return 1;

cantidad:
    snprintf(error, nerr, "%s: cantidad de operandos invalida", mn);
    return 0;
desconocida:
    snprintf(error, nerr, "instruccion ALU desconocida: %s", mn);
    return 0;
invalida:
    snprintf(error, nerr, "instruccion ALU invalida");
    return 0;
}

static int parsear_direccion(char *texto, uint32_t *off, unsigned *rb)
{
    char *abre = strchr(texto, '(');
    char *cierra = strrchr(texto, ')');
    char *desp;
    char *reg;

    if (abre == NULL || cierra == NULL || cierra < abre || *trim(cierra + 1) != '\0')
        return 0;
    *abre = '\0';
    *cierra = '\0';
    desp = trim(texto);
    reg = trim(abre + 1);
    return inmediato(desp, 12, off) && leer_registro(reg, rb);
}

static int codificar_lsu(char *texto, uint32_t *valor, char *error, size_t nerr)
{
    char *mn, *resto, *ops[BIN_MAX_OPS];
    int n;
    unsigned reg, rb;
    uint32_t off, opcode;

    if (!extraer_mnemonico(texto, &mn, &resto)) goto invalida;
    mayusculas(mn);
    n = separar_operandos(resto, ops, BIN_MAX_OPS);
    if (n < 0) goto invalida;

    if (strcmp(mn, "NOP") == 0) {
        if (n != 0) goto cantidad;
        *valor = 0;
        return 1;
    }
    if (strcmp(mn, "LOAD") == 0) opcode = 0b01;
    else if (strcmp(mn, "STORE") == 0) opcode = 0b10;
    else {
        snprintf(error, nerr, "instruccion LSU desconocida: %s", mn);
        return 0;
    }

    if (n != 2) goto cantidad;
    if (!leer_registro(ops[0], &reg) || !parsear_direccion(ops[1], &off, &rb))
        goto invalida;

    *valor = (off << 10) | (rb << 6) | (reg << 2) | opcode;
    return 1;

cantidad:
    snprintf(error, nerr, "%s: cantidad de operandos invalida", mn);
    return 0;
invalida:
    snprintf(error, nerr, "instruccion LSU invalida");
    return 0;
}

static int codificar_bru(char *texto, uint32_t *valor, char *error, size_t nerr)
{
    char *mn, *resto, *ops[BIN_MAX_OPS];
    int n;
    unsigned rs1, rs2;
    uint32_t off, opcode;

    if (!extraer_mnemonico(texto, &mn, &resto)) goto invalida;
    mayusculas(mn);
    n = separar_operandos(resto, ops, BIN_MAX_OPS);
    if (n < 0) goto invalida;

    if (strcmp(mn, "NOP") == 0) {
        if (n != 0) goto cantidad;
        *valor = 0;
        return 1;
    }
    if (strcmp(mn, "JMP") == 0) {
        if (n != 1) goto cantidad;
        if (!inmediato(ops[0], 20, &off)) goto invalida;
        *valor = (off << 3) | 0b001;
        return 1;
    }

    if (strcmp(mn, "BGE") == 0) opcode = 0b010;
    else if (strcmp(mn, "BLE") == 0) opcode = 0b011;
    else if (strcmp(mn, "BEQ") == 0) opcode = 0b100;
    else if (strcmp(mn, "BNE") == 0) opcode = 0b101;
    else {
        snprintf(error, nerr, "instruccion BRU desconocida: %s", mn);
        return 0;
    }

    if (n != 3) goto cantidad;
    if (!leer_registro(ops[0], &rs1) || !leer_registro(ops[1], &rs2) ||
        !inmediato(ops[2], 12, &off)) goto invalida;
    *valor = (off << 11) | (rs2 << 7) | (rs1 << 3) | opcode;
    return 1;

cantidad:
    snprintf(error, nerr, "%s: cantidad de operandos invalida", mn);
    return 0;
invalida:
    snprintf(error, nerr, "instruccion BRU invalida o inmediato fuera de rango");
    return 0;
}

static int codificar_feist(char *texto, uint32_t *valor, char *error, size_t nerr)
{
    char *mn, *resto, *ops[BIN_MAX_OPS];
    int n;
    unsigned a, b, c, d;
    int64_t key, ronda, modo;

    if (!extraer_mnemonico(texto, &mn, &resto)) goto invalida;
    mayusculas(mn);
    n = separar_operandos(resto, ops, BIN_MAX_OPS);
    if (n < 0) goto invalida;

    if (strcmp(mn, "NOP") == 0) {
        if (n != 0) goto cantidad;
        *valor = 0;
        return 1;
    }
    if (strcmp(mn, "AUTH") == 0) {
        if (n != 2) goto cantidad;
        if (!leer_registro(ops[0], &a) || !leer_registro(ops[1], &b)) goto invalida;
        *valor = (b << 7) | (a << 3) | 0b001;
        return 1;
    }
    if (strcmp(mn, "LOGOUT") == 0) {
        if (n != 0) goto cantidad;
        *valor = 0b010;
        return 1;
    }
    if (strcmp(mn, "KSTORE") == 0) {
        if (n != 3) goto cantidad;
        if (!leer_numero(ops[0], &key) || !leer_numero(ops[1], &ronda) ||
            !leer_registro(ops[2], &a) || key < 0 || key > 3 || ronda < 0 || ronda > 3)
            goto invalida;
        *valor = ((uint32_t)key << 9) | ((uint32_t)ronda << 7) | (a << 3) | 0b011;
        return 1;
    }
    if (strcmp(mn, "FROUND") == 0) {
        if (n != 7) goto cantidad;
        if (!leer_registro(ops[0], &a) || !leer_registro(ops[1], &b) ||
            !leer_registro(ops[2], &c) || !leer_registro(ops[3], &d) ||
            !leer_numero(ops[4], &key) || !leer_numero(ops[5], &ronda) ||
            !leer_numero(ops[6], &modo) || key < 0 || key > 3 ||
            ronda < 0 || ronda > 3 || (modo != 0 && modo != 1))
            goto invalida;
        *valor = (a << 20) | (b << 16) | (c << 12) | (d << 8) |
                 ((uint32_t)key << 6) | ((uint32_t)ronda << 4) |
                 ((uint32_t)modo << 3) | 0b100;
        return 1;
    }

    snprintf(error, nerr, "instruccion FEIST desconocida: %s", mn);
    return 0;

cantidad:
    snprintf(error, nerr, "%s: cantidad de operandos invalida", mn);
    return 0;
invalida:
    snprintf(error, nerr, "instruccion FEIST invalida");
    return 0;
}

static int es_etiqueta(const char *linea)
{
    size_t n = strlen(linea);
    if (n == 0 || linea[n - 1] != ':') return 0;
    return strncmp(linea, "ALU:", 4) != 0 &&
           strncmp(linea, "LSU:", 4) != 0 &&
           strncmp(linea, "BRU:", 4) != 0 &&
           strncmp(linea, "FEIST:", 6) != 0;
}

static int codificar_bundle(char *linea, BundleCodificado *bundle,
                            char *error, size_t nerr)
{
    char *segmento = linea;
    char *p = linea;
    unsigned usados = 0;

    memset(bundle, 0, sizeof(*bundle));

    while (1) {
        char *fin = strchr(p, '|');
        char *dos_puntos;
        char *slot;
        char *instr;
        unsigned bit;
        uint32_t valor;
        int ok;

        if (fin != NULL) *fin = '\0';
        segmento = trim(p);
        if (*segmento == '\0') {
            snprintf(error, nerr, "slot vacio en bundle");
            return 0;
        }

        dos_puntos = strchr(segmento, ':');
        if (dos_puntos == NULL) {
            snprintf(error, nerr, "falta ':' en slot: %s", segmento);
            return 0;
        }
        *dos_puntos = '\0';
        slot = trim(segmento);
        instr = trim(dos_puntos + 1);
        mayusculas(slot);

        if (strcmp(slot, "ALU") == 0) bit = 1u;
        else if (strcmp(slot, "LSU") == 0) bit = 2u;
        else if (strcmp(slot, "BRU") == 0) bit = 4u;
        else if (strcmp(slot, "FEIST") == 0) bit = 8u;
        else {
            snprintf(error, nerr, "slot invalido: %s", slot);
            return 0;
        }
        if (usados & bit) {
            snprintf(error, nerr, "slot %s repetido en bundle", slot);
            return 0;
        }
        usados |= bit;

        if (bit == 1u) ok = codificar_alu(instr, &valor, error, nerr);
        else if (bit == 2u) ok = codificar_lsu(instr, &valor, error, nerr);
        else if (bit == 4u) ok = codificar_bru(instr, &valor, error, nerr);
        else ok = codificar_feist(instr, &valor, error, nerr);
        if (!ok) return 0;

        if (bit == 1u) bundle->alu = valor;
        else if (bit == 2u) bundle->lsu = valor;
        else if (bit == 4u) bundle->bru = valor;
        else bundle->feist = valor;

        if (fin == NULL) break;
        p = fin + 1;
    }

    return 1;
}

static void imprimir_bits32(FILE *f, uint32_t v)
{
    int i;
    for (i = 31; i >= 0; --i)
        fputc((v & (1u << i)) ? '1' : '0', f);
}

int binario_ensamblar_archivo(const char *entrada_asm,
                              const char *salida_bin,
                              int mostrar_paquetes,
                              FILE *reporte)
{
    FILE *in = NULL;
    FILE *out = NULL;
    FILE *rep = reporte != NULL ? reporte : stdout;
    char linea[BIN_MAX_LINEA];
    size_t numero_linea = 0;
    size_t bundles = 0;
    int ok = 0;

    if (entrada_asm == NULL || salida_bin == NULL) return 0;

    in = fopen(entrada_asm, "r");
    if (in == NULL) {
        fprintf(stderr, "Error: no se pudo abrir el ensamblador %s\n", entrada_asm);
        goto fin;
    }
    out = fopen(salida_bin, "w");
    if (out == NULL) {
        fprintf(stderr, "Error: no se pudo crear %s\n", salida_bin);
        goto fin;
    }

    while (fgets(linea, sizeof(linea), in) != NULL) {
        char original[BIN_MAX_LINEA];
        char trabajo[BIN_MAX_LINEA];
        char error[256];
        char *contenido;
        char *comentario;
        BundleCodificado b;

        numero_linea++;
        if (strchr(linea, '\n') == NULL && !feof(in)) {
            fprintf(stderr, "Error en linea %zu: linea demasiado larga\n", numero_linea);
            goto fin;
        }

        snprintf(original, sizeof(original), "%s", linea);
        contenido = trim(original);
        if (*contenido == '\0' || *contenido == '#') continue;

        comentario = strchr(contenido, '#');
        if (comentario != NULL) *comentario = '\0';
        comentario = strstr(contenido, "//");
        if (comentario != NULL) *comentario = '\0';
        contenido = trim(contenido);
        if (*contenido == '\0' || es_etiqueta(contenido)) continue;

        snprintf(trabajo, sizeof(trabajo), "%s", contenido);
        if (!codificar_bundle(trabajo, &b, error, sizeof(error))) {
            fprintf(stderr, "Error en linea %zu: %s\n", numero_linea, error);
            goto fin;
        }

        if (fprintf(out, "%08X%08X%08X%08X\n",
                    b.feist, b.bru, b.lsu, b.alu) < 0)
            goto fin;

        if (mostrar_paquetes) {
            fprintf(rep, "\nPaquete %zu\n", bundles);
            fprintf(rep, "  ASM: %s\n", contenido);
            fprintf(rep, "  HEX: %08X%08X%08X%08X\n",
                    b.feist, b.bru, b.lsu, b.alu);
            fprintf(rep, "  BIN: ");
            imprimir_bits32(rep, b.feist);
            imprimir_bits32(rep, b.bru);
            imprimir_bits32(rep, b.lsu);
            imprimir_bits32(rep, b.alu);
            fputc('\n', rep);
        }
        bundles++;
    }

    if (ferror(in)) goto fin;
    if (fclose(out) != 0) { out = NULL; goto fin; }
    out = NULL;
    ok = 1;

    fprintf(rep, "Archivo binario generado: %s\n", salida_bin);
    fprintf(rep, "Bundles generados: %zu\n", bundles);
    fprintf(rep, "Bytes de instrucciones: %zu\n", bundles * 16u);

fin:
    if (out != NULL) {
        fclose(out);
        if (!ok) remove(salida_bin);
    }
    if (in != NULL) fclose(in);
    return ok;
}
