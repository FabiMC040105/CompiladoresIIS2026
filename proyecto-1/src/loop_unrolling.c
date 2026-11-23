#include "loop_unrolling.h"
#include "basic_blocks.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Guardo el temporal original y el nombre nuevo de cada copia */
typedef struct {
    const char *old_name;
    char *new_name;
} TempRename;

/* Las posiciones que necesito para encontrar el inicio y el final del ciclo */
typedef struct {
    size_t start;
    size_t condition_jump;
    size_t latch;
} LoopMatch;

/* Comparo cadenas sin intentar leer un puntero nulo */
static int eq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

/* Reviso si un nombre corresponde a un temporal nuestro, como %t3 */
static int is_temp(const char *s)
{
    if (!s || s[0] != '%' || s[1] != 't' || !isdigit((unsigned char)s[2]))
        return 0;
    for (s += 2; *s; ++s)
        if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

/* Solo tomo operaciones normales que producen un resultado
   Los saltos y las escrituras a memoria necesitan otro tratamiento */
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


/*
   Esta es una primera regla para no agrandar ciclos muy cortos
   Solo intenta reconocer una comparacion i < N con N constante
   y busca que i se haya inicializado en 0 antes de entrar al ciclo
   Si no reconoce ese caso deja que el resto del analisis decida
*/
static int skip_short_constant_loop(
    const IRProgram *p,
    size_t start,
    size_t branch,
    unsigned factor)
{
    const IRInstruction *cmp;
    char *end;
    long bound;
    size_t j;

    if (branch <= start + 1)
        return 0;

    cmp = &p->instructions[branch - 1];

    if (cmp->op != IR_LT ||
        !eq(cmp->result, p->instructions[branch].arg1) ||
        !cmp->arg1 || !cmp->arg2)
        return 0;

    errno = 0;
    bound = strtol(cmp->arg2, &end, 10);

    if (errno || end == cmp->arg2 || *end != '\0' ||
        bound < 0 || (unsigned long)bound >= factor)
        return 0;

    /* Busco la ultima asignacion al contador antes del ciclo */
    for (j = start; j > 0;)
    {
        const IRInstruction *v = &p->instructions[--j];

        if (v->op == IR_FUNC_BEGIN ||
            v->op == IR_LABEL ||
            v->op == IR_GOTO ||
            v->op == IR_IF_TRUE ||
            v->op == IR_IF_FALSE)
            break;

        if (v->op == IR_COPY && eq(v->result, cmp->arg1))
            return eq(v->arg1, "0");

        if (is_pure_assignment(v->op) &&
            eq(v->result, cmp->arg1))
            break;
    }

    return 0;
}

/* Agrego una instruccion copiando sus datos y la linea de origen
   ir_emit_ex reserva la memoria de las cadenas por nosotros */
static int emit_copy(IRProgram *out, const IRInstruction *v,
                     const char *a, const char *b, const char *r,
                     const char *extra)
{
    /* Conservamos la información del AST al copiar la instrucción.
       El backend necesita este nodo para identificar correctamente
       las variables locales y sus ámbitos. */
    if (!ir_emit_ex(out, v->op, a, b, r, extra, v->source_line))
        return 0;

    out->instructions[out->count - 1].nodo_origen = v->nodo_origen;

    return 1;
}

/* Esta copia queda igual, no hace renombramiento */
static int raw_copy(IRProgram *out, const IRInstruction *v)
{
    return emit_copy(out, v, v->arg1, v->arg2, v->result, v->extra);
}

/* Busco a que bloque basico pertenece una instruccion de la IR */
static size_t containing_block(const BasicBlockCFG *cfg, size_t instruction)
{
    size_t i;
    for (i = 0; i < cfg->block_count; ++i)
        if (instruction >= cfg->blocks[i].start &&
            instruction <= cfg->blocks[i].end)
            return i;
    return SIZE_MAX;
}

/* Confirmo en el CFG que hay una conexion del final al inicio
   Sin ese regreso no tratamos el fragmento como un ciclo */
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

/* Busco ciclos con la forma que genera actualmente ir_ast.c
   Etiqueta, condicion, cuerpo, salto al inicio y etiqueta de salida
   Si tiene rompa, saltos internos o algo que no manejo lo dejo igual */
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


    /* No expando los ciclos cortos que la heuristica reconoce */
    if (skip_short_constant_loop(p, start, branch, factor))
        return 0;
    if (latch - branch - 1 > max_body) return 0;

    /* No puedo copiar a ciegas un cuerpo con saltos internos */
    for (i = start + 1; i < latch; ++i) {
        const IRInstruction *v = &p->instructions[i];
        if (i == branch) continue;
        if (!is_pure_assignment(v->op)) return 0;
    }

    /* Evito que el codigo crezca demasiado segun el factor elegido */
    if (latch - start - 1 > max_extra / (factor - 1)) return 0;

    /* Reviso los temporales para no cambiar valores que se usan
       en otra iteracion o fuera del ciclo */
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
        /* Un temporal del ciclo no puede necesitarse despues de salir */
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

/* Reviso el numero mayor de los temporales existentes
   Asi los nombres nuevos no chocan con los que ya tenia la IR */
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

/* Si un temporal tiene nombre nuevo uso ese, si no lo dejo igual */
static const char *renamed(const char *s, const TempRename *map, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (eq(s, map[i].old_name)) return map[i].new_name;
    return s;
}

/* Libero los nombres que cree al duplicar una iteracion */
static void free_renames(TempRename *map, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) free(map[i].new_name);
    free(map);
}

/* Camino rápido: únicamente para i=0; i<N; i=i+1, N literal y múltiplo
   del factor. En ese caso sólo hace falta comprobar el límite al comienzo
   de cada grupo; no hay iteraciones sobrantes ni saltos dentro del cuerpo. */
static int can_group_without_checks(const IRProgram *p,
                                    const LoopMatch *loop, unsigned factor)
{
    if (loop->condition_jump <= loop->start + 1 || factor < 2) return 0;
    const IRInstruction *cmp = &p->instructions[loop->condition_jump - 1];
    const IRInstruction *br = &p->instructions[loop->condition_jump];
    if (cmp->op != IR_LT || !eq(cmp->result, br->arg1) ||
        !cmp->arg1 || !cmp->arg2) return 0;

    char *end = NULL;
    errno = 0;
    long bound = strtol(cmp->arg2, &end, 10);
    if (errno || end == cmp->arg2 || *end ||
        bound < (long)factor * 3L || bound > INT32_MAX ||
        bound % (long)factor) return 0;

    const char *counter = cmp->arg1;
    int initialized = 0;
    for (size_t j = loop->start; j > 0;) {
        const IRInstruction *v = &p->instructions[--j];
        if (v->op == IR_LABEL || v->op == IR_FUNC_BEGIN ||
            v->op == IR_GOTO || v->op == IR_IF_TRUE ||
            v->op == IR_IF_FALSE) break;
        if (is_pure_assignment(v->op) && eq(v->result, counter)) {
            initialized = v->op == IR_COPY && eq(v->arg1, "0");
            break;
        }
    }
    if (!initialized) return 0;

    /* No aceptar entradas alternativas al encabezado. */
    const char *header = p->instructions[loop->start].result;
    for (size_t j = 0; j < p->count; ++j) {
        const IRInstruction *v = &p->instructions[j];
        if (j == loop->latch) continue;
        if ((v->op == IR_GOTO || v->op == IR_IF_TRUE ||
             v->op == IR_IF_FALSE) && eq(v->result, header)) return 0;
    }

    /* El cuerpo debe actualizar el contador exactamente una vez,
       mediante suma de uno seguida de una copia, y sin otras escrituras. */
    int increments = 0;
    for (size_t j = loop->start + 1; j < loop->latch; ++j) {
        const IRInstruction *v = &p->instructions[j];
        if (!is_pure_assignment(v->op) || !eq(v->result, counter)) continue;
        if (j <= loop->condition_jump + 1 || v->op != IR_COPY) return 0;
        const IRInstruction *prev = &p->instructions[j-1];
        if (prev->op != IR_ADD || !eq(prev->result, v->arg1) ||
            !((eq(prev->arg1, counter) && eq(prev->arg2, "1")) ||
              (eq(prev->arg2, counter) && eq(prev->arg1, "1")))) return 0;
        ++increments;
    }
    return increments == 1;
}

/* Copio una iteracion completa del ciclo
   La primera mantiene sus temporales y las otras crean nombres nuevos
   El orden de las instrucciones se conserva */
static int emit_iteration(const IRProgram *input, IRProgram *output,
                          const LoopMatch *loop, int first, int skip_condition)
{
    TempRename *map = NULL;
    size_t map_count = 0, capacity = loop->latch - loop->start;
    size_t i;
    size_t begin = skip_condition ? loop->condition_jump + 1 : loop->start + 1;
    if (!first) {
        map = calloc(capacity, sizeof(*map));
        if (!map) return 0;
        for (i = begin; i < loop->latch; ++i) {
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
    for (i = begin; i < loop->latch; ++i) {
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

/* Esta es la funcion principal que van a llamar desde el compilador
   Recibe una IR, el factor y los limites de crecimiento
   Construye otra IR para no cambiar la original
   Al final valida el nuevo CFG y guarda las estadisticas */
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

    /* Cuento los ciclos posibles aunque despues alguno sea omitido */
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

    /* Recorro la IR, duplico los ciclos seguros y copio el resto sin cambios */
    for (i = 0; i < input->count;) {
        LoopMatch match;
        if (input->instructions[i].op == IR_LABEL &&
            find_simple_loop(input, cfg, i, max_body, max_extra, factor, &match)) {
            unsigned rep;
            int fast_path = can_group_without_checks(input, &match, factor);

            if (!raw_copy(&generated, &input->instructions[i])) goto fail;
            for (rep = 0; rep < factor; ++rep)
                if (!emit_iteration(input, &generated, &match, rep == 0,
                                    fast_path && rep > 0)) goto fail;
            if (!raw_copy(&generated, &input->instructions[match.latch])) goto fail;
            ++record.loops_unrolled;
            i = match.latch + 1;
        } else {
            if (!raw_copy(&generated, &input->instructions[i])) goto fail;
            ++i;
        }
    }

    /* Armo el CFG otra vez para revisar que los saltos sigan apuntando bien */
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
    /* Si algo falla libero lo temporal y no entrego una IR incompleta */
    bb_free(verified);
    bb_free(cfg);
    ir_free(&generated);
    return 0;
}
