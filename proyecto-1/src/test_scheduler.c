#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>

static void exigir(int condicion, const char *mensaje)
{
    if (!condicion) {
        fprintf(stderr, "FALLO: %s\n", mensaje);
        exit(1);
    }
}

static SchedulerInfo analizar(const char *slot, const char *texto)
{
    SchedulerInfo info;
    if (!scheduler_analizar(slot, texto, &info)) {
        fprintf(stderr, "FALLO: no se pudo analizar %s: %s\n", slot, texto);
        exit(1);
    }
    return info;
}

int main(void)
{
    Scheduler s;
    SchedulerInfo prod, cons, indep;
    unsigned nops;

    scheduler_inicializar(&s);

    /* Productor en bundle 0: x4 no puede leerse antes del bundle 3. */
    prod = analizar("ALU", "ADDI x4, x0, 5");
    scheduler_registrar_escrituras(&s, &prod, 0);

    cons = analizar("ALU", "ADD x5, x4, x6");
    nops = scheduler_nops_necesarios(&s, &cons, 1);
    exigir(nops == 2, "dependencia inmediata debe requerir 2 NOPs");

    nops = scheduler_nops_necesarios(&s, &cons, 2);
    exigir(nops == 1, "una instruccion independiente debe reducir a 1 NOP");

    nops = scheduler_nops_necesarios(&s, &cons, 3);
    exigir(nops == 0, "dos bundles de separacion deben eliminar los NOPs");

    /* Dos fuentes: manda la que este disponible mas tarde. */
    scheduler_inicializar(&s);
    prod = analizar("LSU", "LOAD x4, -4(x15)");
    scheduler_registrar_escrituras(&s, &prod, 10); /* x4 desde 13 */
    prod = analizar("ALU", "ADDI x5, x0, 1");
    scheduler_registrar_escrituras(&s, &prod, 11); /* x5 desde 14 */
    cons = analizar("ALU", "ADD x6, x4, x5");
    exigir(scheduler_nops_necesarios(&s, &cons, 12) == 2,
           "debe respetarse la fuente mas tardia");

    /* x0 y x1 no se calendarizan como GPR ordinarios. */
    scheduler_inicializar(&s);
    indep = analizar("ALU", "ADDI x3, x1, ret_call_0");
    exigir(scheduler_nops_necesarios(&s, &indep, 0) == 0,
           "leer x1 como PC no debe crear RAW de GPR");

    /* STORE lee tanto el dato como la base. */
    scheduler_inicializar(&s);
    prod = analizar("ALU", "ADDI x14, x14, -4");
    scheduler_registrar_escrituras(&s, &prod, 0);
    cons = analizar("LSU", "STORE x4, 0(x14)");
    exigir(scheduler_nops_necesarios(&s, &cons, 1) == 2,
           "STORE debe esperar a la nueva base x14");

    /* BRU condicional tambien consume registros. */
    scheduler_inicializar(&s);
    prod = analizar("ALU", "SLT x4, x5, x6");
    scheduler_registrar_escrituras(&s, &prod, 0);
    cons = analizar("BRU", "BNE x4, x0, etiqueta");
    exigir(scheduler_nops_necesarios(&s, &cons, 1) == 2,
           "branch debe esperar el resultado que compara");

    /* FROUND queda preparado para la integracion posterior. */
    cons = analizar("FEIST", "FROUND x7, x8, x5, x6, 0, 1, 0");
    exigir(cons.lee[5] && cons.lee[6] && cons.escribe[7] && cons.escribe[8],
           "FROUND debe declarar dos fuentes y dos destinos");

    printf("PASS: RAW inmediato -> 2 NOPs.\n");
    printf("PASS: 1 bundle independiente -> 1 NOP.\n");
    printf("PASS: 2 bundles independientes -> 0 NOPs.\n");
    printf("PASS: ALU/LSU/BRU/FEIST exponen fuentes y destinos al scheduler.\n");
    return 0;
}
