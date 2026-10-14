/*
Incluye las definiciones del scheduler y las bibliotecas necesarias para
identificar registros, analizar instrucciones y calcular separaciones RAW.
Esta fase prepara las dependencias de las instrucciones antes de emitirlas.
*/
#include "scheduler.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
Verifica que un registro se encuentre dentro del banco de registros
reconocido por el scheduler. Retorna 1 si es valido y 0 en caso contrario.
*/
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

/*
Registra el destino de una instruccion en la informacion del scheduler.
Si se escribe x1 se indica una redireccion del PC en lugar de una escritura
normal, mientras que los registros x2 en adelante se marcan como escritos.
*/
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

/*
Inicializa el estado del scheduler colocando en cero la disponibilidad
de los registros. Se utiliza al comenzar la generacion y cuando el flujo
de control requiere reiniciar el seguimiento de dependencias.
*/
void scheduler_inicializar(Scheduler *scheduler)
{
    if (scheduler != NULL)
        memset(scheduler, 0, sizeof(*scheduler));
}

/*
Analiza las instrucciones del slot ALU y reconoce sus operandos.
Registra los registros fuente y destino utilizados por operaciones
aritmeticas, logicas y desplazamientos para detectar dependencias RAW.
Retorna 1 cuando reconoce la instruccion y 0 si el formato es invalido.
*/
static int analizar_alu(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int rd, rs1, rs2;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0)
        return 1;

    /* Las operaciones con tres registros leen rs1 y rs2 y escriben rd. */
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

    /* Las instrucciones con inmediato solo leen un registro fuente. */
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

/*
Reconoce operaciones LOAD y STORE de la unidad de memoria.
Identifica el registro base utilizado para calcular la direccion y el
registro que se lee o escribe segun el tipo de acceso realizado.
*/
static int analizar_lsu(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int reg, base;
    long offset;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0)
        return 1;

    /* LOAD lee el registro base y deja el dato obtenido en el destino. */
    if (strcmp(op, "LOAD") == 0) {
        if (sscanf(texto, "%*s x%d, %ld(x%d)", &reg, &offset, &base) != 3)
            return 0;
        (void)offset;
        marcar_lectura(info, base);
        marcar_escritura_con_pc(info, reg);
        return 1;
    }

    /* STORE lee el dato a guardar y el registro base de la direccion. */
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

/*
Analiza las instrucciones de control de flujo del slot BRU.
Los saltos condicionales leen los dos registros comparados, mientras
que JMP no necesita registrar operandos de proposito general.
*/
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

/*
Reconoce las instrucciones de la unidad criptografica FEIST.
Registra los registros utilizados por AUTH y KSTORE y las fuentes y
destinos de FROUND para que tambien respeten las dependencias de datos.
*/
static int analizar_feist(const char *texto, SchedulerInfo *info)
{
    char op[16];
    int a, b, c, d;

    if (sscanf(texto, "%15s", op) != 1)
        return 0;

    if (strcmp(op, "NOP") == 0 || strcmp(op, "LOGOUT") == 0)
        return 1;

    /* AUTH necesita leer ambos registros utilizados para autenticacion. */
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

    /* FROUND lee dos mitades y escribe los dos registros de resultado. */
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

/*
Funcion principal de analisis de instrucciones del scheduler.
Recibe el slot y el texto ensamblador, limpia la informacion previa y
utiliza la funcion correspondiente para identificar lecturas y escrituras.
No genera bundles: proporciona los datos que utiliza codegen al emitirlos.
*/
int scheduler_analizar(const char *slot,
                       const char *instruccion,
                       SchedulerInfo *info)
{
    if (slot == NULL || instruccion == NULL || info == NULL)
        return 0;

    /* Evita conservar dependencias de una instruccion anterior. */
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

/*
Calcula la cantidad de bundles NOP que se deben insertar antes de una
instruccion para evitar riesgos RAW en el pipeline sin forwarding.
Revisa cuando estara disponible cada registro que la instruccion lee
y espera hasta la posicion mas lejana necesaria.
*/
unsigned scheduler_nops_necesarios(const Scheduler *scheduler,
                                    const SchedulerInfo *info,
                                    size_t bundle_actual)
{
    size_t mayor = bundle_actual;
    int reg;

    if (scheduler == NULL || info == NULL)
        return 0;

    /* Busca el ultimo registro fuente que todavia no estaria disponible. */
    for (reg = 2; reg < SCHEDULER_REGISTROS; reg++) {
        if (info->lee[reg] && scheduler->disponible_desde[reg] > mayor)
            mayor = scheduler->disponible_desde[reg];
    }

    /* Cuando todos los operandos estan disponibles no hacen falta NOPs. */
    if (mayor <= bundle_actual)
        return 0;

    /* Evita desbordamiento al convertir la diferencia a unsigned. */
    if (mayor - bundle_actual > UINT_MAX)
        return UINT_MAX;

    return (unsigned)(mayor - bundle_actual);
}

/*
Actualiza el estado del scheduler despues de emitir una instruccion.
Por cada registro destino almacena el primer bundle desde el cual su
valor puede leerse de forma segura, usando SCHEDULER_SEPARACION_RAW.
*/
void scheduler_registrar_escrituras(Scheduler *scheduler,
                                     const SchedulerInfo *info,
                                     size_t bundle_instruccion)
{
    int reg;

    if (scheduler == NULL || info == NULL)
        return;

    /* Solo los registros escritos modifican su disponibilidad futura. */
    for (reg = 2; reg < SCHEDULER_REGISTROS; reg++) {
        if (info->escribe[reg])
            scheduler->disponible_desde[reg] =
                bundle_instruccion + SCHEDULER_SEPARACION_RAW;
    }
}
