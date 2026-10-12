#include "scheduler.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static int registro_valido(int reg)
{
    return reg >= 0 && reg < SCHEDULER_REGISTROS;
}

/* x0 es constante y x1 representa el PC, no un GPR ordinario. */
static void marcar_lectura(SchedulerInfo *info, int reg)
{
    if (registro_valido(reg) && reg >= 2)
        info->lee[reg] = 1;
}

static void marcar_escritura_con_pc(SchedulerInfo *info, int reg)
{
    if (!registro_valido(reg))
        return;

    if (reg == 1) {
        info->redirecciona_pc = 1;
        return;
    }

    if (reg >= 2)
        info->escribe[reg] = 1;
}

/* FEIST nunca puede redirigir x1 en el RTL final. */
static void marcar_escritura_gpr(SchedulerInfo *info, int reg)
{
    if (registro_valido(reg) && reg >= 2)
        info->escribe[reg] = 1;
}

void scheduler_inicializar(Scheduler *scheduler)
{
    if (scheduler != NULL)
        memset(scheduler, 0, sizeof(*scheduler));
}

static int analizar_alu(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int rd, rs1, rs2;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0)
        return 1;

    if (strcmp(op, "ADD") == 0 ||
        strcmp(op, "SUB") == 0 ||
        strcmp(op, "OR") == 0 ||
        strcmp(op, "XOR") == 0 ||
        strcmp(op, "AND") == 0 ||
        strcmp(op, "SLT") == 0) {
        if (sscanf(texto, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2) != 3)
            return 0;
        marcar_lectura(info, rs1);
        marcar_lectura(info, rs2);
        marcar_escritura_con_pc(info, rd);
        return 1;
    }

    if (strcmp(op, "ADDI") == 0 ||
        strcmp(op, "SLLI") == 0 ||
        strcmp(op, "SRLI") == 0) {
        /* El tercer operando puede ser numerico o una etiqueta de Fase 5. */
        if (sscanf(texto, "%*s x%d, x%d", &rd, &rs1) != 2)
            return 0;
        marcar_lectura(info, rs1);
        marcar_escritura_con_pc(info, rd);
        return 1;
    }

    return 0;
}

static int analizar_lsu(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int reg, base;
    long offset;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0)
        return 1;

    if (strcmp(op, "LOAD") == 0) {
        if (sscanf(texto, "%*s x%d, %ld(x%d)", &reg, &offset, &base) != 3)
            return 0;
        (void)offset;
        marcar_lectura(info, base);
        marcar_escritura_con_pc(info, reg);
        return 1;
    }

    if (strcmp(op, "STORE") == 0) {
        if (sscanf(texto, "%*s x%d, %ld(x%d)", &reg, &offset, &base) != 3)
            return 0;
        (void)offset;
        marcar_lectura(info, reg);
        marcar_lectura(info, base);
        return 1;
    }

    return 0;
}

static int analizar_bru(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int rs1, rs2;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0 || strcmp(op, "JMP") == 0)
        return 1;

    if (strcmp(op, "BGE") == 0 ||
        strcmp(op, "BLE") == 0 ||
        strcmp(op, "BEQ") == 0 ||
        strcmp(op, "BNE") == 0) {
        if (sscanf(texto, "%*s x%d, x%d", &rs1, &rs2) != 2)
            return 0;
        marcar_lectura(info, rs1);
        marcar_lectura(info, rs2);
        return 1;
    }

    return 0;
}

static int analizar_feist(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int a, b, c, d;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0 || strcmp(op, "LOGOUT") == 0)
        return 1;

    if (strcmp(op, "AUTH") == 0) {
        if (sscanf(texto, "%*s x%d, x%d", &a, &b) != 2)
            return 0;
        marcar_lectura(info, a);
        marcar_lectura(info, b);
        return 1;
    }

    if (strcmp(op, "KSTORE") == 0) {
        /* KSTORE key, round, rs1 */
        if (sscanf(texto, "%*s %*d, %*d, x%d", &a) != 1)
            return 0;
        marcar_lectura(info, a);
        return 1;
    }

    if (strcmp(op, "FROUND") == 0) {
        /* FROUND rdL, rdR, rsL, rsR, key, round, mode */
        if (sscanf(texto, "%*s x%d, x%d, x%d, x%d", &a, &b, &c, &d) != 4)
            return 0;
        marcar_escritura_gpr(info, a);
        marcar_escritura_gpr(info, b);
        marcar_lectura(info, c);
        marcar_lectura(info, d);
        return 1;
    }

    return 0;
}

int scheduler_analizar(const char *slot,
                       const char *instruccion,
                       SchedulerInfo *info)
{
    if (slot == NULL || instruccion == NULL || info == NULL)
        return 0;

    memset(info, 0, sizeof(*info));

    if (strcmp(slot, "ALU") == 0)
        return analizar_alu(instruccion, info);
    if (strcmp(slot, "LSU") == 0)
        return analizar_lsu(instruccion, info);
    if (strcmp(slot, "BRU") == 0)
        return analizar_bru(instruccion, info);
    if (strcmp(slot, "FEIST") == 0)
        return analizar_feist(instruccion, info);

    return 0;
}

unsigned scheduler_nops_necesarios(const Scheduler *scheduler,
                                    const SchedulerInfo *info,
                                    size_t bundle_actual)
{
    size_t mayor = bundle_actual;
    int reg;

    if (scheduler == NULL || info == NULL)
        return 0;

    for (reg = 2; reg < SCHEDULER_REGISTROS; reg++) {
        if (info->lee[reg] && scheduler->disponible_desde[reg] > mayor)
            mayor = scheduler->disponible_desde[reg];
    }

    if (mayor <= bundle_actual)
        return 0;

    if (mayor - bundle_actual > UINT_MAX)
        return UINT_MAX;

    return (unsigned)(mayor - bundle_actual);
}

void scheduler_registrar_escrituras(Scheduler *scheduler,
                                     const SchedulerInfo *info,
                                     size_t bundle_instruccion)
{
    int reg;

    if (scheduler == NULL || info == NULL)
        return;

    for (reg = 2; reg < SCHEDULER_REGISTROS; reg++) {
        if (info->escribe[reg])
            scheduler->disponible_desde[reg] =
                bundle_instruccion + SCHEDULER_SEPARACION_RAW;
    }
}
