#include "vliw_packer.h"
#include "scheduler.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VLIW_LINEA_MAX 2048u
#define VLIW_SLOT_MAX  512u
#define VLIW_LABEL_MAX 256u

typedef enum {
    SLOT_ALU = 0,
    SLOT_LSU = 1,
    SLOT_BRU = 2,
    SLOT_FEIST = 3,
    SLOT_CANTIDAD = 4
} SlotVLIW;

typedef struct {
    int presente;
    int nop;
    char texto[VLIW_SLOT_MAX];       /* instruccion sin "SLOT:" */
    SchedulerInfo info;
} OperacionVLIW;

typedef struct {
    OperacionVLIW op[SLOT_CANTIDAD];
} BundleVLIW;

typedef struct {
    char nombre[VLIW_LABEL_MAX];
    size_t bundle;
} EtiquetaVLIW;

typedef struct {
    BundleVLIW *bundles;
    size_t cantidad;
    size_t capacidad;
    EtiquetaVLIW *etiquetas;
    size_t cantidad_etiquetas;
    size_t capacidad_etiquetas;
} ProgramaVLIW;

typedef struct {
    size_t alu_lsu;
    size_t alu_bru;
    size_t lsu_bru;
    size_t feist_mix;
    size_t cuatro_slots;
} EstadisticasVLIW;

static char *trim(char *s)
{
    char *fin;
    while (isspace((unsigned char)*s)) s++;
    fin = s + strlen(s);
    while (fin > s && isspace((unsigned char)fin[-1])) *--fin = '\0';
    return s;
}

static void quitar_comentario(char *s)
{
    char *a = strchr(s, '#');
    char *b = strstr(s, "//");
    char *c = NULL;
    if (a != NULL && b != NULL) c = a < b ? a : b;
    else c = a != NULL ? a : b;
    if (c != NULL) *c = '\0';
}

static int es_nombre_etiqueta(const char *s)
{
    size_t i, n = strlen(s);
    if (n < 2 || s[n - 1] != ':') return 0;
    if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) return 0;
    for (i = 1; i + 1 < n; i++)
        if (!(isalnum((unsigned char)s[i]) || s[i] == '_')) return 0;
    return 1;
}

static const char *nombre_slot(SlotVLIW s)
{
    switch (s) {
        case SLOT_ALU: return "ALU";
        case SLOT_LSU: return "LSU";
        case SLOT_BRU: return "BRU";
        case SLOT_FEIST: return "FEIST";
        default: return "?";
    }
}

static int parsear_slot(const char *s, SlotVLIW *slot)
{
    if (strcmp(s, "ALU") == 0) *slot = SLOT_ALU;
    else if (strcmp(s, "LSU") == 0) *slot = SLOT_LSU;
    else if (strcmp(s, "BRU") == 0) *slot = SLOT_BRU;
    else if (strcmp(s, "FEIST") == 0) *slot = SLOT_FEIST;
    else return 0;
    return 1;
}

static void mayusculas(char *s)
{
    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

static int agregar_bundle(ProgramaVLIW *p, const BundleVLIW *b)
{
    BundleVLIW *nuevo;
    size_t cap;
    if (p->cantidad == p->capacidad) {
        cap = p->capacidad == 0 ? 64u : p->capacidad * 2u;
        if (cap < p->capacidad || cap > SIZE_MAX / sizeof(*nuevo)) return 0;
        nuevo = realloc(p->bundles, cap * sizeof(*nuevo));
        if (nuevo == NULL) return 0;
        p->bundles = nuevo;
        p->capacidad = cap;
    }
    p->bundles[p->cantidad++] = *b;
    return 1;
}

static int agregar_etiqueta(ProgramaVLIW *p, const char *nombre, size_t bundle)
{
    EtiquetaVLIW *nuevo;
    size_t cap, n = strlen(nombre);
    if (n >= VLIW_LABEL_MAX) return 0;
    if (p->cantidad_etiquetas == p->capacidad_etiquetas) {
        cap = p->capacidad_etiquetas == 0 ? 16u : p->capacidad_etiquetas * 2u;
        if (cap < p->capacidad_etiquetas || cap > SIZE_MAX / sizeof(*nuevo)) return 0;
        nuevo = realloc(p->etiquetas, cap * sizeof(*nuevo));
        if (nuevo == NULL) return 0;
        p->etiquetas = nuevo;
        p->capacidad_etiquetas = cap;
    }
    memcpy(p->etiquetas[p->cantidad_etiquetas].nombre, nombre, n + 1u);
    p->etiquetas[p->cantidad_etiquetas].bundle = bundle;
    p->cantidad_etiquetas++;
    return 1;
}

static void liberar_programa(ProgramaVLIW *p)
{
    free(p->bundles);
    free(p->etiquetas);
    memset(p, 0, sizeof(*p));
}

static int parsear_operacion(char *pieza, BundleVLIW *bundle,
                             size_t numero_linea, FILE *err)
{
    char *dos = strchr(pieza, ':');
    char slot_txt[16];
    char *slot_inicio, *instr;
    size_t n;
    SlotVLIW slot;
    OperacionVLIW *op;

    if (dos == NULL) {
        fprintf(err, "Empaquetador VLIW, linea %zu: falta ':' en '%s'\n",
                numero_linea, pieza);
        return 0;
    }
    *dos = '\0';
    slot_inicio = trim(pieza);
    n = strlen(slot_inicio);
    if (n == 0 || n >= sizeof(slot_txt)) return 0;
    memcpy(slot_txt, slot_inicio, n + 1u);
    mayusculas(slot_txt);
    if (!parsear_slot(slot_txt, &slot)) {
        fprintf(err, "Empaquetador VLIW, linea %zu: slot invalido '%s'\n",
                numero_linea, slot_txt);
        return 0;
    }
    op = &bundle->op[slot];
    if (op->presente) {
        fprintf(err, "Empaquetador VLIW, linea %zu: slot %s repetido\n",
                numero_linea, slot_txt);
        return 0;
    }
    instr = trim(dos + 1);
    if (*instr == '\0' || strlen(instr) >= sizeof(op->texto)) {
        fprintf(err, "Empaquetador VLIW, linea %zu: instruccion invalida\n",
                numero_linea);
        return 0;
    }
    if (!scheduler_analizar(slot_txt, instr, &op->info)) {
        fprintf(err, "Empaquetador VLIW, linea %zu: instruccion no reconocida: %s: %s\n",
                numero_linea, slot_txt, instr);
        return 0;
    }
    op->presente = 1;
    op->nop = strcmp(instr, "NOP") == 0;
    memcpy(op->texto, instr, strlen(instr) + 1u);
    return 1;
}

static int leer_programa(const char *ruta, ProgramaVLIW *p, FILE *err)
{
    FILE *f = fopen(ruta, "r");
    char linea[VLIW_LINEA_MAX];
    size_t numero = 0;
    if (f == NULL) {
        fprintf(err, "Empaquetador VLIW: no se pudo abrir %s\n", ruta);
        return 0;
    }
    while (fgets(linea, sizeof(linea), f) != NULL) {
        char *s, *cursor;
        BundleVLIW b;
        numero++;
        if (strchr(linea, '\n') == NULL && !feof(f)) {
            fprintf(err, "Empaquetador VLIW, linea %zu: linea demasiado larga\n", numero);
            fclose(f);
            return 0;
        }
        quitar_comentario(linea);
        s = trim(linea);
        if (*s == '\0') continue;
        if (es_nombre_etiqueta(s)) {
            size_t n = strlen(s);
            s[n - 1] = '\0';
            if (!agregar_etiqueta(p, s, p->cantidad)) {
                fclose(f);
                return 0;
            }
            continue;
        }
        memset(&b, 0, sizeof(b));
        cursor = s;
        while (cursor != NULL) {
            char *sep = strchr(cursor, '|');
            char *pieza;
            if (sep != NULL) *sep = '\0';
            pieza = trim(cursor);
            if (*pieza == '\0' || !parsear_operacion(pieza, &b, numero, err)) {
                fclose(f);
                return 0;
            }
            cursor = sep != NULL ? sep + 1 : NULL;
        }
        if (!agregar_bundle(p, &b)) {
            fclose(f);
            return 0;
        }
    }
    if (ferror(f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

static int actual(const OperacionVLIW *op)
{
    return op->presente && !op->nop;
}

static int cantidad_actuales(const BundleVLIW *b)
{
    int n = 0, s;
    for (s = 0; s < SLOT_CANTIDAD; s++) if (actual(&b->op[s])) n++;
    return n;
}

static void lecturas_bundle(const BundleVLIW *b, unsigned char out[16])
{
    int s, r;
    memset(out, 0, 16);
    for (s = 0; s < SLOT_CANTIDAD; s++) if (actual(&b->op[s]))
        for (r = 2; r < 16; r++) if (b->op[s].info.lee[r]) out[r] = 1;
}

static void escrituras_bundle(const BundleVLIW *b, unsigned char out[16])
{
    int s, r;
    memset(out, 0, 16);
    for (s = 0; s < SLOT_CANTIDAD; s++) if (actual(&b->op[s]))
        for (r = 2; r < 16; r++) if (b->op[s].info.escribe[r]) out[r] = 1;
}

static int interseccion(const unsigned char a[16], const unsigned char b[16])
{
    int r;
    for (r = 2; r < 16; r++) if (a[r] && b[r]) return 1;
    return 0;
}

static int redireccion_pc_no_bru(const BundleVLIW *b)
{
    int s;
    for (s = 0; s < SLOT_CANTIDAD; s++)
        if (s != SLOT_BRU && actual(&b->op[s]) && b->op[s].info.redirecciona_pc)
            return 1;
    return 0;
}

static int parse_long_completo(const char *s, long *v)
{
    char *fin;
    errno = 0;
    *v = strtol(s, &fin, 0);
    while (isspace((unsigned char)*fin)) fin++;
    return errno != ERANGE && *s != '\0' && *fin == '\0';
}

static int info_branch(const OperacionVLIW *op, size_t pos, size_t total,
                       long *offset, size_t *destino,
                       unsigned char lee[16], char *nombre, size_t n_nombre)
{
    char mn[16];
    const char *t = op->texto;
    int r1, r2;
    long off;
    memset(lee, 0, 16);
    if (!actual(op) || sscanf(t, "%15s", mn) != 1) return 0;
    if (strcmp(mn, "JMP") == 0) {
        const char *p = t + 3;
        while (isspace((unsigned char)*p)) p++;
        if (!parse_long_completo(p, &off)) return 0;
    } else if (strcmp(mn, "BGE") == 0 || strcmp(mn, "BLE") == 0 ||
               strcmp(mn, "BEQ") == 0 || strcmp(mn, "BNE") == 0) {
        const char *coma = strrchr(t, ',');
        if (coma == NULL || sscanf(t, "%*s x%d, x%d", &r1, &r2) != 2) return 0;
        if (!parse_long_completo(trim((char *)coma + 1), &off)) return 0;
        if (r1 >= 2 && r1 < 16) lee[r1] = 1;
        if (r2 >= 2 && r2 < 16) lee[r2] = 1;
    } else return 0;
    {
        int64_t d = (int64_t)pos * 16 + (int64_t)off;
        if (d < 0 || d % 16 != 0 || d / 16 >= (int64_t)total) return 0;
        *destino = (size_t)(d / 16);
    }
    *offset = off;
    snprintf(nombre, n_nombre, "%s", mn);
    return 1;
}

static int es_addi_constante_temporal(const OperacionVLIW *op,
                                      int *rd)
{
    long imm;
    int d, rs;
    char extra;
    if (!actual(op)) return 0;
    if (sscanf(op->texto, "ADDI x%d, x%d, %ld %c", &d, &rs, &imm, &extra) < 3)
        return 0;
    if (strncmp(op->texto, "ADDI ", 5) != 0 || rs != 0 || d < 4 || d > 12)
        return 0;
    *rd = d;
    return 1;
}

static int info_load(const OperacionVLIW *op, int *rd, int *base)
{
    long off;
    if (!actual(op) || strncmp(op->texto, "LOAD ", 5) != 0) return 0;
    return sscanf(op->texto, "LOAD x%d, %ld(x%d)", rd, &off, base) == 3;
}

static int es_store(const OperacionVLIW *op)
{
    int r, b;
    long off;
    return actual(op) && strncmp(op->texto, "STORE ", 6) == 0 &&
           sscanf(op->texto, "STORE x%d, %ld(x%d)", &r, &off, &b) == 3;
}

static int feist_usa_x1(const OperacionVLIW *op)
{
    char mn[16];
    int a, b, c, d;
    if (!actual(op) || sscanf(op->texto, "%15s", mn) != 1) return 0;
    if (strcmp(mn, "AUTH") == 0)
        return sscanf(op->texto, "%*s x%d, x%d", &a, &b) == 2 && (a == 1 || b == 1);
    if (strcmp(mn, "KSTORE") == 0)
        return sscanf(op->texto, "%*s %*d, %*d, x%d", &a) == 1 && a == 1;
    if (strcmp(mn, "FROUND") == 0)
        return sscanf(op->texto, "%*s x%d, x%d, x%d, x%d", &a, &b, &c, &d) == 4 &&
               (c == 1 || d == 1);
    return 0;
}

static int feist_cambia_boveda(const OperacionVLIW *op)
{
    return actual(op) &&
           (strncmp(op->texto, "AUTH", 4) == 0 ||
            strncmp(op->texto, "KSTORE", 6) == 0 ||
            strncmp(op->texto, "LOGOUT", 6) == 0);
}

static void marcar_destinos(const ProgramaVLIW *p, unsigned char *targets)
{
    size_t i, e;
    memset(targets, 0, p->cantidad);
    for (e = 0; e < p->cantidad_etiquetas; e++)
        if (p->etiquetas[e].bundle < p->cantidad)
            targets[p->etiquetas[e].bundle] = 1;
    for (i = 0; i < p->cantidad; i++) {
        long off;
        size_t dest;
        unsigned char lee[16];
        char mn[16];
        if (info_branch(&p->bundles[i].op[SLOT_BRU], i, p->cantidad,
                        &off, &dest, lee, mn, sizeof(mn)))
            targets[dest] = 1;
    }
}

static void mover_op(OperacionVLIW *destino, OperacionVLIW *origen)
{
    *destino = *origen;
    memset(origen, 0, sizeof(*origen));
}

static void poner_nop(BundleVLIW *b)
{
    SchedulerInfo info;
    int s;
    for (s = 0; s < SLOT_CANTIDAD; s++) memset(&b->op[s], 0, sizeof(b->op[s]));
    b->op[SLOT_ALU].presente = 1;
    b->op[SLOT_ALU].nop = 1;
    strcpy(b->op[SLOT_ALU].texto, "NOP");
    scheduler_analizar("ALU", "NOP", &info);
    b->op[SLOT_ALU].info = info;
}

static void limpiar_nops_si_hay_operacion(BundleVLIW *b)
{
    int s;
    if (cantidad_actuales(b) == 0) return;
    for (s = 0; s < SLOT_CANTIDAD; s++)
        if (b->op[s].presente && b->op[s].nop)
            memset(&b->op[s], 0, sizeof(b->op[s]));
}

static void fase_alu_lsu(ProgramaVLIW *p, const unsigned char *targets,
                         EstadisticasVLIW *st)
{
    size_t i;
    for (i = 0; i + 1 < p->cantidad; i++) {
        BundleVLIW *a = &p->bundles[i];
        BundleVLIW *b = &p->bundles[i + 1];
        int rd_load, base, rd_addi;
        if (cantidad_actuales(a) != 1 || !actual(&a->op[SLOT_LSU]) ||
            !info_load(&a->op[SLOT_LSU], &rd_load, &base)) continue;
        if (cantidad_actuales(b) != 1 || !actual(&b->op[SLOT_ALU]) ||
            !es_addi_constante_temporal(&b->op[SLOT_ALU], &rd_addi)) continue;
        if (rd_addi == rd_load || rd_addi == base || targets[i + 1]) continue;
        limpiar_nops_si_hay_operacion(a);
        mover_op(&a->op[SLOT_ALU], &b->op[SLOT_ALU]);
        poner_nop(b);
        st->alu_lsu++;
    }
}

static int ajustar_branch(OperacionVLIW *op, long nuevo)
{
    char mn[16];
    int a, b;
    if (sscanf(op->texto, "%15s", mn) != 1) return 0;
    if (strcmp(mn, "JMP") == 0) {
        snprintf(op->texto, sizeof(op->texto), "JMP %ld", nuevo);
        return 1;
    }
    if (sscanf(op->texto, "%*s x%d, x%d", &a, &b) != 2) return 0;
    snprintf(op->texto, sizeof(op->texto), "%s x%d, x%d, %ld", mn, a, b, nuevo);
    return 1;
}

static void fase_bru(ProgramaVLIW *p, const unsigned char *targets,
                     EstadisticasVLIW *st)
{
    size_t i;
    for (i = 0; i + 1 < p->cantidad; i++) {
        BundleVLIW *a = &p->bundles[i];
        BundleVLIW *b = &p->bundles[i + 1];
        OperacionVLIW *branch = &b->op[SLOT_BRU];
        int fuente = -1, rd_dummy;
        long off, nuevo;
        size_t dest;
        unsigned char lee[16], escribe[16];
        char mn[16];
        int bits;
        size_t j;

        if (cantidad_actuales(a) != 1 || cantidad_actuales(b) != 1 || !actual(branch))
            continue;
        if (actual(&a->op[SLOT_ALU]) && es_addi_constante_temporal(&a->op[SLOT_ALU], &rd_dummy))
            fuente = SLOT_ALU;
        else if (actual(&a->op[SLOT_LSU]) && es_store(&a->op[SLOT_LSU]))
            fuente = SLOT_LSU;
        else continue;
        if (!info_branch(branch, i + 1, p->cantidad, &off, &dest, lee, mn, sizeof(mn)))
            continue;
        if (targets[i] || targets[i + 1]) continue;
        escrituras_bundle(a, escribe);
        if (interseccion(lee, escribe)) continue;
        for (j = i >= 2 ? i - 2 : 0; j < i; j++) {
            escrituras_bundle(&p->bundles[j], escribe);
            if (interseccion(lee, escribe)) break;
        }
        if (j < i) continue;
        nuevo = off + 16;
        bits = strcmp(mn, "JMP") == 0 ? 20 : 12;
        if (nuevo < -(1L << (bits - 1)) || nuevo > (1L << (bits - 1)) - 1) continue;
        if ((int64_t)i * 16 + nuevo != (int64_t)(i + 1) * 16 + off) continue;
        if (!ajustar_branch(branch, nuevo)) continue;
        limpiar_nops_si_hay_operacion(a);
        mover_op(&a->op[SLOT_BRU], branch);
        poner_nop(b);
        if (fuente == SLOT_ALU) st->alu_bru++;
        else st->lsu_bru++;
    }
}

static void fase_feist(ProgramaVLIW *p, const unsigned char *targets,
                       EstadisticasVLIW *st)
{
    size_t i, j;
    for (i = 0; i + 1 < p->cantidad; i++) {
        BundleVLIW *a = &p->bundles[i];
        BundleVLIW *b = &p->bundles[i + 1];
        OperacionVLIW *f = &a->op[SLOT_FEIST];
        unsigned char fr[16], fw[16], br[16], bw[16], tmp[16];
        int peligro = 0;
        if (cantidad_actuales(a) != 1 || !actual(f)) continue;
        if (actual(&b->op[SLOT_FEIST]) || cantidad_actuales(b) == 0) continue;
        if (targets[i] || targets[i + 1] || feist_usa_x1(f)) continue;
        memcpy(fr, f->info.lee, 16);
        memcpy(fw, f->info.escribe, 16);
        lecturas_bundle(b, br);
        escrituras_bundle(b, bw);
        if (interseccion(fw, bw) || interseccion(fw, br) || interseccion(fr, bw)) continue;
        if (redireccion_pc_no_bru(b)) continue;
        for (j = i >= 3 ? i - 3 : 0; j < i; j++) {
            escrituras_bundle(&p->bundles[j], tmp);
            if (interseccion(fr, tmp)) { peligro = 1; break; }
        }
        if (peligro) continue;
        for (j = i + 2; j < p->cantidad && j < i + 4; j++) {
            lecturas_bundle(&p->bundles[j], tmp);
            if (interseccion(fw, tmp)) { peligro = 1; break; }
        }
        if (peligro) continue;
        if (feist_cambia_boveda(f)) {
            for (j = i + 2; j < p->cantidad && j < i + 4; j++)
                if (actual(&p->bundles[j].op[SLOT_FEIST])) { peligro = 1; break; }
        }
        if (peligro) continue;
        limpiar_nops_si_hay_operacion(b);
        mover_op(&b->op[SLOT_FEIST], f);
        poner_nop(a);
        st->feist_mix++;
        if (cantidad_actuales(b) == 4) st->cuatro_slots++;
    }
}

static int escribir_programa(const char *ruta, const ProgramaVLIW *p, FILE *err)
{
    FILE *f = fopen(ruta, "w");
    size_t i, e;
    int s, primero;
    if (f == NULL) {
        fprintf(err, "Empaquetador VLIW: no se pudo escribir %s\n", ruta);
        return 0;
    }
    for (i = 0; i <= p->cantidad; i++) {
        for (e = 0; e < p->cantidad_etiquetas; e++)
            if (p->etiquetas[e].bundle == i)
                fprintf(f, "%s:\n", p->etiquetas[e].nombre);
        if (i == p->cantidad) break;
        primero = 1;
        if (cantidad_actuales(&p->bundles[i]) == 0) {
            fprintf(f, "ALU: NOP\n");
            continue;
        }
        for (s = 0; s < SLOT_CANTIDAD; s++) {
            const OperacionVLIW *op = &p->bundles[i].op[s];
            if (!actual(op)) continue;
            if (!primero) fputs(" | ", f);
            fprintf(f, "%s: %s", nombre_slot((SlotVLIW)s), op->texto);
            primero = 0;
        }
        fputc('\n', f);
    }
    if (fclose(f) != 0) return 0;
    return 1;
}

int vliw_empaquetar_archivo(const char *entrada,
                            const char *salida,
                            FILE *reporte)
{
    ProgramaVLIW p = {0};
    EstadisticasVLIW st = {0};
    unsigned char *targets = NULL;
    FILE *rep = reporte != NULL ? reporte : stdout;
    int ok = 0;

    if (entrada == NULL || salida == NULL || strcmp(entrada, salida) == 0) {
        fprintf(stderr, "Empaquetador VLIW: entrada y salida deben ser distintas\n");
        return 0;
    }
    remove(salida);
    if (!leer_programa(entrada, &p, stderr)) goto fin;
    if (p.cantidad == 0) {
        fprintf(stderr, "Empaquetador VLIW: el programa no contiene bundles\n");
        goto fin;
    }
    targets = calloc(p.cantidad, 1u);
    if (targets == NULL) goto fin;
    marcar_destinos(&p, targets);

    fase_alu_lsu(&p, targets, &st);
    fase_bru(&p, targets, &st);
    fase_feist(&p, targets, &st);

    if (!escribir_programa(salida, &p, stderr)) goto fin;
    ok = 1;

    if (reporte != NULL) {
        fprintf(rep, "Empaquetado VLIW: %zu bundles conservados\n", p.cantidad);
        fprintf(rep, "  ALU + LSU: %zu\n", st.alu_lsu);
        fprintf(rep, "  ALU + BRU: %zu\n", st.alu_bru);
        fprintf(rep, "  LSU + BRU: %zu\n", st.lsu_bru);
        fprintf(rep, "  FEIST + otras unidades: %zu\n", st.feist_mix);
        fprintf(rep, "  Bundles con 4 slots activos: %zu\n", st.cuatro_slots);
    }

fin:
    if (!ok) remove(salida);
    free(targets);
    liberar_programa(&p);
    return ok;
}
