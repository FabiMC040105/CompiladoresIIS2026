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



    return fin + e->cantidad_temporales * 4u

           < MEMORIA_TOTAL_BYTES;



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

                        e->config->original.fin_globales))

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
static int comparar_ir(EstadoIR *e, IROp op)
{
    char verdadero[80], fin[80];
    const char *salto = NULL;
    int izq = IR_R1, der = IR_R2;

    if (!etiqueta_auxiliar(e, "cmp_true", verdadero, sizeof(verdadero)) ||
        !etiqueta_auxiliar(e, "cmp_end", fin, sizeof(fin)))
        return 0;

    switch (op) {
    case IR_EQ: salto = "BEQ"; break;
    case IR_NE: salto = "BNE"; break;
    case IR_LE: salto = "BLE"; break;
    case IR_GE: salto = "BGE"; break;
    case IR_LT: salto = "BGE"; izq = IR_R2; der = IR_R1; break; /* b >= a no equivale a a < b si son iguales: usar BLE con b,a y negar igualdad */
    case IR_GT: salto = "BGE"; izq = IR_R1; der = IR_R2; break;
    default: return 0;
    }
    /* Las relaciones estrictas se expresan como <= y !=. */
    if (op == IR_LT || op == IR_GT) {
        if (!emitir(e, "ALU", "ADDI x%d, x0, 0", IR_RD) ||
            !emitir(e, "BRU", "BEQ x%d, x%d, %s", IR_R1, IR_R2, fin) ||
            !emitir(e, "BRU", "%s x%d, x%d, %s", op == IR_LT ? "BLE" : "BGE", izq, der, verdadero) ||
            !emitir(e, "BRU", "JMP %s", fin) ||
            !codegen_motor_etiqueta(e->motor, verdadero) ||
            !emitir(e, "ALU", "ADDI x%d, x0, 1", IR_RD) ||
            !codegen_motor_etiqueta(e->motor, fin)) return 0;
        return 1;
    }
    return emitir(e, "ALU", "ADDI x%d, x0, 0", IR_RD) &&
           emitir(e, "BRU", "%s x%d, x%d, %s", salto, izq, der, verdadero) &&
           emitir(e, "BRU", "JMP %s", fin) &&
           codegen_motor_etiqueta(e->motor, verdadero) &&
           emitir(e, "ALU", "ADDI x%d, x0, 1", IR_RD) &&
           codegen_motor_etiqueta(e->motor, fin);
}

/*
 * Multiplicacion de 32 bits por desplazamiento y suma.
 * Trabajamos con los patrones binarios de los operandos: los 32 bits
 * bajos coinciden con la multiplicacion con signo en complemento a dos.
 * x7 guarda el acumulador y x8 el bit menos significativo.
 */
static int multiplicar_ir(EstadoIR *e)
{
    char bucle[80], omitir[80], fin[80];
    if (!etiqueta_auxiliar(e, "mul_loop", bucle, sizeof(bucle)) ||
        !etiqueta_auxiliar(e, "mul_skip", omitir, sizeof(omitir)) ||
        !etiqueta_auxiliar(e, "mul_end", fin, sizeof(fin))) return 0;
    return emitir(e, "ALU", "ADDI x7, x0, 0") &&
           emitir(e, "ALU", "ADDI x8, x0, 1") &&
           codegen_motor_etiqueta(e->motor, bucle) &&
           emitir(e, "BRU", "BEQ x%d, x0, %s", IR_R2, fin) &&
           emitir(e, "ALU", "AND x8, x%d, x8", IR_R2) &&
           emitir(e, "BRU", "BEQ x8, x0, %s", omitir) &&
           emitir(e, "ALU", "ADD x7, x7, x%d", IR_R1) &&
           codegen_motor_etiqueta(e->motor, omitir) &&
           emitir(e, "ALU", "SLLI x%d, x%d, 1", IR_R1, IR_R1) &&
           emitir(e, "ALU", "SRLI x%d, x%d, 1", IR_R2, IR_R2) &&
           emitir(e, "ALU", "ADDI x8, x0, 1") &&
           emitir(e, "BRU", "JMP %s", bucle) &&
           codegen_motor_etiqueta(e->motor, fin) &&
           emitir(e, "ALU", "ADD x%d, x7, x0", IR_RD);
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

    /* Primero se guardan los registros del llamador. */
    for (int reg = 4; reg <= 12; ++reg)
        if (!apilar_ir(e, reg)) return 0;

    /* Preservamos temporales IR, incluso durante llamadas recursivas. */
    for (size_t t = 0; t < e->cantidad_temporales; ++t)
        if (!cargar(e, e->temporales[t], IR_R1) || !apilar_ir(e, IR_R1)) return 0;

    /* El primer argumento debe quedar en 8(FP) de la funcion llamada. */
    for (size_t a = n; a > 0; --a)
        if (!cargar(e, e->argumentos[a - 1], IR_R1) || !apilar_ir(e, IR_R1)) return 0;

    if (!emitir(e, "ALU", "ADDI x14, x14, -4") ||
        !emitir(e, "ALU", "ADDI x3, x1, %s", retorno) ||
        !emitir(e, "LSU", "STORE x3, 0(x14)") ||
        !emitir(e, "BRU", "JMP %s", destino) ||
        !codegen_motor_etiqueta(e->motor, retorno)) return 0;

    /* El callee ya retiro su cabecera; retiramos los argumentos. */
    for (size_t a = 0; a < n; ++a)
        if (!emitir(e, "ALU", "ADDI x14, x14, 4")) return 0;

    /* Se conserva x2 mientras se restauran los datos del llamador. */
    for (size_t t = e->cantidad_temporales; t > 0; --t)
        if (!desapilar_ir(e, IR_R1) ||
            !guardar(e, e->temporales[t - 1], IR_R1)) return 0;
    for (int reg = 12; reg >= 4; --reg)
        if (!desapilar_ir(e, reg)) return 0;

    e->cantidad_argumentos = 0;
    return !i->result || guardar(e, i->result, 2);
}

/* Calcula la base real de una lista o matriz, incluyendo parametros por referencia. */
static int base_indexada_ir(EstadoIR *e, const IRInstruction *i,
                            const char *nombre, int matriz)
{
    CodegenSimbolo s;
    if (!nombre || !e->config || !e->config->resolver_ir ||
        !e->config->resolver_ir(e->config->contexto_ir, e->nodo_actual,
                                e->funcion, nombre, &s))
        return fallo(e, i, "no se encontro el arreglo en su ambito");
    if (s.forma != (matriz ? CODEGEN_FORMA_MATRIZ : CODEGEN_FORMA_LISTA) ||
        s.cantidad_dimensiones < (matriz ? 2u : 1u))
        return fallo(e, i, "forma o dimensiones del arreglo incompatibles");
    if (s.ubicacion == CODEGEN_UBICACION_GLOBAL) {
        if (s.direccion < INT32_MIN || s.direccion > INT32_MAX) return 0;
        return constante(e, IR_DIR, (int32_t)s.direccion);
    }
    if (s.ubicacion != CODEGEN_UBICACION_LOCAL &&
        s.ubicacion != CODEGEN_UBICACION_PARAMETRO) return 0;
    if (s.direccion < INT32_MIN || s.direccion > INT32_MAX) return 0;
    if (!constante(e, IR_DIR, (int32_t)s.direccion) ||
        !emitir(e, "ALU", "ADD x%d, x15, x%d", IR_DIR, IR_DIR)) return 0;
    if (s.parametro_por_referencia)
        return emitir(e, "LSU", "LOAD x%d, 0(x%d)", IR_DIR, IR_DIR);
    return 1;
}

/* Genera la direccion del elemento en x11, preservandola ante cargar(). */
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
    if (matriz) {
        if (!columna || s.cantidad_dimensiones < 2 || !s.dimensiones[1] ||
            s.dimensiones[1] > INT32_MAX) return fallo(e, i, "columnas invalidas");
        /* Guardamos columna antes de multiplicar fila por cantidad de columnas. */
        if (!cargar(e, columna, 10) || !cargar(e, fila, IR_R1) ||
            !constante(e, IR_R2, (int32_t)s.dimensiones[1]) ||
            !multiplicar_ir(e) ||
            !emitir(e, "ALU", "ADD x6, x6, x10") ||
            !emitir(e, "ALU", "SLLI x6, x6, 2")) return 0;
    } else {
        if (!cargar(e, fila, 6) ||
            !emitir(e, "ALU", "SLLI x6, x6, 2")) return 0;
    }
    /* x6 es el desplazamiento y no se modifica al obtener la base. */
    return base_indexada_ir(e, i, nombre, matriz) &&
           emitir(e, "ALU", "ADD x11, x%d, x6", IR_DIR);
}

/* Traduce lectura/escritura; IR almacena la segunda dimension en extra. */
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
        /* cargar() puede modificar x13, por eso guardamos la direccion en x11. */
        return cargar(e, valor, 4) && emitir(e, "LSU", "STORE x4, 0(x11)");
    }
    return emitir(e, "LSU", "LOAD x4, 0(x11)") && guardar(e, valor, 4);
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
        return i->arg1 && i->arg2 && i->result &&
               cargar(e, i->arg1, IR_R1) && cargar(e, i->arg2, IR_R2) &&
               multiplicar_ir(e) && guardar(e, i->result, IR_RD);
    case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
        return i->arg1 && i->arg2 && i->result &&
               cargar(e, i->arg1, IR_R1) && cargar(e, i->arg2, IR_R2) &&
               comparar_ir(e, i->op) && guardar(e, i->result, IR_RD);

    case IR_NEG:

        return i->arg1 && i->result && cargar(e, i->arg1, IR_R1) &&

               emitir(e, "ALU", "SUB x%d, x0, x%d", IR_RD, IR_R1) &&

               guardar(e, i->result, IR_RD);

    case IR_LABEL:

        return i->result && codegen_motor_etiqueta(e->motor, i->result);

    case IR_GOTO:

        return i->result && emitir(e, "BRU", "JMP %s", i->result);

    case IR_IF_TRUE:

    case IR_IF_FALSE:

        if (!i->arg1 || !i->result || !cargar(e, i->arg1, IR_R1)) return 0;

        return emitir(e, "BRU", "%s x%d, x0, %s",

                      i->op == IR_IF_TRUE ? "BNE" : "BEQ", IR_R1, i->result);



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

            if (!cargar(e, i->arg1, IR_R1) ||

                !emitir(e, "ALU",

                        "ADD x2, x%d, x0", IR_R1))

                return 0;

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
        e->argumentos[e->cantidad_argumentos++] = i->arg1;
        return 1;

    case IR_CALL:
        return llamar_ir(e, i);

    case IR_ARRAY_DECL:
    case IR_MATRIX_DECL:
        /* El analisis semantico ya reservo el espacio del simbolo. */
        return base_indexada_ir(e, i, i->result, i->op == IR_MATRIX_DECL);

    case IR_ARRAY_LOAD:
        return acceso_indexado_ir(e, i, 0, 0);
    case IR_ARRAY_STORE:
        return acceso_indexado_ir(e, i, 0, 1);
    case IR_MATRIX_LOAD:
        return acceso_indexado_ir(e, i, 1, 0);
    case IR_MATRIX_STORE:
        return acceso_indexado_ir(e, i, 1, 1);

    default:

        return fallo(e, i, "operacion IR pendiente de traducir correctamente");

    }

    return operacion && i->arg1 && i->arg2 && i->result &&

           cargar(e, i->arg1, IR_R1) && cargar(e, i->arg2, IR_R2) &&

           emitir(e, "ALU", "%s x%d, x%d, x%d", operacion, IR_RD, IR_R1, IR_R2) &&

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