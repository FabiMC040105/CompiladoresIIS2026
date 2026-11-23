/*
 * CodeGen desde IR - primera implementacion comprobable.
 *
 * No reemplaza el generador del AST. Trabaja con los cuadruplos del IR
 * y reutiliza el emisor y el scheduler del procesador del Proyecto I.
 *
 * Esta version admite programas escalares sin funciones ni arreglos.
 * Para los casos pendientes falla de forma explicita; nunca genera
 * ensamblador incompleto diciendo que la compilacion fue exitosa.
 */

#include "codegen_ir.h"

#include "memoria.h"
#include "spill_transform.h"

#include <errno.h>

#include <limits.h>

#include <stdint.h>

#include <stdarg.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>



/* x2 es retorno, x3 y x13 son auxiliares, x14 es SP y x15 es FP.

 * Para cada operacion usamos x4 y x5 como entradas y x6 como resultado. */

#define IR_R1 4

#define IR_R2 5

#define IR_RD 6

#define IR_DIR 13



typedef struct {

    CodegenMotor *motor;

    const CodegenIRConfig *config;

    CodegenIREstadisticas stats;

    const char *funcion;

    char **temporales;

    size_t cantidad_temporales;



    uint32_t inicio_temporales;

    /* Memoria reservada para los slots de Spilling IR. */
    uint32_t inicio_spilling;
    size_t cantidad_slots_spilling;
    uint32_t limite_datos_ir;

    /*

 * Nodo del AST asociado a la instruccion

 * IR que estamos traduciendo actualmente.

 */

    const NodoAST *nodo_actual;



    /* Argumentos pendientes de la siguiente llamada IR. */
    const char *argumentos[64];
    size_t cantidad_argumentos;
    unsigned contador_llamadas;
    int error;

} EstadoIR;





/* Muestra la linea del IR que produjo el problema. */

static int fallo(EstadoIR *e, const IRInstruction *i, const char *mensaje)

{

    fprintf(stderr, "CodeGen IR%s%d: %s\n",

            i && i->source_line > 0 ? " linea " : " (sin linea) ",

            i ? i->source_line : 0, mensaje);

    e->error = 1;

    return 0;

}



/* Todas las instrucciones pasan por el motor del CodeGen original.

 * Asi no duplicamos la logica de NOPs ni las barreras de control. */

static int emitir(EstadoIR *e, const char *slot, const char *fmt, ...)

{

    char texto[256];

    va_list ap;

    int n;



    va_start(ap, fmt);

    n = vsnprintf(texto, sizeof(texto), fmt, ap);

    va_end(ap);



    if (n < 0 || (size_t)n >= sizeof(texto) ||

        !codegen_motor_emitir(e->motor, slot, texto)) {

        e->error = 1;

        return 0;

    }

    return 1;

}



/* El procesador no tiene una instruccion para cargar cualquier entero.

 * Reutilizamos el procedimiento de construir los cuatro bytes. */

static int constante(EstadoIR *e, int reg, int32_t valor)

{

    uint32_t bits = (uint32_t)valor;

    if (valor >= -2048 && valor <= 2047)

        return emitir(e, "ALU", "ADDI x%d, x0, %d", reg, valor);

    if (!emitir(e, "ALU", "ADDI x%d, x0, %u", reg, (bits >> 24) & 255u)) return 0;

    for (int desplazamiento = 16; desplazamiento >= 0; desplazamiento -= 8) {

        if (!emitir(e, "ALU", "SLLI x%d, x%d, 8", reg, reg) ||

            !emitir(e, "ALU", "ADDI x%d, x%d, %u", reg, reg,

                    (bits >> desplazamiento) & 255u)) return 0;

    }

    return 1;

}



/* Distingue literales de identificadores sin confundir 12abc con 12. */

static int numero(const char *texto, int32_t *salida)

{

    char *fin;

    long long valor;

    if (!texto || !*texto) return 0;

    errno = 0;

    valor = strtoll(texto, &fin, 10);

    if (errno || *fin || fin == texto || valor < INT32_MIN || valor > INT32_MAX)

        return 0;

    *salida = (int32_t)valor;

    return 1;

}



static int es_temporal(const char *nombre)

{

    return nombre && nombre[0] == '%' && nombre[1] == 't';

}



/**

 * Busca la direccion de un temporal del IR.

 *

 * Los temporales se almacenan en un espacio reservado

 * despues de las variables globales. Por ahora mantenemos

 * este mecanismo porque no pertenecen a la tabla de simbolos.

 */

static int direccion_temporal(EstadoIR *e,

                             const char *nombre,

                             long *salida)

{

    if (e == NULL || nombre == NULL || salida == NULL)

        return 0;



    if (!es_temporal(nombre))

        return 0;



    for (size_t j = 0; j < e->cantidad_temporales; j++) {

        if (strcmp(e->temporales[j], nombre) == 0) {

            *salida = (long)e->inicio_temporales +

                      (long)j * 4;

            return 1;

        }

    }



    return 0;

}



/**

 * Resuelve una variable utilizando la informacion

 * semantica proporcionada al adaptador IR.

 *

 * En esta etapa solo aceptamos variables globales

 * escalares. No intentamos adivinar el ambito de

 * una variable local.

 */

static int resolver_variable(EstadoIR *e,

                             const char *nombre,

                             CodegenSimbolo *simbolo)

{

    if (e == NULL || nombre == NULL || simbolo == NULL)

        return 0;



    if (e->config == NULL ||

        e->config->resolver_ir == NULL)

        return 0;





    /*

 * Enviamos el nodo original para que la

 * tabla busque primero en el ambito correcto.

 */

    if (!e->config->resolver_ir(
            e->config->contexto_ir,
            e->nodo_actual,
            e->funcion,
            nombre,
            simbolo)) {
        fprintf(stderr,
                "[IR] No se encontro '%s' en '%s' (nodo=%p)\n",
                nombre,
                e->funcion ? e->funcion : "(global)",
                (void *)e->nodo_actual);
        return 0;
    }









    /*

 * Permitimos variables escalares globales,

 * locales y parametros por valor.

 *

 * El motor de CodeGen ya sabe calcular sus

 * direcciones utilizando la memoria global

 * o los desplazamientos relativos al FP.

 */

    if (simbolo->forma != CODEGEN_FORMA_ESCALAR ||

        simbolo->categoria == CODEGEN_CAT_FUNCION ||

        simbolo->parametro_por_referencia)

        return 0;



    if (simbolo->ubicacion != CODEGEN_UBICACION_GLOBAL &&

        simbolo->ubicacion != CODEGEN_UBICACION_LOCAL &&

        simbolo->ubicacion != CODEGEN_UBICACION_PARAMETRO)

        return 0;



    return 1;

}



/**

 * Carga un operando del IR en un registro.

 *

 * Puede ser una constante, un temporal o una

 * variable global registrada en la tabla.

 */

static int cargar(EstadoIR *e,

                  const char *nombre,

                  int reg)

{

    int32_t valor;

    long dir;

    CodegenSimbolo simbolo;



    if (e == NULL || nombre == NULL)

        return 0;



    /*

 * Las constantes se construyen directamente

 * en el registro solicitado.

 */

    if (numero(nombre, &valor))

        return constante(e, reg, valor);



    /*

 * Los temporales conservan su espacio propio

 * hasta implementar una asignacion de registros.

 */

    if (es_temporal(nombre)) {

        const AsignacionTemporal *a = NULL;

        if (e->config != NULL &&
            e->config->asignacion_registros != NULL)
            a = ar_buscar(e->config->asignacion_registros, nombre);

        if (a != NULL && !a->es_spill &&
            a->registro >= 4 && a->registro <= 12) {
            if (reg == a->registro)
                return 1;

            return emitir(e, "ALU",
                          "ADD x%d, x%d, x0",
                          reg, a->registro);
        }

        if (!direccion_temporal(e, nombre, &dir))
            return 0;

        return constante(e, IR_DIR, (int32_t)dir) &&
               emitir(e, "LSU",
                      "LOAD x%d, 0(x%d)",
                      reg, IR_DIR);

    }



    /*

 * Para variables reales, reutilizamos la

 * logica de memoria del CodeGen original.

 */

    if (!resolver_variable(e, nombre, &simbolo))

        return 0;



    return codegen_motor_cargar_simbolo(

        e->motor, &simbolo, reg);

}



/**

 * Guarda el valor de un registro en un destino IR.

 *

 * Los destinos pueden ser temporales o variables.

 * Las constantes no son destinos validos.

 */

static int guardar(EstadoIR *e,

                   const char *nombre,

                   int reg)

{

    long dir;

    CodegenSimbolo simbolo;



    if (e == NULL || nombre == NULL)

        return 0;



    /*

 * Los temporales mantienen su almacenamiento

 * independiente de la tabla de simbolos.

 */

    if (es_temporal(nombre)) {

        const AsignacionTemporal *a = NULL;

        if (e->config != NULL &&
            e->config->asignacion_registros != NULL)
            a = ar_buscar(e->config->asignacion_registros, nombre);

        if (a != NULL && !a->es_spill &&
            a->registro >= 4 && a->registro <= 12) {
            if (reg == a->registro)
                return 1;

            return emitir(e, "ALU",
                          "ADD x%d, x%d, x0",
                          a->registro, reg);
        }

        if (!direccion_temporal(e, nombre, &dir))
            return 0;

        return constante(e, IR_DIR, (int32_t)dir) &&
               emitir(e, "LSU",
                      "STORE x%d, 0(x%d)",
                      reg, IR_DIR);

    }



    /*

 * Las variables se guardan mediante el

 * motor que ya utiliza el compilador.

 */

    if (!resolver_variable(e, nombre, &simbolo))

        return 0;



    return codegen_motor_guardar_simbolo(

        e->motor, &simbolo, reg);

}



/* Devuelve el registro fisico asignado a un temporal o -1. */
static int registro_temporal_mapeado(EstadoIR *e, const char *nombre)
{
    const AsignacionTemporal *a;

    if (e == NULL || nombre == NULL || !es_temporal(nombre) ||
        e->config == NULL || e->config->asignacion_registros == NULL)
        return -1;

    a = ar_buscar(e->config->asignacion_registros, nombre);

    if (a == NULL || a->es_spill || a->registro < 4 || a->registro > 12)
        return -1;

    return a->registro;
}

/* Obtiene un operando sin copiar temporales coloreados. */
static int operando_en_registro(EstadoIR *e, const char *nombre,
                                int auxiliar, int *registro)
{
    int r;

    if (e == NULL || nombre == NULL || registro == NULL)
        return 0;

    r = registro_temporal_mapeado(e, nombre);
    if (r >= 0) {
        *registro = r;
        return 1;
    }

    if (!cargar(e, nombre, auxiliar))
        return 0;

    *registro = auxiliar;
    return 1;
}

/* Recolectamos los temporales de todas las instrucciones, sin duplicados. */

static int registrar_temporal(EstadoIR *e, const char *nombre)

{

    if (!es_temporal(nombre)) return 1;

    for (size_t j = 0; j < e->cantidad_temporales; j++)

        if (!strcmp(e->temporales[j], nombre)) return 1;

    char **nuevos = realloc(e->temporales,

                            (e->cantidad_temporales + 1) * sizeof(char *));

    if (!nuevos) return 0;

    e->temporales = nuevos;

    size_t len = strlen(nombre) + 1;

    char *copia = malloc(len);

    if (!copia) return 0;

    memcpy(copia, nombre, len);

    e->temporales[e->cantidad_temporales++] = copia;

    return 1;

}



static int preparar(EstadoIR *e, const IRProgram *p)

{

    uint32_t fin = e->config->original.fin_globales;

    if (fin > MEMORIA_TOTAL_BYTES || fin % TAMANO_PALABRA) return 0;

    e->inicio_temporales = fin;

    for (size_t j = 0; j < p->count; j++) {

        const IRInstruction *i = &p->instructions[j];

        if (!registrar_temporal(e, i->arg1) ||

            !registrar_temporal(e, i->arg2) ||

            !registrar_temporal(e, i->result) ||

            !registrar_temporal(e, i->extra)) return 0;

    }

    /* Identificamos los slots utilizados por Spilling IR. */
    for (size_t j = 0; j < p->count; ++j) {
        const IRInstruction *i = &p->instructions[j];
        const char *indice = NULL;
        int32_t slot;

        if (i->op == IR_ARRAY_LOAD && i->arg1 &&
            strcmp(i->arg1, SPILL_IR_AREA) == 0)
            indice = i->arg2;

        if (i->op == IR_ARRAY_STORE && i->result &&
            strcmp(i->result, SPILL_IR_AREA) == 0)
            indice = i->arg1;

        if (!indice)
            continue;

        if (!numero(indice, &slot) || slot < 0)
            return 0;

        if ((size_t)slot + 1 > e->cantidad_slots_spilling)
            e->cantidad_slots_spilling = (size_t)slot + 1;
    }

    e->stats.temporales = e->cantidad_temporales;



    /*

 * Por ahora los temporales del IR se almacenan

 * despues de las variables globales.

 *

 * Comprobamos que ese espacio no sobrepase la

 * memoria disponible y que quede al menos una

 * parte libre para la pila.

 *

 * Cuando implementemos funciones recursivas,

 * moveremos estos temporales a cada marco de pila.

 */

    if (e->cantidad_temporales >

        (MEMORIA_TOTAL_BYTES - fin) / 4u)

    {

        return 0;

    }



    {
        uint64_t inicio =
            (uint64_t)fin +
            (uint64_t)e->cantidad_temporales * TAMANO_PALABRA;

        uint64_t limite =
            inicio +
            (uint64_t)e->cantidad_slots_spilling * TAMANO_PALABRA;

        if (limite >= MEMORIA_TOTAL_BYTES)
            return 0;

        e->inicio_spilling = (uint32_t)inicio;
        e->limite_datos_ir = (uint32_t)limite;
    }

    return 1;



}





/**

 * Revisa la estructura de las funciones del IR

 * antes de comenzar a generar ensamblador.

 *

 * Comprobamos que las funciones existan en la

 * tabla de simbolos, que no esten anidadas y

 * que cada inicio tenga su final correspondiente.

 */

static int validar_funciones(

    EstadoIR *e,

    const IRProgram *programa)

{

    const char *funcion_actual = NULL;

    int encontro_principal = 0;



    if (e == NULL || programa == NULL ||

        e->config == NULL ||

        e->config->resolver_ir == NULL)

        return 0;



    for (size_t j = 0; j < programa->count; j++) {

        const IRInstruction *i = &programa->instructions[j];



        if (i->op == IR_FUNC_BEGIN) {

            CodegenSimbolo simbolo;



            /*

 * No permitimos iniciar otra funcion

 * mientras una anterior siga abierta.

 */

            if (funcion_actual != NULL)

                return fallo(e, i,

                    "se encontro una funcion dentro de otra");



            if (i->result == NULL || i->result[0] == '\0')

                return fallo(e, i,

                    "la funcion no tiene nombre");



            memset(&simbolo, 0, sizeof(simbolo));



            /*

 * Las funciones se declaran en el

 * ambito global, por eso consultamos

 * sin un nodo local.

 */

            if (!e->config->resolver_ir(

                    e->config->contexto_ir,

                    NULL,

                    NULL,

                    i->result,

                    &simbolo))



                return fallo(e, i,

                    "no se encontro la funcion en la tabla de simbolos");



            if (simbolo.categoria != CODEGEN_CAT_FUNCION ||

                simbolo.ubicacion != CODEGEN_UBICACION_FUNCION)

                return fallo(e, i,

                    "el simbolo no corresponde a una funcion");





            /*

 * Comprobamos que el marco de la funcion

 * tenga un tamaño valido para nuestra memoria.

 *

 * Principal no necesita guardar el FP anterior

 * ni una direccion de retorno.

 *

 * Las funciones normales utilizan una cabecera

 * de ocho bytes, como en el CodeGen original.

 */

            {

                uint32_t cabecera =

                    strcmp(i->result, "principal") == 0

                    ? 0u

                    : TAMANO_CABECERA;



                if (!pila_hay_espacio(

                        MEMORIA_TOTAL_BYTES,

                        simbolo.tamano_marco,

                        cabecera,

                        e->limite_datos_ir))

                {

                    return fallo(

                        e,

                        i,

                        "el marco de pila de la funcion "

                        "excede la memoria disponible");

                }

            }





            /*

 * Principal debe aparecer una sola vez.

 */

            if (strcmp(i->result, "principal") == 0) {

                if (encontro_principal)

                    return fallo(e, i,

                        "principal fue declarada mas de una vez");



                encontro_principal = 1;

            }



            funcion_actual = i->result;



            /*

 * Este tamano_marco sera utilizado

 * posteriormente para reservar los

 * locales de la funcion.

 */

            continue;

        }



        if (i->op == IR_FUNC_END) {

            if (funcion_actual == NULL)

                return fallo(e, i,

                    "se encontro un final sin funcion activa");



            if (i->result == NULL ||

                strcmp(funcion_actual, i->result) != 0)

                return fallo(e, i,

                    "el final no coincide con la funcion activa");



            funcion_actual = NULL;

        }

    }



    /*

 * Al terminar el recorrido no debe quedar

 * ninguna funcion sin cerrar.

 */

    if (funcion_actual != NULL) {

        fprintf(stderr,

                "CodeGen IR: la funcion '%s' no tiene IR_FUNC_END\n",

                funcion_actual);

        return 0;

    }



    if (!encontro_principal) {

        fprintf(stderr,

                "CodeGen IR: no se encontro la funcion principal\n");

        return 0;

    }



    return 1;

}





/**

 * Recupera la informacion de una funcion desde

 * la tabla de simbolos de la Fase 3.

 *

 * Utilizamos esta informacion para reservar

 * exactamente el marco de pila que ya calculo

 * el compilador original.

 */

static int obtener_funcion_ir(

    EstadoIR *e,

    const char *nombre,

    CodegenSimbolo *simbolo)

{

    if (e == NULL || nombre == NULL ||

        simbolo == NULL || e->config == NULL ||

        e->config->resolver_ir == NULL)

        return 0;



    memset(simbolo, 0, sizeof(*simbolo));



    if (!e->config->resolver_ir(

            e->config->contexto_ir,

            NULL,

            NULL,

            nombre,

            simbolo))

        return 0;



    return simbolo->categoria == CODEGEN_CAT_FUNCION &&

           simbolo->ubicacion == CODEGEN_UBICACION_FUNCION;

}





/*
 * Etiquetas privadas del backend IR. Usamos un contador independiente
 * para que los bucles auxiliares no colisionen con las etiquetas del IR.
 */
static unsigned long siguiente_etiqueta_ir = 0;

static int etiqueta_auxiliar(EstadoIR *e, const char *prefijo,
                            char *salida, size_t capacidad)
{
    int n = snprintf(salida, capacidad, "__ir_%s_%lu",
                     prefijo, siguiente_etiqueta_ir++);
    (void)e;
    return n > 0 && (size_t)n < capacidad;
}

/*
 * Convierte una comparacion a un booleano 0/1. El ISA admite BEQ,
 * BNE, BGE y BLE; las otras relaciones se obtienen intercambiando
 * los operandos. Nunca emitimos operadores inexistentes.
 */
static int preparar_binaria_directa(EstadoIR *e,
                                    const char *arg1,
                                    const char *arg2,
                                    int destino,
                                    int *reg1,
                                    int *reg2)
{
    int r1, r2;

    if (!e || !arg1 || !arg2 || !reg1 || !reg2 ||
        destino < 4 || destino > 12)
        return 0;

    r1 = registro_temporal_mapeado(e, arg1);
    r2 = registro_temporal_mapeado(e, arg2);

    if (r1 >= 0 && r2 >= 0) {
        *reg1 = r1;
        *reg2 = r2;
        return 1;
    }

    if (r1 >= 0) {
        *reg1 = r1;
        if (destino != r1) {
            if (!cargar(e, arg2, destino)) return 0;
            *reg2 = destino;
        } else {
            if (!cargar(e, arg2, 3)) return 0;
            *reg2 = 3;
        }
        return 1;
    }

    if (r2 >= 0) {
        *reg2 = r2;
        if (destino != r2) {
            if (!cargar(e, arg1, destino)) return 0;
            *reg1 = destino;
        } else {
            if (!cargar(e, arg1, 3)) return 0;
            *reg1 = 3;
        }
        return 1;
    }

    if (!cargar(e, arg1, destino) || !cargar(e, arg2, 3))
        return 0;

    *reg1 = destino;
    *reg2 = 3;
    return 1;
}

static int comparar_ir(EstadoIR *e, IROp op,
                       int izq, int der, int destino)
{
    char verdadero[80], fin[80];
    const char *salto = NULL;

    if (!etiqueta_auxiliar(e, "cmp_true", verdadero, sizeof(verdadero)) ||
        !etiqueta_auxiliar(e, "cmp_end", fin, sizeof(fin)))
        return 0;

    switch (op) {
    case IR_EQ: salto = "BEQ"; break;
    case IR_NE: salto = "BNE"; break;
    case IR_LE: salto = "BLE"; break;
    case IR_GE: salto = "BGE"; break;
    case IR_LT: salto = "BLE"; break;
    case IR_GT: salto = "BGE"; break;
    default: return 0;
    }

    if (op == IR_LT || op == IR_GT) {
        if (!emitir(e, "ALU", "ADDI x%d, x0, 0", destino) ||
            !emitir(e, "BRU", "BEQ x%d, x%d, %s", izq, der, fin) ||
            !emitir(e, "BRU", "%s x%d, x%d, %s",
                    op == IR_LT ? "BLE" : "BGE", izq, der, verdadero) ||
            !emitir(e, "BRU", "JMP %s", fin) ||
            !codegen_motor_etiqueta(e->motor, verdadero) ||
            !emitir(e, "ALU", "ADDI x%d, x0, 1", destino) ||
            !codegen_motor_etiqueta(e->motor, fin)) return 0;
        return 1;
    }

    return emitir(e, "ALU", "ADDI x%d, x0, 0", destino) &&
           emitir(e, "BRU", "%s x%d, x%d, %s", salto, izq, der, verdadero) &&
           emitir(e, "BRU", "JMP %s", fin) &&
           codegen_motor_etiqueta(e->motor, verdadero) &&
           emitir(e, "ALU", "ADDI x%d, x0, 1", destino) &&
           codegen_motor_etiqueta(e->motor, fin);
}

/* Traducimos las instrucciones sencillas y rechazamos las no implementadas.

 * Nunca convertimos una instruccion desconocida en un NOP. */



/* Inserta una palabra en la pila y mantiene el convenio del CodeGen AST. */
static int apilar_ir(EstadoIR *e, int reg)
{
    return emitir(e, "ALU", "ADDI x14, x14, -4") &&
           emitir(e, "LSU", "STORE x%d, 0(x14)", reg);
}

static int desapilar_ir(EstadoIR *e, int reg)
{
    return emitir(e, "LSU", "LOAD x%d, 0(x14)", reg) &&
           emitir(e, "ALU", "ADDI x14, x14, 4");
}


static int guardar_banco_temporal_ir(EstadoIR *e)
{
    for (int reg = 4; reg <= 12; ++reg)
        if (!apilar_ir(e, reg)) return 0;
    return 1;
}

static int restaurar_banco_temporal_ir(EstadoIR *e)
{
    for (int reg = 12; reg >= 4; --reg)
        if (!desapilar_ir(e, reg)) return 0;
    return 1;
}

/* Empuja un operando sin copiar un temporal coloreado sobre otro x4-x12. */
static int apilar_operando_ir(EstadoIR *e, const char *nombre)
{
    int reg = registro_temporal_mapeado(e, nombre);

    if (reg >= 0)
        return apilar_ir(e, reg);

    return cargar(e, nombre, 3) && apilar_ir(e, 3);
}

/*
 * IR_PARAM consume semanticamente el argumento en el punto donde aparece.
 * Con Chaitin, dos temporales de parametros distintos pueden reutilizar el
 * mismo registro despues de su IR_PARAM. Como llamar_ir materializa la pila
 * mas tarde, congelamos aqui los temporales coloreados en su slot de respaldo
 * y los recuperamos al construir la llamada.
 */
static int congelar_argumento_ir(EstadoIR *e, const char *nombre)
{
    long dir;
    int reg;

    if (!e || !nombre)
        return 0;

    if (e->config == NULL ||
        e->config->asignacion_registros == NULL ||
        !es_temporal(nombre))
        return 1;

    reg = registro_temporal_mapeado(e, nombre);

    /* Un temporal no coloreado ya utiliza su respaldo en memoria. */
    if (reg < 0)
        return 1;

    if (!direccion_temporal(e, nombre, &dir))
        return 0;

    return constante(e, 13, (int32_t)dir) &&
           emitir(e, "LSU", "STORE x%d, 0(x13)", reg);
}

static int apilar_argumento_congelado_ir(EstadoIR *e, const char *nombre)
{
    long dir;

    if (e == NULL || nombre == NULL)
        return 0;

    if (e->config != NULL &&
        e->config->asignacion_registros != NULL &&
        es_temporal(nombre)) {
        if (!direccion_temporal(e, nombre, &dir))
            return 0;

        return constante(e, 13, (int32_t)dir) &&
               emitir(e, "LSU", "LOAD x3, 0(x13)") &&
               apilar_ir(e, 3);
    }

    return apilar_operando_ir(e, nombre);
}

/*
 * Las rutinas software MUL/DIV/MOD/POW usan libremente x4-x12. Antes de
 * llamarlas preservamos el banco asignado por Chaitin, colocamos los dos
 * operandos en x4/x5 y conservamos el resultado en x2 mientras restauramos.
 */
static int operacion_entera_resguardada(EstadoIR *e,
                                        const IRInstruction *i,
                                        CodegenOperacionEntera operacion)
{
    if (!e || !i || !i->arg1 || !i->arg2 || !i->result)
        return 0;

    if (!guardar_banco_temporal_ir(e) ||
        !apilar_operando_ir(e, i->arg1) ||
        !apilar_operando_ir(e, i->arg2) ||
        !desapilar_ir(e, IR_R2) ||
        !desapilar_ir(e, IR_R1) ||
        !codegen_motor_operacion_entera(e->motor, operacion,
                                       IR_R1, IR_R2, IR_RD) ||
        !emitir(e, "ALU", "ADD x2, x%d, x0", IR_RD) ||
        !restaurar_banco_temporal_ir(e) ||
        !guardar(e, i->result, 2))
        return 0;

    return 1;
}

/*
 * Realiza una llamada con el mismo marco del generador AST.
 * Ademas de los registros, preserva los temporales IR del llamador.
 * Es necesario porque actualmente estos temporales ocupan memoria estatica.
 */
static int llamar_ir(EstadoIR *e, const IRInstruction *i)
{
    CodegenSimbolo funcion;
    char retorno[96], destino[160];
    unsigned numero_llamada = e->contador_llamadas++;
    size_t n = e->cantidad_argumentos;
    long cantidad = 0;

    {
        char *fin;
        if (!i->arg2) return fallo(e, i, "llamada sin cantidad de argumentos");
        cantidad = strtol(i->arg2, &fin, 10);
        if (*fin || cantidad < 0 || cantidad > 64)
            return fallo(e, i, "cantidad de argumentos invalida");
    }
    if ((size_t)cantidad != n || !i->arg1)
        return fallo(e, i, "los IR_PARAM no coinciden con IR_CALL");
    if (!obtener_funcion_ir(e, i->arg1, &funcion) ||
        funcion.tamano_parametros != n * 4u)
        return fallo(e, i, "firma o memoria de funcion incompatible");
    if (snprintf(retorno, sizeof(retorno), "ir_retorno_%u", numero_llamada) >= (int)sizeof(retorno) ||
        snprintf(destino, sizeof(destino), "func_%s", i->arg1) >= (int)sizeof(destino))
        return fallo(e, i, "etiqueta de llamada demasiado larga");

    /* Primero se guardan los registros x4-x12 del llamador. */
    if (!guardar_banco_temporal_ir(e)) return 0;

    /*
     * Los argumentos se empujan directamente desde el registro asignado
     * por Chaitin. Para constantes/variables usamos x3, que es auxiliar.
     */
    for (size_t a = n; a > 0; --a)
        if (!apilar_argumento_congelado_ir(e, e->argumentos[a - 1])) return 0;

    if (!emitir(e, "ALU", "ADDI x14, x14, -4") ||
        !emitir(e, "ALU", "ADDI x3, x1, %s", retorno) ||
        !emitir(e, "LSU", "STORE x3, 0(x14)") ||
        !emitir(e, "BRU", "JMP %s", destino) ||
        !codegen_motor_etiqueta(e->motor, retorno)) return 0;

    /* El callee ya retiro su cabecera; retiramos los argumentos. */
    for (size_t a = 0; a < n; ++a)
        if (!emitir(e, "ALU", "ADDI x14, x14, 4")) return 0;

    /* x2 conserva el valor de retorno mientras restauramos x4-x12. */
    if (!restaurar_banco_temporal_ir(e)) return 0;

    e->cantidad_argumentos = 0;
    return !i->result || guardar(e, i->result, 2);
}

/* Las intrinsecas FEIST son instrucciones del procesador, no funciones de usuario. */
static int es_feist_ir(const char *nombre)
{
    return nombre && (!strcmp(nombre, "feist_auth") ||
                      !strcmp(nombre, "feist_kstore") ||
                      !strcmp(nombre, "feist_fround") ||
                      !strcmp(nombre, "feist_logout"));
}

/* Los campos de llave, ronda y modo deben ser literales del AST. */
static int inmediato_feist_ir(const NodoAST *n, int maximo)
{
    int32_t valor;
    if (!n || n->tipo != AST_NUMERO || !numero(n->valor, &valor) ||
        valor < 0 || valor > maximo) return -1;
    return (int)valor;
}

static int traducir_feist_ir(EstadoIR *e, const IRInstruction *i)
{
    const NodoAST *args[7] = {0}, *actual;
    size_t cantidad = 0, n = e->cantidad_argumentos;
    char *fin;
    long declarados;
    int llave, ronda, modo;
    const char *nombre = i->arg1;
    int ok = 0;

    if (!i->arg2 || !nombre || !i->nodo_origen ||
        i->nodo_origen->tipo != AST_LLAMADA)
        return fallo(e, i, "llamada FEIST sin informacion de origen");
    declarados = strtol(i->arg2, &fin, 10);
    if (*fin || declarados < 0 || declarados > 7 || (size_t)declarados != n)
        return fallo(e, i, "cantidad de argumentos FEIST incompatible");
    for (actual = i->nodo_origen->hijo; actual; actual = actual->siguiente) {
        if (cantidad >= 7) return fallo(e, i, "demasiados argumentos FEIST");
        args[cantidad++] = actual;
    }
    if (cantidad != n) return fallo(e, i, "argumentos AST e IR no coinciden");

    if (!strcmp(nombre, "feist_logout")) {
        if (n != 0) return fallo(e, i, "LOGOUT no recibe argumentos");
        ok = emitir(e, "FEIST", "LOGOUT");
    } else if (!strcmp(nombre, "feist_auth")) {
        int r0, r1;
        if (n != 2) return fallo(e, i, "AUTH requiere dos argumentos");
        ok = operando_en_registro(e, e->argumentos[0], 2, &r0) &&
             operando_en_registro(e, e->argumentos[1], 3, &r1) &&
             emitir(e, "FEIST", "AUTH x%d, x%d", r0, r1);
    } else if (!strcmp(nombre, "feist_kstore")) {
        int r;
        if (n != 3) return fallo(e, i, "KSTORE requiere tres argumentos");
        llave = inmediato_feist_ir(args[0], 3);
        ronda = inmediato_feist_ir(args[1], 3);
        if (llave < 0 || ronda < 0)
            return fallo(e, i, "KSTORE requiere llave y ronda literales 0..3");
        ok = operando_en_registro(e, e->argumentos[2], 3, &r) &&
             emitir(e, "FEIST", "KSTORE %d, %d, x%d", llave, ronda, r);
    } else if (!strcmp(nombre, "feist_fround")) {
        if (n != 7) return fallo(e, i, "FROUND requiere siete argumentos");
        llave = inmediato_feist_ir(args[4], 3);
        ronda = inmediato_feist_ir(args[5], 3);
        modo = inmediato_feist_ir(args[6], 1);
        if (llave < 0 || ronda < 0 || modo < 0 ||
            !args[0] || !args[1] ||
            args[0]->tipo != AST_VARIABLE || args[1]->tipo != AST_VARIABLE ||
            !args[0]->valor || !args[1]->valor ||
            !strcmp(args[0]->valor, args[1]->valor))
            return fallo(e, i, "destinos o inmediatos FROUND invalidos");
        {
            int rL, rR;
            /* x2/x3 son auxiliares fuera del rango asignado por Chaitin. */
            ok = operando_en_registro(e, e->argumentos[2], 2, &rL) &&
                 operando_en_registro(e, e->argumentos[3], 3, &rR) &&
                 emitir(e, "FEIST",
                        "FROUND x2, x3, x%d, x%d, %d, %d, %d",
                        rL, rR, llave, ronda, modo) &&
                 guardar(e, args[0]->valor, 2) &&
                 guardar(e, args[1]->valor, 3);
        }
    }
    if (!ok) return fallo(e, i, "no se pudo emitir la instruccion FEIST");
    e->cantidad_argumentos = 0;
    return 1;
}

/* Calcula la base real de una lista o matriz en el registro indicado. */
static int base_indexada_ir(EstadoIR *e, const IRInstruction *i,
                            const char *nombre, int matriz, int reg_destino)
{
    CodegenSimbolo s;

    if (!nombre || !e->config || !e->config->resolver_ir ||
        !e->config->resolver_ir(e->config->contexto_ir, e->nodo_actual,
                                e->funcion, nombre, &s))
        return fallo(e, i, "no se encontro el arreglo en su ambito");

    if (s.forma != (matriz ? CODEGEN_FORMA_MATRIZ : CODEGEN_FORMA_LISTA) ||
        s.cantidad_dimensiones < (matriz ? 2u : 1u))
        return fallo(e, i, "forma o dimensiones del arreglo incompatibles");

    if (s.direccion < INT32_MIN || s.direccion > INT32_MAX)
        return 0;

    if (s.ubicacion == CODEGEN_UBICACION_GLOBAL)
        return constante(e, reg_destino, (int32_t)s.direccion);

    if (s.ubicacion != CODEGEN_UBICACION_LOCAL &&
        s.ubicacion != CODEGEN_UBICACION_PARAMETRO)
        return 0;

    if (!constante(e, reg_destino, (int32_t)s.direccion) ||
        !emitir(e, "ALU", "ADD x%d, x15, x%d",
                reg_destino, reg_destino))
        return 0;

    if (s.parametro_por_referencia)
        return emitir(e, "LSU", "LOAD x%d, 0(x%d)",
                      reg_destino, reg_destino);

    return 1;
}

/*
 * Calcula la direccion de un elemento exclusivamente con registros
 * reservados del backend (x2, x3 y x13), por lo que no destruye los
 * temporales x4-x12 asignados por Chaitin.
 */
static int direccion_indexada_ir(EstadoIR *e, const IRInstruction *i,
                                 const char *nombre, const char *fila,
                                 const char *columna, int matriz)
{
    CodegenSimbolo s;

    if (!nombre || !fila || !e->config->resolver_ir ||
        !e->config->resolver_ir(e->config->contexto_ir, e->nodo_actual,
                                e->funcion, nombre, &s))
        return fallo(e, i, "arreglo sin simbolo valido");

    if (s.forma != (matriz ? CODEGEN_FORMA_MATRIZ : CODEGEN_FORMA_LISTA))
        return fallo(e, i, "tipo de arreglo incompatible");

    if (!matriz) {
        if (!cargar(e, fila, 3) ||
            !emitir(e, "ALU", "SLLI x3, x3, 2") ||
            !base_indexada_ir(e, i, nombre, 0, 13) ||
            !emitir(e, "ALU", "ADD x13, x13, x3"))
            return 0;
        return 1;
    }

    if (!columna || s.cantidad_dimensiones < 2 ||
        !s.dimensiones[1] || s.dimensiones[1] > INT32_MAX)
        return fallo(e, i, "columnas invalidas");

    {
        char bucle[80], fin[80];

        if (!etiqueta_auxiliar(e, "idx_mul", bucle, sizeof(bucle)) ||
            !etiqueta_auxiliar(e, "idx_mul_fin", fin, sizeof(fin)))
            return 0;

        /* x13 = fila * numero_columnas mediante suma repetida. */
        if (!cargar(e, fila, 2) ||
            !constante(e, 3, (int32_t)s.dimensiones[1]) ||
            !constante(e, 13, 0) ||
            !codegen_motor_etiqueta(e->motor, bucle) ||
            !emitir(e, "BRU", "BEQ x2, x0, %s", fin) ||
            !emitir(e, "ALU", "ADD x13, x13, x3") ||
            !emitir(e, "ALU", "ADDI x2, x2, -1") ||
            !emitir(e, "BRU", "JMP %s", bucle) ||
            !codegen_motor_etiqueta(e->motor, fin))
            return 0;
    }

    /* offset = (fila * columnas + columna) * 4 */
    if (!cargar(e, columna, 3) ||
        !emitir(e, "ALU", "ADD x13, x13, x3") ||
        !emitir(e, "ALU", "SLLI x13, x13, 2") ||
        !base_indexada_ir(e, i, nombre, 1, 2) ||
        !emitir(e, "ALU", "ADD x13, x2, x13"))
        return 0;

    return 1;
}

/* Traduce los accesos internos a la memoria de Spilling. */
static int acceso_spilling_ir(EstadoIR *e, const IRInstruction *i,
                              int escritura)
{
    const char *indice = escritura ? i->arg1 : i->arg2;
    const char *valor = escritura ? i->arg2 : i->result;
    int32_t slot;
    uint64_t direccion;

    if (!indice || !valor || !numero(indice, &slot) ||
        slot < 0 || (size_t)slot >= e->cantidad_slots_spilling)
        return fallo(e, i, "slot de Spilling invalido");

    direccion = (uint64_t)e->inicio_spilling +
                (uint64_t)slot * TAMANO_PALABRA;

    if (direccion > MEMORIA_TOTAL_BYTES - TAMANO_PALABRA)
        return fallo(e, i, "direccion de Spilling invalida");

    if (!constante(e, 13, (int32_t)direccion))
        return 0;

    if (escritura) {
        int rv;
        return operando_en_registro(e, valor, 3, &rv) &&
               emitir(e, "LSU", "STORE x%d, 0(x13)", rv);
    }

    {
        int rd = registro_temporal_mapeado(e, valor);
        if (rd >= 0)
            return emitir(e, "LSU", "LOAD x%d, 0(x13)", rd);

        return emitir(e, "LSU", "LOAD x3, 0(x13)") &&
               guardar(e, valor, 3);
    }
}

static int acceso_indexado_ir(EstadoIR *e, const IRInstruction *i,
                              int matriz, int escritura)
{
    const char *nombre = escritura ? i->result : i->arg1;
    const char *fila = escritura ? i->arg1 : i->arg2;
    const char *columna = matriz ? (escritura ? i->arg2 : i->extra) : NULL;
    const char *valor = escritura ? (matriz ? i->extra : i->arg2) : i->result;

    if (!nombre || !fila || !valor ||
        !direccion_indexada_ir(e, i, nombre, fila, columna, matriz))
        return fallo(e, i, "no se pudo calcular la direccion indexada");

    if (escritura) {
        int rv;
        return operando_en_registro(e, valor, 3, &rv) &&
               emitir(e, "LSU", "STORE x%d, 0(x13)", rv);
    }

    {
        int rd = registro_temporal_mapeado(e, valor);
        if (rd >= 0)
            return emitir(e, "LSU", "LOAD x%d, 0(x13)", rd);

        return emitir(e, "LSU", "LOAD x3, 0(x13)") &&
               guardar(e, valor, 3);
    }
}

static int traducir(EstadoIR *e, const IRInstruction *i)

{

    const char *operacion = NULL;



    /*

 * Guardamos el nodo de origen antes de

 * procesar la instruccion.

 *

 * De esta manera, cargar() y guardar()

 * pueden consultar el ambito correcto

 * sin modificar todas sus llamadas.

 */

    e->nodo_actual = i->nodo_origen;



    switch (i->op) {

    case IR_DECL:

        return 1; /* La memoria de variables ya fue asignada en Fase 3. */



    case IR_COPY:

        /*

 * Comprobamos por separado la carga y el

 * almacenamiento para identificar exactamente

 * donde falla una asignacion del IR.

 */

        if (i->arg1 == NULL || i->result == NULL)

            return fallo(e, i,

                "la asignacion tiene operandos incompletos");



        /* Una lista/matriz usada como argumento se pasa por referencia.
           ir_ast genera una copia a %tN para congelar argumentos; en este
           caso copiamos la DIRECCION base, no un elemento de la lista. */
        if (es_temporal(i->result) && !es_temporal(i->arg1)) {
            CodegenSimbolo origen = {0};
            if (e->config->resolver_ir &&
                e->config->resolver_ir(e->config->contexto_ir, e->nodo_actual,
                                       e->funcion, i->arg1, &origen) &&
                (origen.forma == CODEGEN_FORMA_LISTA ||
                 origen.forma == CODEGEN_FORMA_MATRIZ)) {
                int rd = registro_temporal_mapeado(e, i->result);
                int destino = rd >= 0 ? rd : IR_R1;
                return base_indexada_ir(e, i, i->arg1,
                          origen.forma == CODEGEN_FORMA_MATRIZ, destino) &&
                       (rd >= 0 || guardar(e, i->result, destino));
            }
        }

        if (e->config->asignacion_registros != NULL) {
            int rd = registro_temporal_mapeado(e, i->result);

            if (rd >= 0)
                return cargar(e, i->arg1, rd);

            {
                int rs;
                if (!operando_en_registro(e, i->arg1, 3, &rs))
                    return 0;
                return guardar(e, i->result, rs);
            }
        }

        if (!cargar(e, i->arg1, IR_R1)) {

            fprintf(stderr,

                    "IR_COPY: no se pudo cargar '%s' "

                    "dentro de '%s'\n",

                    i->arg1,

                    e->funcion ? e->funcion : "(global)");



            return fallo(e, i,

                "fallo al cargar el origen de la asignacion");

        }



        if (!guardar(e, i->result, IR_R1)) {

            fprintf(stderr,

                    "IR_COPY: no se pudo guardar en '%s' "

                    "dentro de '%s'\n",

                    i->result,

                    e->funcion ? e->funcion : "(global)");



            return fallo(e, i,

                "fallo al guardar el destino de la asignacion");

        }



        return 1;

    case IR_ADD: operacion = "ADD"; break;

    case IR_SUB: operacion = "SUB"; break;

    case IR_MUL:
        if (e->config->asignacion_registros != NULL)
            return operacion_entera_resguardada(e, i, CODEGEN_ENTERA_MUL);
        return i->arg1 && i->arg2 && i->result &&
               cargar(e, i->arg1, IR_R1) &&
               cargar(e, i->arg2, IR_R2) &&
               codegen_motor_operacion_entera(
                   e->motor, CODEGEN_ENTERA_MUL, IR_R1, IR_R2, IR_RD) &&
               guardar(e, i->result, IR_RD);
    case IR_DIV:
    case IR_MOD:
    case IR_POW:
        if (e->config->asignacion_registros != NULL)
            return operacion_entera_resguardada(
                e, i,
                i->op == IR_DIV ? CODEGEN_ENTERA_DIV :
                i->op == IR_MOD ? CODEGEN_ENTERA_MOD : CODEGEN_ENTERA_POW);
        return i->arg1 && i->arg2 && i->result &&
               cargar(e, i->arg1, IR_R1) &&
               cargar(e, i->arg2, IR_R2) &&
               codegen_motor_operacion_entera(
                   e->motor,
                   i->op == IR_DIV ? CODEGEN_ENTERA_DIV :
                   i->op == IR_MOD ? CODEGEN_ENTERA_MOD : CODEGEN_ENTERA_POW,
                   IR_R1, IR_R2, IR_RD) &&
               guardar(e, i->result, IR_RD);
    case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
        if (e->config->asignacion_registros != NULL) {
            int rd = registro_temporal_mapeado(e, i->result);
            if (rd < 0 ||
                !cargar(e, i->arg1, 2) ||
                !cargar(e, i->arg2, 3))
                return 0;
            return comparar_ir(e, i->op, 2, 3, rd);
        }
        return i->arg1 && i->arg2 && i->result &&
               cargar(e, i->arg1, IR_R1) && cargar(e, i->arg2, IR_R2) &&
               comparar_ir(e, i->op, IR_R1, IR_R2, IR_RD) &&
               guardar(e, i->result, IR_RD);

    case IR_NEG:
        if (e->config->asignacion_registros != NULL) {
            int rd = registro_temporal_mapeado(e, i->result);
            int rs;
            if (rd < 0) return 0;
            if (registro_temporal_mapeado(e, i->arg1) >= 0)
                rs = registro_temporal_mapeado(e, i->arg1);
            else {
                if (!cargar(e, i->arg1, rd)) return 0;
                rs = rd;
            }
            return emitir(e, "ALU", "SUB x%d, x0, x%d", rd, rs);
        }
        return i->arg1 && i->result && cargar(e, i->arg1, IR_R1) &&
               emitir(e, "ALU", "SUB x%d, x0, x%d", IR_RD, IR_R1) &&
               guardar(e, i->result, IR_RD);

    case IR_LABEL:

        return i->result && codegen_motor_etiqueta(e->motor, i->result);

    case IR_GOTO:

        return i->result && emitir(e, "BRU", "JMP %s", i->result);

    case IR_IF_TRUE:

    case IR_IF_FALSE:

        if (!i->arg1 || !i->result) return 0;
        if (e->config->asignacion_registros != NULL) {
            int rc;
            if (!operando_en_registro(e, i->arg1, 3, &rc)) return 0;
            return emitir(e, "BRU", "%s x%d, x0, %s",
                          i->op == IR_IF_TRUE ? "BNE" : "BEQ",
                          rc, i->result);
        }
        if (!cargar(e, i->arg1, IR_R1)) return 0;
        return emitir(e, "BRU", "%s x%d, x0, %s",
                      i->op == IR_IF_TRUE ? "BNE" : "BEQ",
                      IR_R1, i->result);



    case IR_FUNC_BEGIN:

    {

        CodegenSimbolo simbolo;



        /*

 * Iniciamos la funcion usando el marco

 * calculado por el analisis semantico.

 */

        if (e->funcion != NULL ||

            !i->result ||

            !obtener_funcion_ir(e, i->result, &simbolo))

            return fallo(e, i,

                "no se pudo preparar la funcion");



        e->funcion = i->result;



        return codegen_motor_iniciar_funcion(

            e->motor,

            e->funcion,

            simbolo.tamano_marco,

            strcmp(e->funcion, "principal") == 0);

    }



    case IR_FUNC_END:

    {

        int es_principal;

        int correcto;



        /*

 * Cerramos la funcion activa y restauramos

 * el marco de pila correspondiente.

 */

        if (!e->funcion || !i->result ||

            strcmp(e->funcion, i->result) != 0)

            return fallo(e, i,

                "final de funcion incorrecto");



        es_principal =

            strcmp(e->funcion, "principal") == 0;



        correcto = codegen_motor_finalizar_funcion(

            e->motor,

            e->funcion,

            es_principal);



        if (correcto)

            e->funcion = NULL;



        return correcto;

    }



    case IR_PARAM_DECL:

        /*

 * Los parametros ya tienen posiciones

 * asignadas durante la Fase 3.

 *

 * La declaracion no necesita emitir ASM.

 */

        return e->funcion != NULL;



    case IR_RETURN:

    {

        char etiqueta[160];

        int n;



        if (!e->funcion)

            return fallo(e, i,

                "retorno fuera de una funcion");



        /*

 * El valor devuelto se coloca en x2,

 * siguiendo el convenio del CodeGen original.

 */

        if (i->arg1) {
            int rr;
            if (e->config->asignacion_registros != NULL) {
                if (!operando_en_registro(e, i->arg1, 3, &rr) ||
                    !emitir(e, "ALU", "ADD x2, x%d, x0", rr))
                    return 0;
            } else {
                if (!cargar(e, i->arg1, IR_R1) ||
                    !emitir(e, "ALU", "ADD x2, x%d, x0", IR_R1))
                    return 0;
            }
        }



        /*

 * Principal termina el programa.

 * Las otras funciones saltan a su epilogo.

 */

        if (strcmp(e->funcion, "principal") == 0)

            return emitir(e, "BRU",

                          "JMP fin_programa");



        n = snprintf(etiqueta, sizeof(etiqueta),

                     "ret_%s", e->funcion);



        if (n < 0 || (size_t)n >= sizeof(etiqueta))

            return fallo(e, i,

                "nombre de funcion demasiado largo");



        return emitir(e, "BRU",

                      "JMP %s", etiqueta);

    }



    case IR_PARAM:
        if (!i->arg1 || e->cantidad_argumentos >= 64)
            return fallo(e, i, "argumento IR invalido o limite excedido");
        if (!congelar_argumento_ir(e, i->arg1))
            return fallo(e, i, "no se pudo congelar argumento IR");
        e->argumentos[e->cantidad_argumentos++] = i->arg1;
        return 1;

    case IR_CALL:
        return es_feist_ir(i->arg1) ? traducir_feist_ir(e, i) : llamar_ir(e, i);

    case IR_ARRAY_DECL:
    case IR_MATRIX_DECL:
        /* El analisis semantico ya reservo el espacio del simbolo. */
        return base_indexada_ir(e, i, i->result, i->op == IR_MATRIX_DECL, 13);

    case IR_ARRAY_LOAD:
        if (i->arg1 && strcmp(i->arg1, SPILL_IR_AREA) == 0)
            return acceso_spilling_ir(e, i, 0);
        return acceso_indexado_ir(e, i, 0, 0);

    case IR_ARRAY_STORE:
        if (i->result && strcmp(i->result, SPILL_IR_AREA) == 0)
            return acceso_spilling_ir(e, i, 1);
        return acceso_indexado_ir(e, i, 0, 1);
    case IR_MATRIX_LOAD:
        return acceso_indexado_ir(e, i, 1, 0);
    case IR_MATRIX_STORE:
        return acceso_indexado_ir(e, i, 1, 1);

    default:

        return fallo(e, i, "operacion IR pendiente de traducir correctamente");

    }

    if (operacion && i->arg1 && i->arg2 && i->result &&
        e->config->asignacion_registros != NULL) {
        int rd = registro_temporal_mapeado(e, i->result);
        int r1, r2;
        if (rd < 0 || !preparar_binaria_directa(
                e, i->arg1, i->arg2, rd, &r1, &r2))
            return 0;
        return emitir(e, "ALU", "%s x%d, x%d, x%d",
                      operacion, rd, r1, r2);
    }

    return operacion && i->arg1 && i->arg2 && i->result &&
           cargar(e, i->arg1, IR_R1) && cargar(e, i->arg2, IR_R2) &&
           emitir(e, "ALU", "%s x%d, x%d, x%d",
                  operacion, IR_RD, IR_R1, IR_R2) &&
           guardar(e, i->result, IR_RD);

}



int codegen_ir_generar_archivo(const IRProgram *programa,

                              const char *ruta_salida,

                              const CodegenIRConfig *config,

                              CodegenIREstadisticas *estadisticas)

{

    EstadoIR e = {0};

    int ok = 1;

    if (!programa || !ruta_salida || !config ||

        (programa->count && !programa->instructions)) return 0;

    e.config = config;

    e.stats.instrucciones_ir = programa->count;

    if (!preparar(&e, programa)) {

        fprintf(stderr, "CodeGen IR: memoria insuficiente para temporales\n");

        ok = 0;

        goto fin;

    }



    /*

 * Validamos las funciones antes de abrir

 * el archivo ASM.

 *

 * Asi detectamos problemas estructurales

 * sin generar instrucciones incompletas.

 */

    if (!validar_funciones(&e, programa)) {

        fprintf(stderr,

                "CodeGen IR: la estructura de funciones no es valida\n");

        ok = 0;

        goto fin;

    }



    e.motor = codegen_motor_crear(ruta_salida, &config->original);

    if (!e.motor) {

        ok = 0;

        goto fin;

    }





    /*

 * El programa debe comenzar ejecutando principal.

 *

 * Como el IR puede contener definiciones de funciones

 * antes de principal, saltamos directamente al punto

 * de entrada para no ejecutar esas funciones por error.

 *

 * Cuando una funcion sea llamada, utilizara su propia

 * etiqueta func_nombre.

 */

    if (!emitir(&e, "BRU", "JMP principal")) {

        fprintf(stderr,

                "CodeGen IR: no se pudo emitir el salto inicial\n");

        ok = 0;

    }





    for (size_t j = 0; ok && j < programa->count; j++) {

        if (!traducir(&e, &programa->instructions[j])) {

            fprintf(stderr, "CodeGen IR: fallo en instruccion %zu (op=%d, linea=%d)\n",

                    j, programa->instructions[j].op, programa->instructions[j].source_line);

            ok = 0;

        } else e.stats.instrucciones_traducidas++;

    }

    if (ok && (!codegen_motor_etiqueta(e.motor, "fin_programa") ||

               !emitir(&e, "BRU", "JMP fin_programa"))) ok = 0;

    /* El motor aun no expone el conteo de bundles; no inventamos la cifra. */

    e.stats.bundles_emitidos = 0;

    if (!codegen_motor_cerrar(e.motor)) ok = 0;

    e.motor = NULL;

    if (e.error) ok = 0;

    if (!ok) remove(ruta_salida); /* Evitamos conservar un ASM incompleto. */

fin:

    for (size_t j = 0; j < e.cantidad_temporales; j++) free(e.temporales[j]);

    free(e.temporales);

    if (estadisticas) *estadisticas = e.stats;

    return ok;

}