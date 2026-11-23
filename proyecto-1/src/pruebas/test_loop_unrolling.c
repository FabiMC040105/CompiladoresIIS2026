#include "../loop_unrolling.h"
#include "../basic_blocks.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Memoria sencilla para simular las variables y sus valores */
typedef struct { char name[64]; long value; } Cell;
typedef struct { Cell cells[256]; size_t count; } Memory;

/* Leo un numero directo o busco una variable en la memoria de prueba */
static long read_value(Memory *mem, const char *operand)
{
    char *end;
    long n;
    size_t i;
    if (!operand) return 0;
    n = strtol(operand, &end, 10);
    if (*operand && *end == '\0') return n;
    for (i = 0; i < mem->count; ++i)
        if (strcmp(mem->cells[i].name, operand) == 0)
            return mem->cells[i].value;
    return 0;
}

/* Actualizo el valor de una variable o la agrego si no existe */
static void write_value(Memory *mem, const char *name, long value)
{
    size_t i;
    assert(name);
    for (i = 0; i < mem->count; ++i)
        if (strcmp(mem->cells[i].name, name) == 0) {
            mem->cells[i].value = value;
            return;
        }
    assert(mem->count < 256);
    assert(strlen(name) < sizeof(mem->cells[0].name));
    strcpy(mem->cells[mem->count].name, name);
    mem->cells[mem->count++].value = value;
}

/* Busco la etiqueta a la que tiene que saltar el interprete */
static size_t label_position(const IRProgram *p, const char *label)
{
    size_t i;
    for (i = 0; i < p->count; ++i)
        if (p->instructions[i].op == IR_LABEL &&
            p->instructions[i].result &&
            strcmp(p->instructions[i].result, label) == 0) return i;
    assert(!"Etiqueta inexistente");
    return 0;
}

/* Interprete pequeño para ejecutar las instrucciones que pruebo aqui
   No reemplaza al procesador VLIW, solo compara resultados de la IR */
static long run_program(const IRProgram *p)
{
    Memory mem = {0};
    size_t pc = 0, steps = 0;
    while (pc < p->count) {
        const IRInstruction *v = &p->instructions[pc];
        long a = read_value(&mem, v->arg1);
        long b = read_value(&mem, v->arg2);
        long value = 0;
        assert(++steps < 100000);
        switch (v->op) {
            case IR_FUNC_BEGIN: case IR_FUNC_END:
            case IR_DECL: case IR_LABEL:
                break;
            case IR_COPY:
                write_value(&mem, v->result, a); break;
            case IR_ADD:
                write_value(&mem, v->result, a + b); break;
            case IR_SUB:
                write_value(&mem, v->result, a - b); break;
            case IR_MUL:
                write_value(&mem, v->result, a * b); break;
            case IR_LT:
                write_value(&mem, v->result, a < b); break;
            case IR_GOTO:
                pc = label_position(p, v->result); continue;
            case IR_IF_FALSE:
                if (!a) { pc = label_position(p, v->result); continue; }
                break;
            case IR_IF_TRUE:
                if (a) { pc = label_position(p, v->result); continue; }
                break;
            case IR_RETURN:
                return a;
            default:
                fprintf(stderr, "Opcode inesperado %d\n", (int)v->op);
                assert(0);
        }
        (void)value;
        ++pc;
    }
    assert(!"El programa no retorno");
    return 0;
}

/* Construyo un ciclo en IR para probarlo sin depender del parser
   Puedo cambiar el limite, usar una variable o agregar un salto interno */
static void create_loop(IRProgram *p, long n, int variable_bound, int early_exit)
{
    char limit[32];
    ir_init(p);
    snprintf(limit, sizeof(limit), "%ld", n);
#define ADD(op,a,b,r) assert(ir_emit(p,(op),(a),(b),(r),1))
    ADD(IR_FUNC_BEGIN, NULL, NULL, "principal");
    ADD(IR_COPY, "0", NULL, "i");
    ADD(IR_COPY, "0", NULL, "suma");
    if (variable_bound) ADD(IR_COPY, limit, NULL, "n");
    ADD(IR_LABEL, NULL, NULL, "@L0");
    ADD(IR_LT, "i", variable_bound ? "n" : limit, "%t0");
    ADD(IR_IF_FALSE, "%t0", NULL, "@L1");
    if (early_exit) ADD(IR_IF_TRUE, "detener", NULL, "@L1");
    ADD(IR_ADD, "suma", "i", "%t1");
    ADD(IR_COPY, "%t1", NULL, "suma");
    ADD(IR_ADD, "i", "1", "%t2");
    ADD(IR_COPY, "%t2", NULL, "i");
    ADD(IR_GOTO, NULL, NULL, "@L0");
    ADD(IR_LABEL, NULL, NULL, "@L1");
    ADD(IR_RETURN, "suma", NULL, NULL);
    ADD(IR_FUNC_END, NULL, NULL, "principal");
#undef ADD
}

/* Corro la IR original y la nueva para revisar que den la misma suma
   Tambien reviso si la heuristica desenrollo o conservo el ciclo */
static void test_equivalent(long n, int variable_bound, unsigned factor, int show)
{
    IRProgram original, optimized;
    LoopUnrollConfig cfg = {factor, 32, 256};
    LoopUnrollStats stats;
    BasicBlockCFG *graph;
    long expected = n * (n - 1) / 2;
    create_loop(&original, n, variable_bound, 0);
    ir_init(&optimized);
    assert(run_program(&original) == expected);
    assert(loop_unroll(&original, &optimized, &cfg, &stats) == 1);
    /* Los ciclos constantes de 0 o 1 vueltas se dejan como estaban */
    int should_unroll = variable_bound || n >= (long)factor;

    assert(stats.loops_found == 1);
    assert(stats.loops_unrolled == (size_t)should_unroll);
    if (should_unroll)
        assert(optimized.count > original.count);
    else
        assert(optimized.count == original.count);
    assert(run_program(&optimized) == expected);
    graph = bb_build(&optimized);
    assert(graph && bb_build_cfg(graph));
    bb_free(graph);
    printf("OK: n=%ld, factor=%u, antes=%zu, despues=%zu\n",
           n, factor, stats.instructions_before, stats.instructions_after);
    if (show) {
        puts("\n--- IR ORIGINAL ---"); ir_print(&original, stdout);
        puts("\n--- IR DESENROLLADA ---"); ir_print(&optimized, stdout);
    }
    ir_free(&optimized);
    ir_free(&original);
}

/* Pruebo que un rompa o un cuerpo demasiado grande no se copien */
static void test_unsafe_and_heuristic(void)
{
    IRProgram original, result;
    LoopUnrollConfig cfg = {4, 32, 256};
    LoopUnrollStats stats;
    create_loop(&original, 6, 0, 1);
    ir_init(&result);
    assert(loop_unroll(&original, &result, &cfg, &stats));
    assert(stats.loops_found == 1 && stats.loops_unrolled == 0);
    assert(result.count == original.count);
    assert(run_program(&result) == run_program(&original));
    puts("OK: ciclo con salto interno rechazado de forma segura");
    ir_free(&result);
    ir_free(&original);

    create_loop(&original, 6, 0, 0);
    cfg.max_body_instructions = 1;
    assert(loop_unroll(&original, &result, &cfg, &stats));
    assert(stats.loops_unrolled == 0 && stats.loops_skipped == 1);
    puts("OK: heuristica de tamano conserva ciclo original");
    ir_free(&result);
    ir_free(&original);
}

/* Casos que usamos para ver limites, sobrantes y ciclos grandes */
int main(void)
{
    test_equivalent(0, 0, 4, 0);    /* No entra al ciclo */
    test_equivalent(1, 0, 4, 0);    /* Solo una iteracion */
    test_equivalent(6, 0, 2, 1);    /* Factor 2 y limite constante */
    test_equivalent(5, 1, 4, 0);    /* Limite en variable, queda sobrante */
    test_equivalent(13, 0, 4, 0);   /* No es multiplo de 4 */
    test_equivalent(1000, 1, 8, 0); /* Muchas iteraciones */
    test_unsafe_and_heuristic();    /* Casos que no quiero modificar */
    puts("PASS: Loop Unrolling y equivalencia de resultados");
    return 0;
}
