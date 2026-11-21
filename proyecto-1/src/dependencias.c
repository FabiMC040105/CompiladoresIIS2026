#include "dependencias.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const unsigned campos[] = { DEP_ARG1, DEP_ARG2, DEP_RESULT, DEP_EXTRA };
#define N_CAMPOS (sizeof(campos) / sizeof(campos[0]))

const char *dep_campo(const IRInstruction *inst, unsigned campo)
{
    if (!inst) return NULL;
    switch (campo) {
        case DEP_ARG1: return inst->arg1;
        case DEP_ARG2: return inst->arg2;
        case DEP_RESULT: return inst->result;
        case DEP_EXTRA: return inst->extra;
        default: return NULL;
    }
}

/* Identificadores del lexer y temporales internos %tN de ir_new_temp(). */
int dep_es_nombre(const char *texto)
{
    const unsigned char *s = (const unsigned char *)texto;
    if (!s || !*s) return 0;
    if (s[0] == '%' && s[1] == 't') {
        s += 2;
        if (!isdigit(*s)) return 0;
        while (isdigit(*s)) ++s;
        return *s == '\0';
    }
    if (*s < 'a' || *s > 'z') return 0;
    if (!strcmp(texto, "verdadero") || !strcmp(texto, "falso")) return 0;
    for (++s; *s; ++s)
        if (!isalnum(*s) && *s != '_') return 0;
    return 1;
}

static int es_constante(const char *texto)
{
    const unsigned char *s = (const unsigned char *)texto;
    if (!s || !*s) return 0;
    if (!strcmp(texto, "verdadero") || !strcmp(texto, "falso")) return 1;
    /* El lexer actual acepta enteros; se tolera signo para IR transformada. */
    if (*s == '+' || *s == '-') ++s;
    if (!isdigit(*s)) return 0;
    while (isdigit(*s)) ++s;
    return *s == '\0';
}

int dep_accesos(const IRInstruction *inst, DepAccesos *out)
{
    unsigned r = 0, w = 0, opcionales = 0;
    if (!out) return 0;
    *out = (DepAccesos){0};
    if (!inst) return 0;

    switch (inst->op) {
        case IR_COPY: case IR_NEG:
            r = DEP_ARG1; 
            w = DEP_RESULT; 
            break;
        case IR_ADD: case IR_SUB: case IR_MUL:
        case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
            r = DEP_ARG1 | DEP_ARG2; 
            w = DEP_RESULT; 
            break;
        case IR_DIV: case IR_MOD: case IR_POW:
            r = DEP_ARG1 | DEP_ARG2; 
            w = DEP_RESULT;
            /* Sin prueba de ausencia de errores, no alterar su orden. */
            out->barrera = 1; break;
        case IR_DECL:
            out->declaracion = DEP_RESULT; 
            out->barrera = 1; break;
        case IR_PARAM_DECL:
            r = DEP_ARG2 | DEP_EXTRA; 
            opcionales = r;
            w = DEP_RESULT; 
            out->declaracion = DEP_RESULT;
            out->barrera = 1; break;
        case IR_LABEL: case IR_GOTO: case IR_FUNC_BEGIN: case IR_FUNC_END:
            out->barrera = 1; 
            break;
        case IR_IF_FALSE: case IR_IF_TRUE: case IR_PARAM:
            r = DEP_ARG1; 
            out->barrera = 1;
            break;
        case IR_RETURN:
            r = DEP_ARG1; 
            opcionales = r; 
            out->barrera = 1; 
            break;
        case IR_CALL:
            /* arg1 es la funcion y arg2 es la cantidad de argumentos. */
            w = DEP_RESULT; 
            opcionales = w;
            out->lee_memoria = out->escribe_memoria = out->barrera = 1; 
            break;
        case IR_ARRAY_DECL: case IR_MATRIX_DECL:
            r = DEP_ARG1 | (inst->op == IR_MATRIX_DECL ? DEP_ARG2 : 0u);
            out->declaracion = DEP_RESULT;
            out->escribe_memoria = out->barrera = 1; 
            break;
        case IR_ARRAY_LOAD: case IR_MATRIX_LOAD:
            r = DEP_ARG1 | DEP_ARG2 | (inst->op == IR_MATRIX_LOAD ? DEP_EXTRA : 0u);
            w = DEP_RESULT;
            out->lee_memoria = out->barrera = 1;
            break;
        case IR_ARRAY_STORE: case IR_MATRIX_STORE:
            /* result es la base, NO una nueva definicion escalar. */
            r = DEP_ARG1 | DEP_ARG2 | DEP_RESULT |
                (inst->op == IR_MATRIX_STORE ? DEP_EXTRA : 0u);
            out->escribe_memoria = out->barrera = 1; 
            break;
        default: return 0;
    }
    for (size_t k = 0; k < N_CAMPOS; ++k) {
        unsigned c = campos[k];
        const char *v = dep_campo(inst, c);
        if ((opcionales & c) && !v) continue;
        if (r & c) {
            if (dep_es_nombre(v)) out->lecturas |= c;
            else if (!es_constante(v)) return 0;
        }
        if (w & c) {
            if (!dep_es_nombre(v)) return 0;
            out->escrituras |= c;
        }
        if ((out->declaracion & c) && !dep_es_nombre(v)) return 0;
    }
    return 1;
}

static int contiene(const IRInstruction *inst, unsigned mask, const char *nombre)
{
    if (!nombre) return 0;
    for (size_t k = 0; k < N_CAMPOS; ++k)
        if ((mask & campos[k]) && !strcmp(dep_campo(inst, campos[k]), nombre)) return 1;
    return 0;
}

static int intersectan(const IRInstruction *i, unsigned a,
                       const IRInstruction *j, unsigned b)
{
    for (size_t k = 0; k < N_CAMPOS; ++k)
        if ((a & campos[k]) && contiene(j, b, dep_campo(i, campos[k]))) return 1;
    return 0;
}

static unsigned restricciones(const DepAccesos *a, const DepAccesos *b)
{
    unsigned tipos = 0;
    if ((a->escribe_memoria && (b->lee_memoria || b->escribe_memoria)) ||
        (a->lee_memoria && b->escribe_memoria)) tipos |= DEP_MEMORIA;
    if (a->barrera || b->barrera) tipos |= DEP_BARRERA;
    return tipos;
}

unsigned dep_tipos_par(const IRInstruction *i, const IRInstruction *j)
{
    DepAccesos a, b;
    unsigned tipos;
    if (!dep_accesos(i, &a) || !dep_accesos(j, &b)) return 0;
    tipos = restricciones(&a, &b);
    if (intersectan(i, a.escrituras, j, b.lecturas)) tipos |= DEP_RAW;
    if (intersectan(i, a.lecturas, j, b.escrituras)) tipos |= DEP_WAR;
    if (intersectan(i, a.escrituras, j, b.escrituras)) tipos |= DEP_WAW;
    return tipos;
}

static int agregar(DepGrafo *g, size_t i, size_t j, DepTipo tipo, const char *nombre)
{
    if (g->cantidad == g->capacidad) {
        size_t cap = g->capacidad ? g->capacidad * 2 : 16;
        if (cap < g->capacidad || cap > SIZE_MAX / sizeof(*g->aristas)) return 0;
        DepArista *nuevo = realloc(g->aristas, cap * sizeof(*g->aristas));
        if (!nuevo) return 0;
        g->aristas = nuevo; g->capacidad = cap;
    }
    g->aristas[g->cantidad++] = (DepArista){i, j, tipo, nombre};
    return 1;
}

/* Evitar duplicados cuando una instruccion lee el mismo nombre dos veces. */
static int repetido(const IRInstruction *inst, unsigned mask, size_t k)
{
    const char *nombre = dep_campo(inst, campos[k]);
    for (size_t t = 0; t < k; ++t)
        if ((mask & campos[t]) && !strcmp(nombre, dep_campo(inst, campos[t]))) return 1;
    return 0;
}

int dep_analizar_bloque(const IRProgram *p, const BasicBlock *b, DepGrafo *out)
{
    if (!out) return 0;
    *out = (DepGrafo){0};
    if (!p || !b || !p->instructions || b->start > b->end || b->end >= p->count) return 0;
    const size_t n = b->end - b->start + 1;
    if (n > SIZE_MAX / sizeof(DepAccesos)) return 0;
    DepAccesos *acc = calloc(n, sizeof(*acc));
    if (!acc) return 0;
    for (size_t j = 0; j < n; ++j)
        if (!dep_accesos(&p->instructions[b->start + j], &acc[j])) goto error;

    for (size_t j = 0; j < n; ++j) {
        const size_t dst = b->start + j;
        const IRInstruction *actual = &p->instructions[dst];
        /* RAW: buscar la ultima definicion anterior de cada lectura. */
        for (size_t k = 0; k < N_CAMPOS; ++k) {
            if (!(acc[j].lecturas & campos[k]) || repetido(actual, acc[j].lecturas, k)) continue;
            const char *nombre = dep_campo(actual, campos[k]);
            for (size_t i = j; i > 0;) {
                --i;
                if (contiene(&p->instructions[b->start+i], acc[i].escrituras, nombre)) {
                    if (!agregar(out, b->start+i, dst, DEP_RAW, nombre)) goto error;
                    break;
                }
            }
        }
        /* WAR: lecturas desde la ultima definicion, sin incluirla.
           WAW: ultima escritura (una lectura en ella usa el valor previo). */
        for (size_t k = 0; k < N_CAMPOS; ++k) {
            if (!(acc[j].escrituras & campos[k]) || repetido(actual, acc[j].escrituras, k)) continue;
            const char *nombre = dep_campo(actual, campos[k]);
            for (size_t i = j; i > 0;) {
                --i;
                const IRInstruction *prev = &p->instructions[b->start+i];
                if (contiene(prev, acc[i].escrituras, nombre)) {
                    if (!agregar(out, b->start+i, dst, DEP_WAW, nombre)) goto error;
                    break;
                }
                if (contiene(prev, acc[i].lecturas, nombre) &&
                    !agregar(out, b->start+i, dst, DEP_WAR, nombre)) goto error;
            }
        }
        /* Conservador: memoria posiblemente aliasada y barreras completas. */
        for (size_t i = 0; i < j; ++i) {
            unsigned tipos = restricciones(&acc[i], &acc[j]);
            if ((tipos & DEP_MEMORIA) && !agregar(out, b->start+i, dst, DEP_MEMORIA, NULL)) goto error;
            if ((tipos & DEP_BARRERA) && !agregar(out, b->start+i, dst, DEP_BARRERA, NULL)) goto error;
        }
    }
    free(acc);
    return 1;
error:
    free(acc);
    dep_liberar(out);
    return 0;
}

void dep_liberar(DepGrafo *g)
{
    if (!g) return;
    free(g->aristas);
    *g = (DepGrafo){0};
}

const char *dep_tipo_nombre(DepTipo tipo)
{
    switch (tipo) {
        case DEP_RAW: return "RAW";
        case DEP_WAR: return "WAR";
        case DEP_WAW: return "WAW";
        case DEP_MEMORIA: return "MEMORIA";
        case DEP_BARRERA: return "BARRERA";
        default: return "DESCONOCIDA";
    }
}

void dep_imprimir(const DepGrafo *g, FILE *out)
{
    if (!g || !out) return;
    size_t raw = 0, war = 0, waw = 0, mem = 0, barr = 0;
    for (size_t k = 0; k < g->cantidad; ++k) {
        const DepArista *a = &g->aristas[k];
        fprintf(out, "I%zu -> I%zu : %s", a->origen, a->destino, dep_tipo_nombre(a->tipo));
        if (a->nombre) fprintf(out, " (%s)", a->nombre);
        fputc('\n', out);
        switch (a->tipo) {
            case DEP_RAW: ++raw; break; case DEP_WAR: ++war; break;
            case DEP_WAW: ++waw; break; case DEP_MEMORIA: ++mem; break;
            case DEP_BARRERA: ++barr; break;
        }
    }
    fprintf(out, "RAW=%zu WAR=%zu WAW=%zu MEMORIA=%zu BARRERA=%zu\n", raw, war, waw, mem, barr);
}
