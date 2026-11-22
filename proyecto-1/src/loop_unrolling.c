#include "loop_unrolling.h"
#include "basic_blocks.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *old_name;
    char *new_name;
} TempRename;

typedef struct {
    size_t start;
    size_t condition_jump;
    size_t latch;
} LoopMatch;

static int eq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

/* En nuestra IR, los temporales internos llevan el prefijo %t. */
static int is_temp(const char *s)
{
    if (!s || s[0] != '%' || s[1] != 't' || !isdigit((unsigned char)s[2]))
        return 0;
    for (s += 2; *s; ++s)
        if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

/* Distingue destinos de datos de etiquetas de salto. */
static int is_pure_assignment(IROp op)
{
    switch (op) {
        case IR_COPY: case IR_ADD: case IR_SUB: case IR_MUL:
        case IR_DIV: case IR_MOD: case IR_POW: case IR_NEG:
        case IR_EQ: case IR_NE: case IR_LT: case IR_LE:
        case IR_GT: case IR_GE:
            return 1;
        default:
            return 0;
    }
}

static int emit_copy(IRProgram *out, const IRInstruction *v,
                     const char *a, const char *b, const char *r,
                     const char *extra)
{
    return ir_emit_ex(out, v->op, a, b, r, extra, v->source_line);
}

static int raw_copy(IRProgram *out, const IRInstruction *v)
{
    return emit_copy(out, v, v->arg1, v->arg2, v->result, v->extra);
}

static size_t containing_block(const BasicBlockCFG *cfg, size_t instruction)
{
    size_t i;
    for (i = 0; i < cfg->block_count; ++i)
        if (instruction >= cfg->blocks[i].start &&
            instruction <= cfg->blocks[i].end)
            return i;
    return SIZE_MAX;
}

static int has_backedge(const BasicBlockCFG *cfg, size_t start, size_t latch)
{
    size_t a = containing_block(cfg, latch);
    size_t b = containing_block(cfg, start);
    size_t i;
    if (a == SIZE_MAX || b == SIZE_MAX) return 0;
    for (i = 0; i < cfg->edge_count; ++i)
        if (cfg->edges[i].from == a && cfg->edges[i].to == b)
            return 1;
    return 0;
}

/* Encuentra ciclos de la forma:
   LABEL inicio; [evaluar condicion]; IF_FALSE cond, salida;
   [cuerpo sin saltos internos]; GOTO inicio; LABEL salida.
   Los casos complejos quedan intactos: nunca se transforman a ciegas. */
static int find_simple_loop(const IRProgram *p, const BasicBlockCFG *cfg,
                            size_t start, size_t max_body, size_t max_extra,
                            unsigned factor, LoopMatch *match)
{
    size_t i, branch = SIZE_MAX, latch = SIZE_MAX;
    const char *header, *exit_label;
    if (p->instructions[start].op != IR_LABEL) return 0;
    header = p->instructions[start].result;
    if (!header) return 0;

    for (i = start + 1; i + 1 < p->count; ++i) {
        const IRInstruction *v = &p->instructions[i];
        if (v->op == IR_IF_FALSE && branch == SIZE_MAX) {
            branch = i;
        } else if (v->op == IR_GOTO && eq(v->result, header)) {
            latch = i;
            break;
        } else if (!is_pure_assignment(v->op)) {
            return 0;
        }
    }
    if (branch == SIZE_MAX || latch == SIZE_MAX || branch <= start ||
        latch <= branch + 1 || latch + 1 >= p->count)
        return 0;
    if (p->instructions[latch + 1].op != IR_LABEL) return 0;
    exit_label = p->instructions[latch + 1].result;
    if (!eq(p->instructions[branch].result, exit_label)) return 0;
    if (!has_backedge(cfg, start, latch)) return 0;
    if (latch - branch - 1 > max_body) return 0;

    /* Solo soportamos un salto condicional de salida, sin saltos anidados. */
    for (i = start + 1; i < latch; ++i) {
        const IRInstruction *v = &p->instructions[i];
        if (i == branch) continue;
        if (!is_pure_assignment(v->op)) return 0;
    }

    /* Evitar sobrecrecimiento y desbordamientos. */
    if (latch - start - 1 > max_extra / (factor - 1)) return 0;

    /* Cada temporal interno debe definirse una unica vez, antes de usarse.
       Si es un valor que atraviesa iteraciones, no es seguro renombrarlo. */
    for (i = start + 1; i < latch; ++i) {
        const IRInstruction *v = &p->instructions[i];
        size_t j;
        if (!is_pure_assignment(v->op) || !is_temp(v->result)) continue;
        for (j = start + 1; j < latch; ++j) {
            const IRInstruction *w = &p->instructions[j];
            if (j != i && is_pure_assignment(w->op) && eq(w->result, v->result))
                return 0;
            if (j <= i && (eq(w->arg1, v->result) || eq(w->arg2, v->result) ||
                           eq(w->extra, v->result)))
                return 0;
        }
        /* Un temporal definido en el ciclo no debe usarse fuera del ciclo. */
        for (j = 0; j < p->count; ++j) {
            const IRInstruction *w = &p->instructions[j];
            if (j >= start + 1 && j < latch) continue;
            if (eq(w->arg1, v->result) || eq(w->arg2, v->result) ||
                eq(w->extra, v->result)) return 0;
        }
    }
    match->start = start;
    match->condition_jump = branch;
    match->latch = latch;
    return 1;
}

/* Reconoce nombres de temporales incluso si la IR se construyo a mano
   sin actualizar el contador next_temp. */
static void check_temp_counter(const char *name, size_t *next)
{
    size_t number = 0;
    const unsigned char *p;
    if (!is_temp(name)) return;
    for (p = (const unsigned char *)(name + 2); *p; ++p) {
        size_t digit = (size_t)(*p - '0');
        if (number > (SIZE_MAX - digit) / 10) return;
        number = number * 10 + digit;
    }
    if (number < SIZE_MAX && *next <= number) *next = number + 1;
}

static const char *renamed(const char *s, const TempRename *map, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (eq(s, map[i].old_name)) return map[i].new_name;
    return s;
}

static void free_renames(TempRename *map, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) free(map[i].new_name);
    free(map);
}

static int emit_iteration(const IRProgram *input, IRProgram *output,
                          const LoopMatch *loop, int first)
{
    TempRename *map = NULL;
    size_t map_count = 0, capacity = loop->latch - loop->start;
    size_t i;
    if (!first) {
        map = calloc(capacity, sizeof(*map));
        if (!map) return 0;
        for (i = loop->start + 1; i < loop->latch; ++i) {
            const IRInstruction *v = &input->instructions[i];
            if (is_pure_assignment(v->op) && is_temp(v->result)) {
                map[map_count].old_name = v->result;
                map[map_count].new_name = ir_new_temp(output);
                if (!map[map_count].new_name) {
                    free_renames(map, map_count + 1);
                    return 0;
                }
                ++map_count;
            }
        }
    }
    for (i = loop->start + 1; i < loop->latch; ++i) {
        const IRInstruction *v = &input->instructions[i];
        const char *a = first ? v->arg1 : renamed(v->arg1, map, map_count);
        const char *b = first ? v->arg2 : renamed(v->arg2, map, map_count);
        const char *r = first ? v->result :
                        (is_pure_assignment(v->op) ? renamed(v->result, map, map_count) : v->result);
        const char *e = first ? v->extra : renamed(v->extra, map, map_count);
        if (!emit_copy(output, v, a, b, r, e)) {
            free_renames(map, map_count);
            return 0;
        }
    }
    free_renames(map, map_count);
    return 1;
}

int loop_unroll(const IRProgram *input, IRProgram *output,
                const LoopUnrollConfig *config, LoopUnrollStats *stats)
{
    IRProgram generated;
    BasicBlockCFG *cfg = NULL, *verified = NULL;
    LoopUnrollStats record = {0};
    unsigned factor = config ? config->factor : 2;
    size_t max_body = (config && config->max_body_instructions) ?
                       config->max_body_instructions : 32;
    size_t max_extra = (config && config->max_extra_instructions) ?
                        config->max_extra_instructions : 256;
    size_t i, j;
    if (!input || !output || input == output ||
        (input->count && !input->instructions) || factor < 2 || factor > 16)
        return 0;

    ir_init(&generated);
    generated.next_temp = input->next_temp;
    generated.next_label = input->next_label;
    record.instructions_before = input->count;
    for (i = 0; i < input->count; ++i) {
        const IRInstruction *v = &input->instructions[i];
        check_temp_counter(v->arg1, &generated.next_temp);
        check_temp_counter(v->arg2, &generated.next_temp);
        check_temp_counter(v->result, &generated.next_temp);
        check_temp_counter(v->extra, &generated.next_temp);
    }
    cfg = bb_build(input);
    if (!cfg || !bb_build_cfg(cfg)) goto fail;

    /* Contamos los saltos hacia atras, incluso los que luego se rechazan. */
    for (i = 0; i < input->count; ++i) {
        const IRInstruction *v = &input->instructions[i];
        if (v->op != IR_GOTO || !v->result) continue;
        for (j = 0; j < i; ++j) {
            const IRInstruction *l = &input->instructions[j];
            if (l->op == IR_LABEL && eq(l->result, v->result)) {
                ++record.loops_found;
                break;
            }
        }
    }

    for (i = 0; i < input->count;) {
        LoopMatch match;
        if (input->instructions[i].op == IR_LABEL &&
            find_simple_loop(input, cfg, i, max_body, max_extra, factor, &match)) {
            unsigned rep;
            if (!raw_copy(&generated, &input->instructions[i])) goto fail;
            for (rep = 0; rep < factor; ++rep)
                if (!emit_iteration(input, &generated, &match, rep == 0)) goto fail;
            if (!raw_copy(&generated, &input->instructions[match.latch])) goto fail;
            ++record.loops_unrolled;
            i = match.latch + 1;
        } else {
            if (!raw_copy(&generated, &input->instructions[i])) goto fail;
            ++i;
        }
    }

    /* Validamos los destinos de salto despues de transformar. */
    verified = bb_build(&generated);
    if (!verified || !bb_build_cfg(verified)) goto fail;
    bb_free(verified);
    bb_free(cfg);
    record.instructions_after = generated.count;
    record.loops_skipped = record.loops_found > record.loops_unrolled ?
                           record.loops_found - record.loops_unrolled : 0;
    ir_free(output);
    *output = generated;
    if (stats) *stats = record;
    return 1;

fail:
    bb_free(verified);
    bb_free(cfg);
    ir_free(&generated);
    return 0;
}
