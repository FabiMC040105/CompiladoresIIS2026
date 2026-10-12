#include "codegen.h"
#include "asm_emitter.h"
#include "memoria.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_RETORNO_VALOR 2
#define REG_AUXILIAR 3
#define REG_TEMP_PRIMERO 4
#define REG_TEMP_ULTIMO 12
#define REG_DIRECCION 13
#define REG_SP 14
#define REG_FP 15
#define NOPS_POR_DEFECTO 4
#define MAX_NIVELES_BUCLE 32
#define MAX_PARAMETROS_FUNCION 64
#define TAMANO_ELEMENTO_BYTES 4u
#define PILA_INICIAL_DEFECTO MEMORIA_TOTAL_BYTES
#define BYTES_TEMPORALES_GUARDADOS ((REG_TEMP_ULTIMO - REG_TEMP_PRIMERO + 1) * 4u)

typedef struct {
    AsmEmitter emisor;
    CodegenConfig config;
    unsigned char temp_ocupado[16];
    int errores;
    int encontro_principal;
    unsigned contador_etiquetas;
    NodoAST *raiz;
    const char *funcion_actual;
    char etiqueta_retorno_funcion[96];
    int dentro_funcion;
    struct {
        char etiqueta_break[64];
        char etiqueta_continue[64];
    } pila_bucles[MAX_NIVELES_BUCLE];
    unsigned profundidad_bucle;
    int usa_error_pila;
} Codegen;

static NodoAST *hijo_n(NodoAST *nodo, unsigned indice)
{
    NodoAST *hijo;
    unsigned i;

    if (nodo == NULL)
        return NULL;

    hijo = nodo->hijo;
    for (i = 0; hijo != NULL && i < indice; i++)
        hijo = hijo->siguiente;

    return hijo;
}

static void error_codegen(Codegen *cg, NodoAST *nodo, const char *formato, ...)
{
    va_list args;

    fprintf(stderr, "Error de generacion");
    if (nodo != NULL)
        fprintf(stderr, " en linea %d", nodo->linea);
    fputs(": ", stderr);

    va_start(args, formato);
    vfprintf(stderr, formato, args);
    va_end(args);

    fputc('\n', stderr);
    cg->errores++;
}


static int apilar_bucle(Codegen *cg,
                        NodoAST *nodo,
                        const char *etiqueta_break,
                        const char *etiqueta_continue)
{
    if (cg->profundidad_bucle >= MAX_NIVELES_BUCLE) {
        error_codegen(cg, nodo,
                      "demasiados ciclos anidados (maximo %d)",
                      MAX_NIVELES_BUCLE);
        return 0;
    }

    snprintf(cg->pila_bucles[cg->profundidad_bucle].etiqueta_break,
             sizeof(cg->pila_bucles[cg->profundidad_bucle].etiqueta_break),
             "%s", etiqueta_break);

    snprintf(cg->pila_bucles[cg->profundidad_bucle].etiqueta_continue,
             sizeof(cg->pila_bucles[cg->profundidad_bucle].etiqueta_continue),
             "%s", etiqueta_continue);

    cg->profundidad_bucle++;
    return 1;
}

static void desapilar_bucle(Codegen *cg)
{
    if (cg->profundidad_bucle > 0)
        cg->profundidad_bucle--;
}

static const char *etiqueta_break_actual(const Codegen *cg)
{
    if (cg->profundidad_bucle == 0)
        return NULL;

    return cg->pila_bucles[cg->profundidad_bucle - 1].etiqueta_break;
}

static int reservar_temp(Codegen *cg, NodoAST *nodo)
{
    int reg;

    for (reg = REG_TEMP_PRIMERO; reg <= REG_TEMP_ULTIMO; reg++) {
        if (!cg->temp_ocupado[reg]) {
            cg->temp_ocupado[reg] = 1;
            return reg;
        }
    }

    error_codegen(cg, nodo,
                  "se agotaron los registros temporales x%d-x%d",
                  REG_TEMP_PRIMERO, REG_TEMP_ULTIMO);
    return -1;
}

static void liberar_temp(Codegen *cg, int reg)
{
    if (reg >= REG_TEMP_PRIMERO && reg <= REG_TEMP_ULTIMO)
        cg->temp_ocupado[reg] = 0;
}

static void nueva_etiqueta(Codegen *cg,
                           const char *prefijo,
                           char *destino,
                           size_t capacidad)
{
    snprintf(destino, capacidad, "%s_%u",
             prefijo, cg->contador_etiquetas++);
}

static int emitir_instruccion(Codegen *cg,
                              const char *slot,
                              const char *formato,
                              ...)
{
    char texto[256];
    va_list args;
    int n;

    va_start(args, formato);
    n = vsnprintf(texto, sizeof(texto), formato, args);
    va_end(args);

    if (n < 0 || (size_t)n >= sizeof(texto)) {
        cg->errores++;
        return 0;
    }

    if (!asm_emitir_slot(&cg->emisor, slot, "%s", texto)) {
        cg->errores++;
        return 0;
    }

    if (cg->config.nops_dependencia > 0 &&
        !asm_emitir_nops(&cg->emisor, cg->config.nops_dependencia)) {
        cg->errores++;
        return 0;
    }

    return 1;
}

static int emitir_constante(Codegen *cg, int reg, int32_t valor)
{
    uint32_t bits;
    unsigned byte0, byte1, byte2, byte3;

    if (valor >= -2048 && valor <= 2047)
        return emitir_instruccion(cg, "ALU",
                                  "ADDI x%d, x0, %d", reg, valor);

    /* Construccion portable de los 32 bits con desplazamientos de 8. */
    bits = (uint32_t)valor;
    byte0 = (bits >> 24) & 0xFFu;
    byte1 = (bits >> 16) & 0xFFu;
    byte2 = (bits >> 8) & 0xFFu;
    byte3 = bits & 0xFFu;

    if (!emitir_instruccion(cg, "ALU", "ADDI x%d, x0, %u", reg, byte0))
        return 0;
    if (!emitir_instruccion(cg, "ALU", "SLLI x%d, x%d, 8", reg, reg))
        return 0;
    if (!emitir_instruccion(cg, "ALU", "ADDI x%d, x%d, %u", reg, reg, byte1))
        return 0;
    if (!emitir_instruccion(cg, "ALU", "SLLI x%d, x%d, 8", reg, reg))
        return 0;
    if (!emitir_instruccion(cg, "ALU", "ADDI x%d, x%d, %u", reg, reg, byte2))
        return 0;
    if (!emitir_instruccion(cg, "ALU", "SLLI x%d, x%d, 8", reg, reg))
        return 0;
    return emitir_instruccion(cg, "ALU", "ADDI x%d, x%d, %u", reg, reg, byte3);
}

static int resolver(Codegen *cg,
                    NodoAST *nodo,
                    const char *nombre,
                    CodegenSimbolo *simbolo)
{
    int encontrado = 0;

    if (cg->config.resolver_simbolo_nodo == NULL &&
        cg->config.resolver_simbolo == NULL) {
        error_codegen(cg, nodo,
                      "no hay un resolvedor de simbolos conectado para '%s'",
                      nombre);
        return 0;
    }

    /*
     * Inicializar los campos ampliados del contrato. Asi los resolvedores
     * de las pruebas anteriores, que solo rellenan nombre/direccion/tipo,
     * siguen representando correctamente un escalar.
     */
    memset(simbolo, 0, sizeof(*simbolo));
    simbolo->forma = CODEGEN_FORMA_ESCALAR;
    simbolo->categoria = CODEGEN_CAT_VARIABLE;
    simbolo->ubicacion = CODEGEN_UBICACION_GLOBAL;

    /*
     * La integracion real siempre prefiere el resolvedor por nodo porque
     * conoce el ambito exacto del identificador. El resolvedor legado queda
     * disponible solamente para las pruebas de los pasos anteriores.
     */
    if (cg->config.resolver_simbolo_nodo != NULL) {
        encontrado = cg->config.resolver_simbolo_nodo(
            cg->config.contexto_simbolos, nodo, nombre, simbolo);
    } else if (cg->config.resolver_simbolo != NULL) {
        encontrado = cg->config.resolver_simbolo(
            cg->config.contexto_simbolos, nombre, simbolo);
    }

    if (!encontrado) {
        error_codegen(cg, nodo, "simbolo no encontrado: %s", nombre);
        return 0;
    }

    return 1;
}

static int offset_cabe_imm12(long offset)
{
    return offset >= -2048 && offset <= 2047;
}

static int emitir_ajuste_sp(Codegen *cg, long delta)
{
    if (delta == 0)
        return 1;

    if (offset_cabe_imm12(delta))
        return emitir_instruccion(cg, "ALU",
                                  "ADDI x%d, x%d, %ld",
                                  REG_SP, REG_SP, delta);

    if (delta > INT32_MAX || delta < INT32_MIN) {
        cg->errores++;
        return 0;
    }

    if (!emitir_constante(cg, REG_AUXILIAR, (int32_t)delta))
        return 0;

    return emitir_instruccion(cg, "ALU",
                              "ADD x%d, x%d, x%d",
                              REG_SP, REG_SP, REG_AUXILIAR);
}

static int emitir_direccion_fp(Codegen *cg, int reg_destino, long offset)
{
    if (offset_cabe_imm12(offset))
        return emitir_instruccion(cg, "ALU",
                                  "ADDI x%d, x%d, %ld",
                                  reg_destino, REG_FP, offset);

    if (offset > INT32_MAX || offset < INT32_MIN)
        return 0;

    if (!emitir_constante(cg, REG_AUXILIAR, (int32_t)offset))
        return 0;

    return emitir_instruccion(cg, "ALU",
                              "ADD x%d, x%d, x%d",
                              reg_destino, REG_FP, REG_AUXILIAR);
}

/*
 * Deja en reg_destino la direccion base real del simbolo.
 * - Global: direccion absoluta.
 * - Local:  FP + offset negativo.
 * - Parametro escalar: FP + offset positivo (direccion del slot).
 * - Parametro lista/matriz: el slot contiene una direccion y se desreferencia.
 */
static int emitir_base_simbolo(Codegen *cg,
                               NodoAST *nodo,
                               const CodegenSimbolo *simbolo,
                               int reg_destino)
{
    if (simbolo->ubicacion == CODEGEN_UBICACION_GLOBAL) {
        if (simbolo->direccion < INT32_MIN || simbolo->direccion > INT32_MAX) {
            error_codegen(cg, nodo, "direccion global fuera de rango");
            return 0;
        }
        return emitir_constante(cg, reg_destino, (int32_t)simbolo->direccion);
    }

    if (simbolo->ubicacion == CODEGEN_UBICACION_LOCAL) {
        if (!emitir_direccion_fp(cg, reg_destino, simbolo->direccion)) {
            error_codegen(cg, nodo, "offset local fuera de rango");
            return 0;
        }
        return 1;
    }

    if (simbolo->ubicacion == CODEGEN_UBICACION_PARAMETRO) {
        if (simbolo->parametro_por_referencia) {
            if (offset_cabe_imm12(simbolo->direccion))
                return emitir_instruccion(cg, "LSU",
                                          "LOAD x%d, %ld(x%d)",
                                          reg_destino,
                                          simbolo->direccion,
                                          REG_FP);

            if (!emitir_direccion_fp(cg, reg_destino, simbolo->direccion))
                return 0;
            return emitir_instruccion(cg, "LSU",
                                      "LOAD x%d, 0(x%d)",
                                      reg_destino, reg_destino);
        }

        return emitir_direccion_fp(cg, reg_destino, simbolo->direccion);
    }

    error_codegen(cg, nodo, "el simbolo '%s' no tiene almacenamiento de datos",
                  simbolo->nombre != NULL ? simbolo->nombre : "?");
    return 0;
}

static int cargar_variable(Codegen *cg, NodoAST *nodo, const char *nombre)
{
    CodegenSimbolo simbolo;
    int reg;

    if (!resolver(cg, nodo, nombre, &simbolo))
        return -1;

    reg = reservar_temp(cg, nodo);
    if (reg < 0)
        return -1;

    if (simbolo.ubicacion == CODEGEN_UBICACION_GLOBAL) {
        if (!emitir_base_simbolo(cg, nodo, &simbolo, REG_DIRECCION) ||
            !emitir_instruccion(cg, "LSU",
                                "LOAD x%d, 0(x%d)", reg, REG_DIRECCION)) {
            liberar_temp(cg, reg);
            return -1;
        }
        return reg;
    }

    if ((simbolo.ubicacion == CODEGEN_UBICACION_LOCAL ||
         simbolo.ubicacion == CODEGEN_UBICACION_PARAMETRO) &&
        !simbolo.parametro_por_referencia &&
        offset_cabe_imm12(simbolo.direccion)) {
        if (!emitir_instruccion(cg, "LSU",
                                "LOAD x%d, %ld(x%d)",
                                reg, simbolo.direccion, REG_FP)) {
            liberar_temp(cg, reg);
            return -1;
        }
        return reg;
    }

    if (!emitir_base_simbolo(cg, nodo, &simbolo, REG_DIRECCION) ||
        !emitir_instruccion(cg, "LSU",
                            "LOAD x%d, 0(x%d)", reg, REG_DIRECCION)) {
        liberar_temp(cg, reg);
        return -1;
    }

    return reg;
}

static int guardar_variable(Codegen *cg,
                            NodoAST *identificador,
                            const char *nombre,
                            int reg_valor)
{
    CodegenSimbolo simbolo;

    if (!resolver(cg, identificador, nombre, &simbolo))
        return 0;

    if (simbolo.parametro_por_referencia) {
        error_codegen(cg, identificador,
                      "no se puede asignar un arreglo completo a '%s'",
                      nombre);
        return 0;
    }

    if (simbolo.ubicacion == CODEGEN_UBICACION_GLOBAL) {
        if (!emitir_base_simbolo(cg, identificador, &simbolo, REG_DIRECCION))
            return 0;
        return emitir_instruccion(cg, "LSU",
                                  "STORE x%d, 0(x%d)",
                                  reg_valor, REG_DIRECCION);
    }

    if ((simbolo.ubicacion == CODEGEN_UBICACION_LOCAL ||
         simbolo.ubicacion == CODEGEN_UBICACION_PARAMETRO) &&
        offset_cabe_imm12(simbolo.direccion))
        return emitir_instruccion(cg, "LSU",
                                  "STORE x%d, %ld(x%d)",
                                  reg_valor, simbolo.direccion, REG_FP);

    if (!emitir_base_simbolo(cg, identificador, &simbolo, REG_DIRECCION))
        return 0;

    return emitir_instruccion(cg, "LSU",
                              "STORE x%d, 0(x%d)",
                              reg_valor, REG_DIRECCION);
}



/* Busca una definicion de funcion de nivel superior por nombre. */
static NodoAST *buscar_funcion_ast(Codegen *cg, const char *nombre)
{
    NodoAST *nodo;

    if (cg == NULL || cg->raiz == NULL || nombre == NULL)
        return NULL;

    nodo = (cg->raiz->tipo == AST_PROGRAMA) ? cg->raiz->hijo : cg->raiz;

    while (nodo != NULL) {
        if (nodo->tipo == AST_FUNCION) {
            NodoAST *identificador = hijo_n(nodo, 0);
            if (identificador != NULL && identificador->valor != NULL &&
                strcmp(identificador->valor, nombre) == 0)
                return nodo;
        }
        nodo = nodo->siguiente;
    }

    return NULL;
}

static int generar_expresion(Codegen *cg, NodoAST *nodo);
static int generar_llamada(Codegen *cg, NodoAST *nodo, int necesita_resultado);

static int es_comparador(const char *op)
{
    return op != NULL &&
           (strcmp(op, "==") == 0 ||
            strcmp(op, "!=") == 0 ||
            strcmp(op, "<") == 0 ||
            strcmp(op, ">") == 0 ||
            strcmp(op, "<=") == 0 ||
            strcmp(op, ">=") == 0);
}

/*
 * Emite un salto basado en dos registros ya calculados.
 * Devuelve 1 si pudo generar la comparación.
 *
 * El ISA dispone directamente de BEQ, BNE, BGE y BLE.
 * Para los casos estrictos que no tienen branch directo se usa SLT.
 */
static int emitir_salto_comparacion(Codegen *cg,
                                    NodoAST *nodo,
                                    const char *op,
                                    int izq,
                                    int der,
                                    const char *etiqueta,
                                    int saltar_si_verdadero)
{
    if (saltar_si_verdadero) {
        if (strcmp(op, "==") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BEQ x%d, x%d, %s",
                                      izq, der, etiqueta);
        if (strcmp(op, "!=") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BNE x%d, x%d, %s",
                                      izq, der, etiqueta);
        if (strcmp(op, ">=") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BGE x%d, x%d, %s",
                                      izq, der, etiqueta);
        if (strcmp(op, "<=") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BLE x%d, x%d, %s",
                                      izq, der, etiqueta);

        if (strcmp(op, "<") == 0) {
            if (!emitir_instruccion(cg, "ALU",
                                    "SLT x%d, x%d, x%d", izq, izq, der))
                return 0;
            return emitir_instruccion(cg, "BRU",
                                      "BNE x%d, x0, %s", izq, etiqueta);
        }

        if (strcmp(op, ">") == 0) {
            if (!emitir_instruccion(cg, "ALU",
                                    "SLT x%d, x%d, x%d", izq, der, izq))
                return 0;
            return emitir_instruccion(cg, "BRU",
                                      "BNE x%d, x0, %s", izq, etiqueta);
        }
    } else {
        /* Saltar cuando la comparación original sea falsa. */
        if (strcmp(op, "==") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BNE x%d, x%d, %s",
                                      izq, der, etiqueta);
        if (strcmp(op, "!=") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BEQ x%d, x%d, %s",
                                      izq, der, etiqueta);
        if (strcmp(op, "<") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BGE x%d, x%d, %s",
                                      izq, der, etiqueta);
        if (strcmp(op, ">") == 0)
            return emitir_instruccion(cg, "BRU",
                                      "BLE x%d, x%d, %s",
                                      izq, der, etiqueta);

        /* !(a <= b) equivale a b < a. */
        if (strcmp(op, "<=") == 0) {
            if (!emitir_instruccion(cg, "ALU",
                                    "SLT x%d, x%d, x%d", izq, der, izq))
                return 0;
            return emitir_instruccion(cg, "BRU",
                                      "BNE x%d, x0, %s", izq, etiqueta);
        }

        /* !(a >= b) equivale a a < b. */
        if (strcmp(op, ">=") == 0) {
            if (!emitir_instruccion(cg, "ALU",
                                    "SLT x%d, x%d, x%d", izq, izq, der))
                return 0;
            return emitir_instruccion(cg, "BRU",
                                      "BNE x%d, x0, %s", izq, etiqueta);
        }
    }

    error_codegen(cg, nodo, "comparador no soportado: %s", op);
    return 0;
}

/*
 * Genera un salto a etiqueta si una condición es verdadera o falsa.
 * AND y OR se resuelven con cortocircuito, lo que evita tener que
 * materializar siempre un booleano en un registro.
 */
static int emitir_salto_condicion(Codegen *cg,
                                  NodoAST *nodo,
                                  const char *etiqueta,
                                  int saltar_si_verdadero)
{
    NodoAST *izq_nodo;
    NodoAST *der_nodo;
    int izq, der, ok, reg;
    char etiqueta_aux[64];

    if (nodo == NULL) {
        error_codegen(cg, nodo, "condicion vacia");
        return 0;
    }

    if (nodo->tipo == AST_OPERACION &&
        nodo->valor != NULL &&
        strcmp(nodo->valor, "AND") == 0) {
        izq_nodo = hijo_n(nodo, 0);
        der_nodo = hijo_n(nodo, 1);

        if (!saltar_si_verdadero) {
            /* AND es falso si cualquiera de sus lados es falso. */
            return emitir_salto_condicion(cg, izq_nodo, etiqueta, 0) &&
                   emitir_salto_condicion(cg, der_nodo, etiqueta, 0);
        }

        /* Para ser verdadero, ambos deben ser verdaderos. */
        nueva_etiqueta(cg, "fin_and_falso", etiqueta_aux,
                       sizeof(etiqueta_aux));
        if (!emitir_salto_condicion(cg, izq_nodo, etiqueta_aux, 0))
            return 0;
        if (!emitir_salto_condicion(cg, der_nodo, etiqueta, 1))
            return 0;
        return asm_emitir_etiqueta(&cg->emisor, etiqueta_aux);
    }

    if (nodo->tipo == AST_OPERACION &&
        nodo->valor != NULL &&
        strcmp(nodo->valor, "OR") == 0) {
        izq_nodo = hijo_n(nodo, 0);
        der_nodo = hijo_n(nodo, 1);

        if (saltar_si_verdadero) {
            /* OR es verdadero si cualquiera de sus lados es verdadero. */
            return emitir_salto_condicion(cg, izq_nodo, etiqueta, 1) &&
                   emitir_salto_condicion(cg, der_nodo, etiqueta, 1);
        }

        /* Para ser falso, ambos deben ser falsos. */
        nueva_etiqueta(cg, "fin_or_verdadero", etiqueta_aux,
                       sizeof(etiqueta_aux));
        if (!emitir_salto_condicion(cg, izq_nodo, etiqueta_aux, 1))
            return 0;
        if (!emitir_salto_condicion(cg, der_nodo, etiqueta, 0))
            return 0;
        return asm_emitir_etiqueta(&cg->emisor, etiqueta_aux);
    }

    if (nodo->tipo == AST_OPERACION &&
        es_comparador(nodo->valor)) {
        izq = generar_expresion(cg, hijo_n(nodo, 0));
        if (izq < 0)
            return 0;

        der = generar_expresion(cg, hijo_n(nodo, 1));
        if (der < 0) {
            liberar_temp(cg, izq);
            return 0;
        }

        ok = emitir_salto_comparacion(cg, nodo, nodo->valor,
                                      izq, der, etiqueta,
                                      saltar_si_verdadero);
        liberar_temp(cg, der);
        liberar_temp(cg, izq);
        return ok;
    }

    /* Cualquier otra expresión se interpreta como falsa si vale cero. */
    reg = generar_expresion(cg, nodo);
    if (reg < 0)
        return 0;

    if (saltar_si_verdadero)
        ok = emitir_instruccion(cg, "BRU",
                                "BNE x%d, x0, %s", reg, etiqueta);
    else
        ok = emitir_instruccion(cg, "BRU",
                                "BEQ x%d, x0, %s", reg, etiqueta);

    liberar_temp(cg, reg);
    return ok;
}

static int generar_booleano_desde_condicion(Codegen *cg, NodoAST *nodo)
{
    int reg;
    char etiqueta_verdadero[64];
    char etiqueta_fin[64];

    reg = reservar_temp(cg, nodo);
    if (reg < 0)
        return -1;

    nueva_etiqueta(cg, "bool_verdadero", etiqueta_verdadero,
                   sizeof(etiqueta_verdadero));
    nueva_etiqueta(cg, "fin_bool", etiqueta_fin, sizeof(etiqueta_fin));

    if (!emitir_salto_condicion(cg, nodo, etiqueta_verdadero, 1) ||
        !emitir_constante(cg, reg, 0) ||
        !emitir_instruccion(cg, "BRU", "JMP %s", etiqueta_fin) ||
        !asm_emitir_etiqueta(&cg->emisor, etiqueta_verdadero) ||
        !emitir_constante(cg, reg, 1) ||
        !asm_emitir_etiqueta(&cg->emisor, etiqueta_fin)) {
        liberar_temp(cg, reg);
        return -1;
    }

    return reg;
}


/*
 * Multiplicacion entera de 32 bits implementada unicamente con
 * instrucciones que existen en la ISA del procesador.
 *
 * Se usa el algoritmo shift-and-add:
 *   - normaliza el signo de ambos operandos,
 *   - recorre los bits del multiplicador,
 *   - suma el multiplicando cuando el bit menos significativo es 1,
 *   - desplaza multiplicando a la izquierda y multiplicador a la derecha.
 *
 * El ciclo termina cuando el multiplicador llega a cero, por lo que
 * requiere como maximo 32 iteraciones para operandos de 32 bits.
 * El resultado conserva la semantica modular de 32 bits del hardware.
 *
 * Entradas:
 *   reg_izq -> multiplicando temporal
 *   reg_der -> multiplicador temporal
 *
 * Ambos registros pueden modificarse porque contienen temporales del
 * generador, no las variables originales almacenadas en memoria.
 *
 * Devuelve un nuevo registro temporal con el producto o -1 en error.
 */
static int generar_multiplicacion(Codegen *cg,
                                  NodoAST *nodo,
                                  int reg_izq,
                                  int reg_der)
{
    int reg_resultado;
    int reg_signos;
    int reg_uno;
    int reg_bit;
    char etq_izq_positivo[64];
    char etq_der_positivo[64];
    char etq_bucle[64];
    char etq_sin_suma[64];
    char etq_fin_bucle[64];
    char etq_fin_signo[64];

    reg_resultado = reservar_temp(cg, nodo);
    reg_signos = reservar_temp(cg, nodo);
    reg_uno = reservar_temp(cg, nodo);
    reg_bit = reservar_temp(cg, nodo);

    if (reg_resultado < 0 || reg_signos < 0 ||
        reg_uno < 0 || reg_bit < 0) {
        liberar_temp(cg, reg_resultado);
        liberar_temp(cg, reg_signos);
        liberar_temp(cg, reg_uno);
        liberar_temp(cg, reg_bit);
        return -1;
    }

    nueva_etiqueta(cg, "mul_izq_positivo",
                   etq_izq_positivo, sizeof(etq_izq_positivo));
    nueva_etiqueta(cg, "mul_der_positivo",
                   etq_der_positivo, sizeof(etq_der_positivo));
    nueva_etiqueta(cg, "mul_bucle",
                   etq_bucle, sizeof(etq_bucle));
    nueva_etiqueta(cg, "mul_sin_suma",
                   etq_sin_suma, sizeof(etq_sin_suma));
    nueva_etiqueta(cg, "mul_fin_bucle",
                   etq_fin_bucle, sizeof(etq_fin_bucle));
    nueva_etiqueta(cg, "mul_fin_signo",
                   etq_fin_signo, sizeof(etq_fin_signo));

    asm_emitir_comentario(&cg->emisor,
                          "multiplicacion por software: shift-and-add");

    if (!emitir_constante(cg, reg_resultado, 0) ||
        !emitir_constante(cg, reg_signos, 0) ||
        !emitir_constante(cg, reg_uno, 1))
        goto error;

    /*
     * Convertir el operando izquierdo a magnitud positiva.
     * reg_signos cuenta cuantos operandos eran negativos: 0, 1 o 2.
     */
    if (!emitir_instruccion(cg, "BRU",
                            "BGE x%d, x0, %s",
                            reg_izq, etq_izq_positivo))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_izq, reg_izq) ||
        !emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x%d, 1",
                            reg_signos, reg_signos) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_izq_positivo))
        goto error;

    /* Convertir el operando derecho a magnitud positiva. */
    if (!emitir_instruccion(cg, "BRU",
                            "BGE x%d, x0, %s",
                            reg_der, etq_der_positivo))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_der, reg_der) ||
        !emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x%d, 1",
                            reg_signos, reg_signos) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_der_positivo))
        goto error;

    /*
     * Bucle principal.
     *
     * while (reg_der != 0) {
     *     if (reg_der & 1)
     *         resultado += reg_izq;
     *     reg_izq <<= 1;
     *     reg_der >>= 1;
     * }
     */
    if (!asm_emitir_etiqueta(&cg->emisor, etq_bucle))
        goto error;

    if (!emitir_instruccion(cg, "BRU",
                            "BEQ x%d, x0, %s",
                            reg_der, etq_fin_bucle))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "AND x%d, x%d, x%d",
                            reg_bit, reg_der, reg_uno))
        goto error;

    if (!emitir_instruccion(cg, "BRU",
                            "BEQ x%d, x0, %s",
                            reg_bit, etq_sin_suma))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "ADD x%d, x%d, x%d",
                            reg_resultado, reg_resultado, reg_izq))
        goto error;

    if (!asm_emitir_etiqueta(&cg->emisor, etq_sin_suma))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SLLI x%d, x%d, 1",
                            reg_izq, reg_izq) ||
        !emitir_instruccion(cg, "ALU",
                            "SRLI x%d, x%d, 1",
                            reg_der, reg_der) ||
        !emitir_instruccion(cg, "BRU",
                            "JMP %s", etq_bucle) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_fin_bucle))
        goto error;

    /*
     * Solo un operando negativo => producto negativo.
     * Con cero o dos operandos negativos el resultado ya tiene
     * el signo correcto.
     */
    if (!emitir_instruccion(cg, "BRU",
                            "BNE x%d, x%d, %s",
                            reg_signos, reg_uno, etq_fin_signo))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_resultado, reg_resultado) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_fin_signo))
        goto error;

    liberar_temp(cg, reg_izq);
    liberar_temp(cg, reg_der);
    liberar_temp(cg, reg_signos);
    liberar_temp(cg, reg_uno);
    liberar_temp(cg, reg_bit);

    return reg_resultado;

error:
    liberar_temp(cg, reg_izq);
    liberar_temp(cg, reg_der);
    liberar_temp(cg, reg_resultado);
    liberar_temp(cg, reg_signos);
    liberar_temp(cg, reg_uno);
    liberar_temp(cg, reg_bit);
    return -1;
}


/*
 * Division y modulo enteros con signo implementados sin DIV/MOD.
 *
 * El ISA no posee instrucciones nativas de division. Para esta primera
 * version se usa resta repetida sobre las magnitudes de los operandos:
 *
 *     cociente = 0;
 *     mientras (dividendo >= divisor) {
 *         dividendo -= divisor;
 *         cociente++;
 *     }
 *
 * Al terminar, el dividendo contiene el residuo. Luego se restauran
 * los signos con semantica tipo C: el cociente se trunca hacia cero y
 * el residuo conserva el signo del dividendo original.
 *
 * Si el divisor vale cero en tiempo de ejecucion, esta primera version
 * evita un ciclo infinito y produce cociente 0 y residuo igual al
 * dividendo original. El analisis semantico puede rechazar los casos
 * constantes de division entre cero antes de llegar a Fase 4.
 *
 * devolver_resto == 0 -> devuelve cociente (/)
 * devolver_resto != 0 -> devuelve residuo (%)
 */
static int generar_division_modulo(Codegen *cg,
                                    NodoAST *nodo,
                                    int reg_izq,
                                    int reg_der,
                                    int devolver_resto)
{
    int reg_cociente;
    int reg_signos;
    int reg_uno;
    int reg_dividendo_negativo;
    int reg_resultado;
    char etq_izq_positivo[64];
    char etq_der_positivo[64];
    char etq_divisor_valido[64];
    char etq_bucle[64];
    char etq_restar[64];
    char etq_fin_bucle[64];
    char etq_fin_signo_cociente[64];
    char etq_fin_signo_resto[64];

    reg_cociente = reservar_temp(cg, nodo);
    reg_signos = reservar_temp(cg, nodo);
    reg_uno = reservar_temp(cg, nodo);
    reg_dividendo_negativo = reservar_temp(cg, nodo);

    if (reg_cociente < 0 || reg_signos < 0 ||
        reg_uno < 0 || reg_dividendo_negativo < 0) {
        liberar_temp(cg, reg_izq);
        liberar_temp(cg, reg_der);
        liberar_temp(cg, reg_cociente);
        liberar_temp(cg, reg_signos);
        liberar_temp(cg, reg_uno);
        liberar_temp(cg, reg_dividendo_negativo);
        return -1;
    }

    nueva_etiqueta(cg, "div_izq_positivo",
                   etq_izq_positivo, sizeof(etq_izq_positivo));
    nueva_etiqueta(cg, "div_der_positivo",
                   etq_der_positivo, sizeof(etq_der_positivo));
    nueva_etiqueta(cg, "div_divisor_valido",
                   etq_divisor_valido, sizeof(etq_divisor_valido));
    nueva_etiqueta(cg, "div_bucle",
                   etq_bucle, sizeof(etq_bucle));
    nueva_etiqueta(cg, "div_restar",
                   etq_restar, sizeof(etq_restar));
    nueva_etiqueta(cg, "div_fin_bucle",
                   etq_fin_bucle, sizeof(etq_fin_bucle));
    nueva_etiqueta(cg, "div_fin_signo_cociente",
                   etq_fin_signo_cociente,
                   sizeof(etq_fin_signo_cociente));
    nueva_etiqueta(cg, "div_fin_signo_resto",
                   etq_fin_signo_resto,
                   sizeof(etq_fin_signo_resto));

    asm_emitir_comentario(&cg->emisor,
                          devolver_resto
                              ? "modulo por software: resta repetida"
                              : "division por software: resta repetida");

    if (!emitir_constante(cg, reg_cociente, 0) ||
        !emitir_constante(cg, reg_signos, 0) ||
        !emitir_constante(cg, reg_uno, 1) ||
        !emitir_constante(cg, reg_dividendo_negativo, 0))
        goto error;

    /* Normalizar el dividendo y recordar su signo para el residuo. */
    if (!emitir_instruccion(cg, "BRU",
                            "BGE x%d, x0, %s",
                            reg_izq, etq_izq_positivo))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_izq, reg_izq) ||
        !emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x%d, 1",
                            reg_signos, reg_signos) ||
        !emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x0, 1",
                            reg_dividendo_negativo) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_izq_positivo))
        goto error;

    /* Normalizar el divisor y acumular la paridad de signos. */
    if (!emitir_instruccion(cg, "BRU",
                            "BGE x%d, x0, %s",
                            reg_der, etq_der_positivo))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_der, reg_der) ||
        !emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x%d, 1",
                            reg_signos, reg_signos) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_der_positivo))
        goto error;

    /* Evitar division entre cero en tiempo de ejecucion. */
    if (!emitir_instruccion(cg, "BRU",
                            "BNE x%d, x0, %s",
                            reg_der, etq_divisor_valido))
        goto error;

    asm_emitir_comentario(&cg->emisor,
                          "division entre cero: cociente=0, resto=dividendo");
    if (!emitir_instruccion(cg, "BRU", "JMP %s", etq_fin_bucle) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_divisor_valido))
        goto error;

    /*
     * while (dividendo >= divisor) {
     *     dividendo -= divisor;
     *     cociente += 1;
     * }
     */
    if (!asm_emitir_etiqueta(&cg->emisor, etq_bucle))
        goto error;

    if (!emitir_instruccion(cg, "BRU",
                            "BGE x%d, x%d, %s",
                            reg_izq, reg_der, etq_restar) ||
        !emitir_instruccion(cg, "BRU",
                            "JMP %s", etq_fin_bucle) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_restar))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x%d, x%d",
                            reg_izq, reg_izq, reg_der) ||
        !emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x%d, 1",
                            reg_cociente, reg_cociente) ||
        !emitir_instruccion(cg, "BRU",
                            "JMP %s", etq_bucle) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_fin_bucle))
        goto error;

    /* Un solo operando negativo => cociente negativo. */
    if (!emitir_instruccion(cg, "BRU",
                            "BNE x%d, x%d, %s",
                            reg_signos, reg_uno,
                            etq_fin_signo_cociente))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_cociente, reg_cociente) ||
        !asm_emitir_etiqueta(&cg->emisor,
                             etq_fin_signo_cociente))
        goto error;

    /* El residuo conserva el signo del dividendo original. */
    if (!emitir_instruccion(cg, "BRU",
                            "BNE x%d, x%d, %s",
                            reg_dividendo_negativo, reg_uno,
                            etq_fin_signo_resto))
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "SUB x%d, x0, x%d",
                            reg_izq, reg_izq) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_fin_signo_resto))
        goto error;

    if (devolver_resto) {
        reg_resultado = reg_izq;
        liberar_temp(cg, reg_cociente);
    } else {
        reg_resultado = reg_cociente;
        liberar_temp(cg, reg_izq);
    }

    liberar_temp(cg, reg_der);
    liberar_temp(cg, reg_signos);
    liberar_temp(cg, reg_uno);
    liberar_temp(cg, reg_dividendo_negativo);

    return reg_resultado;

error:
    liberar_temp(cg, reg_izq);
    liberar_temp(cg, reg_der);
    liberar_temp(cg, reg_cociente);
    liberar_temp(cg, reg_signos);
    liberar_temp(cg, reg_uno);
    liberar_temp(cg, reg_dividendo_negativo);
    return -1;
}


/*
 * Potencia entera implementada exclusivamente con instrucciones del ISA.
 *
 * No existe POW en el procesador. Para exponentes enteros no negativos
 * se usa multiplicacion repetida:
 *
 *     resultado = 1;
 *     mientras (exponente > 0) {
 *         resultado = resultado * base;
 *         exponente--;
 *     }
 *
 * Cada producto se baja, a su vez, mediante generar_multiplicacion(),
 * que implementa shift-and-add con ADD/AND/SLLI/SRLI y saltos BRU.
 *
 * Un exponente igual a cero produce 1. Los exponentes negativos no son
 * representables de forma general con el tipo entero del lenguaje; el
 * analisis semantico debe rechazarlos cuando pueda determinarlos. Como
 * proteccion de ejecucion, si un exponente negativo llega al backend se
 * produce 0 y se evita un ciclo infinito.
 *
 * generar_potencia consume reg_base y reg_exponente y devuelve un nuevo
 * temporal con el resultado, o -1 si ocurre un error de generacion.
 */
static int generar_potencia(Codegen *cg,
                            NodoAST *nodo,
                            int reg_base,
                            int reg_exponente)
{
    int reg_resultado;
    int reg_copia_resultado;
    int reg_copia_base;
    int reg_producto;
    char etq_exponente_valido[64];
    char etq_bucle[64];
    char etq_fin[64];

    reg_resultado = reservar_temp(cg, nodo);
    if (reg_resultado < 0) {
        liberar_temp(cg, reg_base);
        liberar_temp(cg, reg_exponente);
        return -1;
    }

    nueva_etiqueta(cg, "pow_exponente_valido",
                   etq_exponente_valido, sizeof(etq_exponente_valido));
    nueva_etiqueta(cg, "pow_bucle",
                   etq_bucle, sizeof(etq_bucle));
    nueva_etiqueta(cg, "pow_fin",
                   etq_fin, sizeof(etq_fin));

    asm_emitir_comentario(&cg->emisor,
                          "potencia por software: multiplicacion repetida");

    /* resultado = 1, de modo que base ** 0 == 1. */
    if (!emitir_constante(cg, reg_resultado, 1))
        goto error;

    /*
     * Exponentes negativos no tienen una representacion entera general.
     * El semantico deberia rechazarlos; esta rama evita un ciclo infinito
     * si el valor negativo solo se conoce en tiempo de ejecucion.
     */
    if (!emitir_instruccion(cg, "BRU",
                            "BGE x%d, x0, %s",
                            reg_exponente, etq_exponente_valido))
        goto error;

    asm_emitir_comentario(&cg->emisor,
                          "exponente negativo no soportado: resultado=0");
    if (!emitir_constante(cg, reg_resultado, 0) ||
        !emitir_instruccion(cg, "BRU", "JMP %s", etq_fin) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_exponente_valido))
        goto error;

    if (!asm_emitir_etiqueta(&cg->emisor, etq_bucle))
        goto error;

    /* while (exponente != 0) */
    if (!emitir_instruccion(cg, "BRU",
                            "BEQ x%d, x0, %s",
                            reg_exponente, etq_fin))
        goto error;

    /*
     * generar_multiplicacion modifica sus dos operandos al normalizar
     * signos y desplazar bits. Por eso copiamos resultado y base antes
     * de emitir la rutina; los registros persistentes se conservan para
     * la siguiente iteracion de la potencia.
     */
    reg_copia_resultado = reservar_temp(cg, nodo);
    reg_copia_base = reservar_temp(cg, nodo);
    if (reg_copia_resultado < 0 || reg_copia_base < 0) {
        liberar_temp(cg, reg_copia_resultado);
        liberar_temp(cg, reg_copia_base);
        goto error;
    }

    if (!emitir_instruccion(cg, "ALU",
                            "ADD x%d, x0, x%d",
                            reg_copia_resultado, reg_resultado) ||
        !emitir_instruccion(cg, "ALU",
                            "ADD x%d, x0, x%d",
                            reg_copia_base, reg_base)) {
        liberar_temp(cg, reg_copia_resultado);
        liberar_temp(cg, reg_copia_base);
        goto error;
    }

    reg_producto = generar_multiplicacion(cg, nodo,
                                          reg_copia_resultado,
                                          reg_copia_base);
    if (reg_producto < 0)
        goto error;

    if (!emitir_instruccion(cg, "ALU",
                            "ADD x%d, x0, x%d",
                            reg_resultado, reg_producto)) {
        liberar_temp(cg, reg_producto);
        goto error;
    }
    liberar_temp(cg, reg_producto);

    if (!emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x%d, -1",
                            reg_exponente, reg_exponente) ||
        !emitir_instruccion(cg, "BRU", "JMP %s", etq_bucle) ||
        !asm_emitir_etiqueta(&cg->emisor, etq_fin))
        goto error;

    liberar_temp(cg, reg_base);
    liberar_temp(cg, reg_exponente);
    return reg_resultado;

error:
    liberar_temp(cg, reg_base);
    liberar_temp(cg, reg_exponente);
    liberar_temp(cg, reg_resultado);
    return -1;
}


/*
 * Calcula la direccion byte efectiva de un acceso a lista o matriz y la
 * deja en x13 (REG_DIRECCION). Los enteros ocupan 4 bytes.
 *
 * lista[i]:      base + i * 4
 * matriz[i][j]: base + ((i * columnas) + j) * 4
 *
 * La matriz se almacena en orden row-major. En esta primera version no se
 * generan comprobaciones dinamicas de limites; esa validacion se deja al
 * analisis semantico cuando los indices sean conocidos en compilacion.
 */
static int generar_direccion_acceso(Codegen *cg, NodoAST *nodo)
{
    NodoAST *variable;
    CodegenSimbolo simbolo;
    int reg_indice;

    if (nodo == NULL ||
        (nodo->tipo != AST_ACCESO_LISTA &&
         nodo->tipo != AST_ACCESO_MATRIZ)) {
        error_codegen(cg, nodo, "acceso a arreglo invalido");
        return 0;
    }

    variable = hijo_n(nodo, 0);
    if (variable == NULL || variable->tipo != AST_VARIABLE ||
        variable->valor == NULL) {
        error_codegen(cg, nodo, "acceso sin identificador valido");
        return 0;
    }

    if (!resolver(cg, variable, variable->valor, &simbolo))
        return 0;

    if (nodo->tipo == AST_ACCESO_LISTA) {
        if (simbolo.forma != CODEGEN_FORMA_LISTA ||
            simbolo.cantidad_dimensiones < 1) {
            error_codegen(cg, nodo, "'%s' no es una lista", variable->valor);
            return 0;
        }

        reg_indice = generar_expresion(cg, hijo_n(nodo, 1));
        if (reg_indice < 0)
            return 0;

        /* Cada entero ocupa 4 bytes: indice * 4 = indice << 2. */
        if (!emitir_instruccion(cg, "ALU",
                                "SLLI x%d, x%d, 2",
                                reg_indice, reg_indice) ||
            !emitir_base_simbolo(cg, variable, &simbolo, REG_DIRECCION) ||
            !emitir_instruccion(cg, "ALU",
                                "ADD x%d, x%d, x%d",
                                REG_DIRECCION,
                                REG_DIRECCION,
                                reg_indice)) {
            liberar_temp(cg, reg_indice);
            return 0;
        }

        liberar_temp(cg, reg_indice);
        return 1;
    }

    /* Matriz bidimensional en orden row-major. */
    if (simbolo.forma != CODEGEN_FORMA_MATRIZ ||
        simbolo.cantidad_dimensiones < 2 ||
        simbolo.dimensiones[1] == 0) {
        error_codegen(cg, nodo,
                      "'%s' no tiene dimensiones de matriz resueltas",
                      variable->valor);
        return 0;
    }

    {
        int reg_fila;
        int reg_columna;
        int reg_columnas;
        int reg_lineal;

        reg_fila = generar_expresion(cg, hijo_n(nodo, 1));
        if (reg_fila < 0)
            return 0;

        reg_columna = generar_expresion(cg, hijo_n(nodo, 2));
        if (reg_columna < 0) {
            liberar_temp(cg, reg_fila);
            return 0;
        }

        reg_columnas = reservar_temp(cg, nodo);
        if (reg_columnas < 0) {
            liberar_temp(cg, reg_fila);
            liberar_temp(cg, reg_columna);
            return 0;
        }

        if (!emitir_constante(cg, reg_columnas,
                              (int32_t)simbolo.dimensiones[1])) {
            liberar_temp(cg, reg_fila);
            liberar_temp(cg, reg_columna);
            liberar_temp(cg, reg_columnas);
            return 0;
        }

        /* i * columnas, reutilizando la multiplicacion software existente. */
        reg_lineal = generar_multiplicacion(cg, nodo,
                                             reg_fila,
                                             reg_columnas);
        if (reg_lineal < 0) {
            liberar_temp(cg, reg_columna);
            return 0;
        }

        if (!emitir_instruccion(cg, "ALU",
                                "ADD x%d, x%d, x%d",
                                reg_lineal, reg_lineal, reg_columna)) {
            liberar_temp(cg, reg_lineal);
            liberar_temp(cg, reg_columna);
            return 0;
        }
        liberar_temp(cg, reg_columna);

        /* ((fila * columnas) + columna) * 4 bytes. */
        if (!emitir_instruccion(cg, "ALU",
                                "SLLI x%d, x%d, 2",
                                reg_lineal, reg_lineal) ||
            !emitir_base_simbolo(cg, variable, &simbolo, REG_DIRECCION) ||
            !emitir_instruccion(cg, "ALU",
                                "ADD x%d, x%d, x%d",
                                REG_DIRECCION,
                                REG_DIRECCION,
                                reg_lineal)) {
            liberar_temp(cg, reg_lineal);
            return 0;
        }

        liberar_temp(cg, reg_lineal);
        return 1;
    }
}

static int cargar_acceso(Codegen *cg, NodoAST *nodo)
{
    int reg;

    if (!generar_direccion_acceso(cg, nodo))
        return -1;

    reg = reservar_temp(cg, nodo);
    if (reg < 0)
        return -1;

    if (!emitir_instruccion(cg, "LSU",
                            "LOAD x%d, 0(x%d)",
                            reg, REG_DIRECCION)) {
        liberar_temp(cg, reg);
        return -1;
    }

    return reg;
}

static int guardar_acceso(Codegen *cg, NodoAST *nodo, int reg_valor)
{
    if (!generar_direccion_acceso(cg, nodo))
        return 0;

    return emitir_instruccion(cg, "LSU",
                              "STORE x%d, 0(x%d)",
                              reg_valor, REG_DIRECCION);
}

/* Guarda un elemento de un inicializador en una direccion ya conocida. */
static int sumar_offset_direccion(Codegen *cg, uint32_t offset)
{
    if (offset == 0)
        return 1;

    if (offset <= 2047u)
        return emitir_instruccion(cg, "ALU",
                                  "ADDI x%d, x%d, %u",
                                  REG_DIRECCION, REG_DIRECCION, offset);

    if (!emitir_constante(cg, REG_AUXILIAR, (int32_t)offset))
        return 0;

    return emitir_instruccion(cg, "ALU",
                              "ADD x%d, x%d, x%d",
                              REG_DIRECCION, REG_DIRECCION, REG_AUXILIAR);
}

/* Guarda un elemento de inicializador en base(simbolo)+offset. */
static int guardar_elemento_literal(Codegen *cg,
                                    NodoAST *variable,
                                    const CodegenSimbolo *simbolo,
                                    uint32_t offset,
                                    NodoAST *expresion)
{
    int reg_valor = generar_expresion(cg, expresion);

    if (reg_valor < 0)
        return 0;

    if (!emitir_base_simbolo(cg, variable, simbolo, REG_DIRECCION) ||
        !sumar_offset_direccion(cg, offset) ||
        !emitir_instruccion(cg, "LSU",
                            "STORE x%d, 0(x%d)",
                            reg_valor, REG_DIRECCION)) {
        liberar_temp(cg, reg_valor);
        return 0;
    }

    liberar_temp(cg, reg_valor);
    return 1;
}



/* Empuja una palabra de 32 bits en la pila. */
static int emitir_push_reg(Codegen *cg, int reg)
{
    return emitir_ajuste_sp(cg, -4) &&
           emitir_instruccion(cg, "LSU",
                              "STORE x%d, 0(x%d)", reg, REG_SP);
}

/*
 * Comprueba en tiempo de ejecucion que una reserva completa no cruce 0x8000.
 * Usa x3 y x13 como registros reservados del backend.
 */
static int emitir_verificacion_pila(Codegen *cg,
                                    NodoAST *nodo,
                                    uint32_t bytes_reserva)
{
    char etiqueta_ok[64];

    if (bytes_reserva == 0)
        return 1;

    nueva_etiqueta(cg, "pila_ok", etiqueta_ok, sizeof(etiqueta_ok));
    cg->usa_error_pila = 1;

    if (bytes_reserva <= 2047u) {
        if (!emitir_instruccion(cg, "ALU",
                                "ADDI x%d, x%d, -%u",
                                REG_DIRECCION, REG_SP, bytes_reserva))
            return 0;
    } else {
        if (bytes_reserva > INT32_MAX ||
            !emitir_constante(cg, REG_AUXILIAR, (int32_t)bytes_reserva) ||
            !emitir_instruccion(cg, "ALU",
                                "SUB x%d, x%d, x%d",
                                REG_DIRECCION, REG_SP, REG_AUXILIAR))
            return 0;
    }

    if (!emitir_constante(cg, REG_AUXILIAR, (int32_t)LIMITE_PILA) ||
        !emitir_instruccion(cg, "BRU",
                            "BGE x%d, x%d, %s",
                            REG_DIRECCION, REG_AUXILIAR, etiqueta_ok) ||
        !emitir_instruccion(cg, "BRU", "JMP error_pila") ||
        !asm_emitir_etiqueta(&cg->emisor, etiqueta_ok)) {
        error_codegen(cg, nodo, "no se pudo emitir verificacion de pila");
        return 0;
    }

    return 1;
}

/*
 * Convencion final de llamadas acordada con Fase 3:
 *   x14 = SP
 *   x15 = FP
 *   x2  = valor de retorno
 *   x3  = auxiliar reservado del backend
 *   x13 = direccion/scratch reservado del backend
 *
 * Marco del callee:
 *   0(FP)  -> FP anterior
 *   4(FP)  -> direccion de retorno
 *   8(FP)  -> parametro 1
 *   12(FP) -> parametro 2
 *   ...
 *   -4(FP), -8(FP), ... -> variables locales
 *
 * El caller guarda x4-x12, coloca argumentos de derecha a izquierda,
 * coloca la direccion de retorno y salta. El callee restaura la cabecera;
 * el caller elimina parametros y recupera x4-x12.
 */
static int generar_llamada(Codegen *cg, NodoAST *nodo, int necesita_resultado)
{
    NodoAST *funcion_ast;
    NodoAST *parametros_ast;
    NodoAST *p;
    NodoAST *a;
    NodoAST *params[MAX_PARAMETROS_FUNCION];
    NodoAST *args[MAX_PARAMETROS_FUNCION];
    CodegenSimbolo simbolo_funcion;
    unsigned cantidad_parametros = 0;
    unsigned cantidad_argumentos = 0;
    unsigned i;
    uint64_t reserva64;
    uint32_t bytes_reserva;
    int reg;
    int reg_resultado = -1;
    int delta_retorno;
    char etiqueta_funcion[160];

    if (nodo == NULL || nodo->valor == NULL) {
        error_codegen(cg, nodo, "llamada sin nombre de funcion");
        return -1;
    }

    funcion_ast = buscar_funcion_ast(cg, nodo->valor);
    if (funcion_ast == NULL) {
        error_codegen(cg, nodo, "funcion no encontrada: %s", nodo->valor);
        return -1;
    }

    if (!resolver(cg, nodo, nodo->valor, &simbolo_funcion) ||
        simbolo_funcion.categoria != CODEGEN_CAT_FUNCION) {
        error_codegen(cg, nodo,
                      "no se encontro informacion de memoria para la funcion '%s'",
                      nodo->valor);
        return -1;
    }

    if (necesita_resultado && funcion_ast->valor != NULL &&
        strcmp(funcion_ast->valor, "vacio") == 0) {
        error_codegen(cg, nodo,
                      "la funcion vacia '%s' no produce un valor",
                      nodo->valor);
        return -1;
    }

    parametros_ast = hijo_n(funcion_ast, 1);
    for (p = parametros_ast != NULL ? parametros_ast->hijo : NULL;
         p != NULL; p = p->siguiente) {
        if (cantidad_parametros >= MAX_PARAMETROS_FUNCION) {
            error_codegen(cg, nodo, "demasiados parametros en '%s'", nodo->valor);
            return -1;
        }
        params[cantidad_parametros++] = p;
    }

    for (a = nodo->hijo; a != NULL; a = a->siguiente) {
        if (cantidad_argumentos >= MAX_PARAMETROS_FUNCION) {
            error_codegen(cg, nodo, "demasiados argumentos en '%s'", nodo->valor);
            return -1;
        }
        args[cantidad_argumentos++] = a;
    }

    if (cantidad_parametros != cantidad_argumentos) {
        error_codegen(cg, nodo,
                      "la funcion '%s' espera %u argumentos y recibio %u",
                      nodo->valor, cantidad_parametros, cantidad_argumentos);
        return -1;
    }

    if (simbolo_funcion.tamano_parametros != cantidad_parametros * 4u) {
        error_codegen(cg, nodo,
                      "tamano de parametros inconsistente para '%s'",
                      nodo->valor);
        return -1;
    }

    reserva64 = (uint64_t)BYTES_TEMPORALES_GUARDADOS +
                simbolo_funcion.tamano_parametros +
                TAMANO_CABECERA +
                simbolo_funcion.tamano_marco;
    if (reserva64 > UINT32_MAX) {
        error_codegen(cg, nodo, "reserva de pila fuera de rango en '%s'",
                      nodo->valor);
        return -1;
    }
    bytes_reserva = (uint32_t)reserva64;

    if (!pila_hay_espacio(PILA_INICIAL_DEFECTO,
                          BYTES_TEMPORALES_GUARDADOS +
                          simbolo_funcion.tamano_parametros +
                          simbolo_funcion.tamano_marco,
                          TAMANO_CABECERA)) {
        error_codegen(cg, nodo,
                      "el marco de '%s' no cabe ni con la pila vacia",
                      nodo->valor);
        return -1;
    }

    asm_emitir_comentario(&cg->emisor,
                          "llamada a %s: caller-save, parametros, retorno",
                          nodo->valor);

    if (!emitir_verificacion_pila(cg, nodo, bytes_reserva))
        return -1;

    /* Preservar temporales del caller. */
    for (reg = REG_TEMP_PRIMERO; reg <= REG_TEMP_ULTIMO; reg++)
        if (!emitir_push_reg(cg, reg))
            return -1;

    /*
     * Colocar argumentos de derecha a izquierda. Asi, despues del prologo
     * del callee, el primer parametro queda exactamente en 8(FP).
     */
    for (i = cantidad_parametros; i > 0; i--) {
        NodoAST *parametro = params[i - 1];
        NodoAST *argumento = args[i - 1];
        NodoAST *id_parametro = hijo_n(parametro, 0);
        CodegenSimbolo simbolo_parametro;

        if (id_parametro == NULL || id_parametro->tipo != AST_VARIABLE ||
            id_parametro->valor == NULL ||
            !resolver(cg, id_parametro, id_parametro->valor,
                      &simbolo_parametro)) {
            error_codegen(cg, parametro,
                          "parametro sin informacion semantica valida");
            return -1;
        }

        if (simbolo_parametro.forma == CODEGEN_FORMA_ESCALAR) {
            reg = generar_expresion(cg, argumento);
            if (reg < 0)
                return -1;
            if (!emitir_push_reg(cg, reg)) {
                liberar_temp(cg, reg);
                return -1;
            }
            liberar_temp(cg, reg);
        } else {
            CodegenSimbolo simbolo_argumento;

            if (argumento->tipo != AST_VARIABLE || argumento->valor == NULL ||
                !resolver(cg, argumento, argumento->valor,
                          &simbolo_argumento)) {
                error_codegen(cg, argumento,
                              "un parametro lista/matriz requiere un arreglo como argumento");
                return -1;
            }

            if (simbolo_argumento.forma != simbolo_parametro.forma) {
                error_codegen(cg, argumento,
                              "forma de arreglo incompatible al llamar '%s'",
                              nodo->valor);
                return -1;
            }

            if (!emitir_base_simbolo(cg, argumento, &simbolo_argumento,
                                     REG_DIRECCION) ||
                !emitir_push_reg(cg, REG_DIRECCION))
                return -1;
        }
    }

    /* Reservar el slot de retorno y calcular su PC real. */
    if (!emitir_ajuste_sp(cg, -4))
        return -1;

    delta_retorno = (int)(3u * (1u + cg->config.nops_dependencia) * 16u);
    if (delta_retorno > 2047) {
        error_codegen(cg, nodo,
                      "la separacion de NOP hace que el retorno no quepa en ADDI");
        return -1;
    }

    if (!emitir_instruccion(cg, "ALU",
                            "ADDI x%d, x1, %d",
                            REG_AUXILIAR, delta_retorno) ||
        !emitir_instruccion(cg, "LSU",
                            "STORE x%d, 0(x%d)", REG_AUXILIAR, REG_SP))
        return -1;

    snprintf(etiqueta_funcion, sizeof(etiqueta_funcion),
             "func_%s", nodo->valor);
    if (!emitir_instruccion(cg, "BRU", "JMP %s", etiqueta_funcion))
        return -1;

    /* El callee retorna con SP apuntando al primer parametro. */
    if (!emitir_ajuste_sp(cg, (long)simbolo_funcion.tamano_parametros))
        return -1;

    /* Recuperar temporales del caller en orden inverso. */
    for (reg = REG_TEMP_ULTIMO; reg >= REG_TEMP_PRIMERO; reg--) {
        if (!emitir_instruccion(cg, "LSU",
                                "LOAD x%d, 0(x%d)", reg, REG_SP) ||
            !emitir_ajuste_sp(cg, 4))
            return -1;
    }

    if (!necesita_resultado)
        return 0;

    reg_resultado = reservar_temp(cg, nodo);
    if (reg_resultado < 0)
        return -1;

    if (!emitir_instruccion(cg, "ALU",
                            "ADD x%d, x0, x%d",
                            reg_resultado, REG_RETORNO_VALOR)) {
        liberar_temp(cg, reg_resultado);
        return -1;
    }

    return reg_resultado;
}

static int generar_expresion(Codegen *cg, NodoAST *nodo)
{
    int izq, der, reg;
    long valor;
    char *fin;

    if (nodo == NULL) {
        error_codegen(cg, nodo, "expresion vacia");
        return -1;
    }

    switch (nodo->tipo) {
    case AST_NUMERO:
        valor = strtol(nodo->valor, &fin, 10);
        if (nodo->valor == NULL || *fin != '\0' ||
            valor < INT32_MIN || valor > UINT32_MAX) {
            error_codegen(cg, nodo, "numero invalido");
            return -1;
        }

        reg = reservar_temp(cg, nodo);
        if (reg < 0)
            return -1;

        if (!emitir_constante(cg, reg, (int32_t)(uint32_t)valor)) {
            liberar_temp(cg, reg);
            return -1;
        }
        return reg;

    case AST_BOOLEANO:
        reg = reservar_temp(cg, nodo);
        if (reg < 0)
            return -1;

        if (!emitir_constante(cg, reg,
                              strcmp(nodo->valor, "verdadero") == 0 ? 1 : 0)) {
            liberar_temp(cg, reg);
            return -1;
        }
        return reg;

    case AST_VARIABLE:
        return cargar_variable(cg, nodo, nodo->valor);

    case AST_ACCESO_LISTA:
    case AST_ACCESO_MATRIZ:
        return cargar_acceso(cg, nodo);

    case AST_LLAMADA:
        return generar_llamada(cg, nodo, 1);

    case AST_OPERACION:
        if (strcmp(nodo->valor, "unario -") == 0) {
            izq = generar_expresion(cg, hijo_n(nodo, 0));
            if (izq < 0)
                return -1;

            if (!emitir_instruccion(cg, "ALU",
                                    "SUB x%d, x0, x%d", izq, izq)) {
                liberar_temp(cg, izq);
                return -1;
            }
            return izq;
        }

        /* Comparaciones y lógica producen un booleano normalizado 0/1. */
        if (es_comparador(nodo->valor) ||
            strcmp(nodo->valor, "AND") == 0 ||
            strcmp(nodo->valor, "OR") == 0)
            return generar_booleano_desde_condicion(cg, nodo);

        izq = generar_expresion(cg, hijo_n(nodo, 0));
        if (izq < 0)
            return -1;

        der = generar_expresion(cg, hijo_n(nodo, 1));
        if (der < 0) {
            liberar_temp(cg, izq);
            return -1;
        }

        if (strcmp(nodo->valor, "+") == 0)
            reg = emitir_instruccion(cg, "ALU",
                                     "ADD x%d, x%d, x%d", izq, izq, der);
        else if (strcmp(nodo->valor, "-") == 0)
            reg = emitir_instruccion(cg, "ALU",
                                     "SUB x%d, x%d, x%d", izq, izq, der);
        else if (strcmp(nodo->valor, "*") == 0) {
            /*
             * El ISA no tiene MUL. No se inventa una instruccion nueva:
             * se baja la operacion a una secuencia de ADD/AND/shifts/BRU.
             * generar_multiplicacion libera izq y der y devuelve el
             * registro que contiene el producto.
             */
            return generar_multiplicacion(cg, nodo, izq, der);
        } else if (strcmp(nodo->valor, "/") == 0) {
            /* El ISA tampoco tiene DIV: division entera por software. */
            return generar_division_modulo(cg, nodo, izq, der, 0);
        } else if (strcmp(nodo->valor, "%") == 0) {
            /* El residuo sale del mismo algoritmo de division. */
            return generar_division_modulo(cg, nodo, izq, der, 1);
        } else if (strcmp(nodo->valor, "**") == 0) {
            /* El ISA no tiene POW: potencia entera por software. */
            return generar_potencia(cg, nodo, izq, der);
        } else {
            error_codegen(cg, nodo,
                          "operador '%s' aun no implementado en Fase 4",
                          nodo->valor);
            reg = 0;
        }

        liberar_temp(cg, der);

        if (!reg) {
            liberar_temp(cg, izq);
            return -1;
        }

        return izq;

    default:
        error_codegen(cg, nodo,
                      "tipo de expresion %d aun no soportado",
                      (int)nodo->tipo);
        return -1;
    }
}

static void generar_nodo(Codegen *cg, NodoAST *nodo);

static void generar_lista_hermanos(Codegen *cg, NodoAST *nodo)
{
    while (nodo != NULL) {
        generar_nodo(cg, nodo);
        nodo = nodo->siguiente;
    }
}

static void generar_declaracion(Codegen *cg, NodoAST *nodo)
{
    NodoAST *variable = hijo_n(nodo, 0);
    NodoAST *inicial = hijo_n(nodo, 1);
    int reg;

    if (variable == NULL || variable->tipo != AST_VARIABLE) {
        error_codegen(cg, nodo, "declaracion escalar sin variable valida");
        return;
    }

    asm_emitir_comentario(&cg->emisor,
                          "declaracion %s", variable->valor);

    /* Si no tiene inicializador, la Fase 4 no escribe memoria. */
    if (inicial == NULL)
        return;

    reg = generar_expresion(cg, inicial);
    if (reg < 0)
        return;

    guardar_variable(cg, variable, variable->valor, reg);
    liberar_temp(cg, reg);
}

static void generar_asignacion(Codegen *cg, NodoAST *nodo)
{
    NodoAST *destino = hijo_n(nodo, 0);
    NodoAST *expresion = hijo_n(nodo, 1);
    int reg;

    if (destino == NULL || expresion == NULL) {
        error_codegen(cg, nodo, "asignacion incompleta");
        return;
    }

    /*
     * Se calcula primero el valor. Si el destino es lista/matriz, el
     * calculo posterior de su indice puede reutilizar x13 sin perder el
     * resultado, que permanece en un temporal x4-x12.
     */
    reg = generar_expresion(cg, expresion);
    if (reg < 0)
        return;

    if (destino->tipo == AST_VARIABLE) {
        guardar_variable(cg, destino, destino->valor, reg);
    } else if (destino->tipo == AST_ACCESO_LISTA ||
               destino->tipo == AST_ACCESO_MATRIZ) {
        guardar_acceso(cg, destino, reg);
    } else {
        error_codegen(cg, destino,
                      "destino de asignacion no soportado (tipo %d)",
                      (int)destino->tipo);
    }

    liberar_temp(cg, reg);
}

static void generar_declaracion_lista(Codegen *cg, NodoAST *nodo)
{
    NodoAST *variable = hijo_n(nodo, 0);
    NodoAST *inicializador = hijo_n(nodo, 2);
    NodoAST *elemento;
    CodegenSimbolo simbolo;
    uint32_t indice = 0;

    if (variable == NULL || variable->tipo != AST_VARIABLE ||
        variable->valor == NULL) {
        error_codegen(cg, nodo, "declaracion de lista sin nombre valido");
        return;
    }

    if (!resolver(cg, variable, variable->valor, &simbolo))
        return;

    if (simbolo.forma != CODEGEN_FORMA_LISTA ||
        simbolo.cantidad_dimensiones < 1) {
        error_codegen(cg, nodo,
                      "el simbolo '%s' no esta descrito como lista",
                      variable->valor);
        return;
    }

    asm_emitir_comentario(&cg->emisor,
                          "declaracion lista %s[%u]",
                          variable->valor, simbolo.dimensiones[0]);

    if (inicializador == NULL)
        return;

    if (inicializador->tipo != AST_LISTA) {
        error_codegen(cg, inicializador,
                      "inicializador de lista invalido");
        return;
    }

    for (elemento = inicializador->hijo;
         elemento != NULL;
         elemento = elemento->siguiente, indice++) {
        uint64_t offset;

        if (indice >= simbolo.dimensiones[0]) {
            error_codegen(cg, elemento,
                          "demasiados elementos para la lista '%s'",
                          variable->valor);
            return;
        }

        offset = (uint64_t)indice * TAMANO_ELEMENTO_BYTES;
        if (offset > UINT32_MAX) {
            error_codegen(cg, elemento,
                          "offset fuera de rango al inicializar '%s'",
                          variable->valor);
            return;
        }

        if (!guardar_elemento_literal(cg, variable, &simbolo,
                                      (uint32_t)offset, elemento))
            return;
    }
}

static void generar_declaracion_matriz(Codegen *cg, NodoAST *nodo)
{
    NodoAST *variable = hijo_n(nodo, 0);
    NodoAST *inicializador = hijo_n(nodo, 3);
    NodoAST *fila;
    CodegenSimbolo simbolo;
    uint32_t indice_fila = 0;

    if (variable == NULL || variable->tipo != AST_VARIABLE ||
        variable->valor == NULL) {
        error_codegen(cg, nodo, "declaracion de matriz sin nombre valido");
        return;
    }

    if (!resolver(cg, variable, variable->valor, &simbolo))
        return;

    if (simbolo.forma != CODEGEN_FORMA_MATRIZ ||
        simbolo.cantidad_dimensiones < 2 ||
        simbolo.dimensiones[1] == 0) {
        error_codegen(cg, nodo,
                      "el simbolo '%s' no tiene dimensiones de matriz resueltas",
                      variable->valor);
        return;
    }

    asm_emitir_comentario(&cg->emisor,
                          "declaracion matriz %s[%u][%u] row-major",
                          variable->valor,
                          simbolo.dimensiones[0],
                          simbolo.dimensiones[1]);

    if (inicializador == NULL)
        return;

    if (inicializador->tipo != AST_MATRIZ) {
        error_codegen(cg, inicializador,
                      "inicializador de matriz invalido");
        return;
    }

    for (fila = inicializador->hijo;
         fila != NULL;
         fila = fila->siguiente, indice_fila++) {
        NodoAST *elemento;
        uint32_t indice_columna = 0;

        if (indice_fila >= simbolo.dimensiones[0]) {
            error_codegen(cg, fila,
                          "demasiadas filas para la matriz '%s'",
                          variable->valor);
            return;
        }

        if (fila->tipo != AST_LISTA) {
            error_codegen(cg, fila,
                          "fila invalida en inicializador de matriz");
            return;
        }

        for (elemento = fila->hijo;
             elemento != NULL;
             elemento = elemento->siguiente, indice_columna++) {
            uint64_t indice_lineal;
            uint64_t offset;

            if (indice_columna >= simbolo.dimensiones[1]) {
                error_codegen(cg, elemento,
                              "demasiadas columnas en la fila %u de '%s'",
                              indice_fila, variable->valor);
                return;
            }

            indice_lineal =
                (uint64_t)indice_fila * simbolo.dimensiones[1] +
                indice_columna;
            offset = indice_lineal * TAMANO_ELEMENTO_BYTES;

            if (offset > UINT32_MAX) {
                error_codegen(cg, elemento,
                              "offset fuera de rango al inicializar '%s'",
                              variable->valor);
                return;
            }

            if (!guardar_elemento_literal(cg, variable, &simbolo,
                                          (uint32_t)offset, elemento))
                return;
        }
    }
}


static void generar_nodo(Codegen *cg, NodoAST *nodo);

/*
 * Genera una rama de una cadena si / sinosi / sino.
 * El parser representa "sino" directamente como AST_BLOQUE y cada
 * "sinosi" como otro AST_IF cuyo valor es "sinosi".
 */
static void generar_rama_if(Codegen *cg,
                            NodoAST *nodo,
                            const char *etiqueta_fin)
{
    NodoAST *condicion = hijo_n(nodo, 0);
    NodoAST *bloque = hijo_n(nodo, 1);
    NodoAST *continuacion = hijo_n(nodo, 2);
    char etiqueta_siguiente[64];
    const char *destino_falso = etiqueta_fin;

    if (condicion == NULL || bloque == NULL) {
        error_codegen(cg, nodo, "estructura si incompleta");
        return;
    }

    if (continuacion != NULL) {
        nueva_etiqueta(cg, "siguiente_if", etiqueta_siguiente,
                       sizeof(etiqueta_siguiente));
        destino_falso = etiqueta_siguiente;
    }

    asm_emitir_comentario(&cg->emisor,
                          "condicion %s linea %d",
                          nodo->valor != NULL ? nodo->valor : "si",
                          nodo->linea);

    if (!emitir_salto_condicion(cg, condicion, destino_falso, 0))
        return;

    generar_nodo(cg, bloque);

    if (continuacion == NULL)
        return;

    /* Si esta rama se ejecutó, se salta el resto de la cadena. */
    if (!emitir_instruccion(cg, "BRU", "JMP %s", etiqueta_fin))
        return;

    asm_emitir_etiqueta(&cg->emisor, etiqueta_siguiente);

    if (continuacion->tipo == AST_IF) {
        generar_rama_if(cg, continuacion, etiqueta_fin);
    } else if (continuacion->tipo == AST_BLOQUE) {
        asm_emitir_comentario(&cg->emisor, "rama sino");
        generar_nodo(cg, continuacion);
    } else {
        error_codegen(cg, continuacion,
                      "continuacion de si no reconocida (tipo %d)",
                      (int)continuacion->tipo);
    }
}

static void generar_if(Codegen *cg, NodoAST *nodo)
{
    char etiqueta_fin[64];

    nueva_etiqueta(cg, "fin_if", etiqueta_fin, sizeof(etiqueta_fin));
    generar_rama_if(cg, nodo, etiqueta_fin);
    asm_emitir_etiqueta(&cg->emisor, etiqueta_fin);
}

static void generar_while(Codegen *cg, NodoAST *nodo)
{
    NodoAST *condicion = hijo_n(nodo, 0);
    NodoAST *bloque = hijo_n(nodo, 1);
    char etiqueta_inicio[64];
    char etiqueta_fin[64];

    if (condicion == NULL || bloque == NULL) {
        error_codegen(cg, nodo, "estructura mientras incompleta");
        return;
    }

    nueva_etiqueta(cg, "inicio_while", etiqueta_inicio,
                   sizeof(etiqueta_inicio));
    nueva_etiqueta(cg, "fin_while", etiqueta_fin,
                   sizeof(etiqueta_fin));

    asm_emitir_comentario(&cg->emisor,
                          "inicio mientras linea %d", nodo->linea);
    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_inicio)) {
        cg->errores++;
        return;
    }

    /* Si la condicion es falsa, abandonamos el ciclo. */
    if (!emitir_salto_condicion(cg, condicion, etiqueta_fin, 0))
        return;

    /*
     * Para while, un futuro continue debe volver a evaluar la condicion,
     * por eso tanto continue como el salto de regreso usan etiqueta_inicio.
     */
    if (!apilar_bucle(cg, nodo, etiqueta_fin, etiqueta_inicio))
        return;

    generar_nodo(cg, bloque);
    desapilar_bucle(cg);

    if (!emitir_instruccion(cg, "BRU", "JMP %s", etiqueta_inicio))
        return;

    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_fin))
        cg->errores++;
}

static void generar_for(Codegen *cg, NodoAST *nodo)
{
    NodoAST *inicializacion = hijo_n(nodo, 0);
    NodoAST *condicion = hijo_n(nodo, 1);
    NodoAST *incremento = hijo_n(nodo, 2);
    NodoAST *bloque = hijo_n(nodo, 3);
    char etiqueta_inicio[64];
    char etiqueta_incremento[64];
    char etiqueta_fin[64];

    if (inicializacion == NULL || condicion == NULL ||
        incremento == NULL || bloque == NULL) {
        error_codegen(cg, nodo, "estructura para incompleta");
        return;
    }

    nueva_etiqueta(cg, "inicio_for", etiqueta_inicio,
                   sizeof(etiqueta_inicio));
    nueva_etiqueta(cg, "incremento_for", etiqueta_incremento,
                   sizeof(etiqueta_incremento));
    nueva_etiqueta(cg, "fin_for", etiqueta_fin,
                   sizeof(etiqueta_fin));

    asm_emitir_comentario(&cg->emisor,
                          "inicializacion para linea %d", nodo->linea);
    generar_nodo(cg, inicializacion);

    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_inicio)) {
        cg->errores++;
        return;
    }

    /* Antes de cada iteracion se evalua la condicion. */
    if (!emitir_salto_condicion(cg, condicion, etiqueta_fin, 0))
        return;

    /*
     * break abandona el for. Un futuro continue debe saltar a la
     * actualizacion y no directamente a la condicion.
     */
    if (!apilar_bucle(cg, nodo, etiqueta_fin, etiqueta_incremento))
        return;

    generar_nodo(cg, bloque);
    desapilar_bucle(cg);

    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_incremento)) {
        cg->errores++;
        return;
    }

    asm_emitir_comentario(&cg->emisor, "incremento para");
    generar_nodo(cg, incremento);

    if (!emitir_instruccion(cg, "BRU", "JMP %s", etiqueta_inicio))
        return;

    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_fin))
        cg->errores++;
}

static void generar_break(Codegen *cg, NodoAST *nodo)
{
    const char *destino = etiqueta_break_actual(cg);

    if (destino == NULL) {
        error_codegen(cg, nodo,
                      "'rompa' solo puede utilizarse dentro de un ciclo");
        return;
    }

    asm_emitir_comentario(&cg->emisor, "rompa -> %s", destino);
    emitir_instruccion(cg, "BRU", "JMP %s", destino);
}

static void generar_return(Codegen *cg, NodoAST *nodo)
{
    NodoAST *expresion;
    int reg;

    if (!cg->dentro_funcion) {
        error_codegen(cg, nodo,
                      "'devuelva' solo puede utilizarse dentro de una funcion");
        return;
    }

    expresion = hijo_n(nodo, 0);
    if (expresion == NULL) {
        error_codegen(cg, nodo, "devuelva sin expresion");
        return;
    }

    reg = generar_expresion(cg, expresion);
    if (reg < 0)
        return;

    asm_emitir_comentario(&cg->emisor,
                          "devuelva de %s",
                          cg->funcion_actual != NULL
                              ? cg->funcion_actual : "funcion");

    if (!emitir_instruccion(cg, "ALU",
                            "ADD x%d, x0, x%d",
                            REG_RETORNO_VALOR, reg) ||
        !emitir_instruccion(cg, "BRU",
                            "JMP %s", cg->etiqueta_retorno_funcion)) {
        liberar_temp(cg, reg);
        return;
    }

    liberar_temp(cg, reg);
}

static void generar_funcion(Codegen *cg, NodoAST *nodo)
{
    NodoAST *nombre = hijo_n(nodo, 0);
    NodoAST *bloque = hijo_n(nodo, 2);
    CodegenSimbolo simbolo_funcion;
    const char *funcion_anterior;
    int dentro_anterior;
    char retorno_anterior[96];
    char etiqueta_funcion[160];
    char etiqueta_retorno[96];

    if (nombre == NULL || nombre->tipo != AST_VARIABLE ||
        nombre->valor == NULL || bloque == NULL) {
        error_codegen(cg, nodo, "definicion de funcion incompleta");
        return;
    }

    if (!resolver(cg, nodo, nombre->valor, &simbolo_funcion) ||
        simbolo_funcion.categoria != CODEGEN_CAT_FUNCION) {
        error_codegen(cg, nodo,
                      "no se encontro informacion de marco para '%s'",
                      nombre->valor);
        return;
    }

    if (!pila_hay_espacio(PILA_INICIAL_DEFECTO,
                          simbolo_funcion.tamano_marco,
                          TAMANO_CABECERA)) {
        error_codegen(cg, nodo,
                      "el marco local de '%s' no cabe en la pila",
                      nombre->valor);
        return;
    }

    snprintf(etiqueta_funcion, sizeof(etiqueta_funcion),
             "func_%s", nombre->valor);
    snprintf(etiqueta_retorno, sizeof(etiqueta_retorno),
             "ret_%s", nombre->valor);

    asm_emitir_comentario(&cg->emisor,
                          "funcion %s: x14=SP, x15=FP, retorno en x2",
                          nombre->valor);
    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_funcion)) {
        cg->errores++;
        return;
    }

    /* Valor definido incluso si el semantico deja pasar un camino sin return. */
    if (!emitir_constante(cg, REG_RETORNO_VALOR, 0))
        return;

    /* Prologo: guardar FP anterior, establecer FP y reservar locales. */
    if (!emitir_ajuste_sp(cg, -4) ||
        !emitir_instruccion(cg, "LSU",
                            "STORE x%d, 0(x%d)", REG_FP, REG_SP) ||
        !emitir_instruccion(cg, "ALU",
                            "ADD x%d, x%d, x0", REG_FP, REG_SP) ||
        !emitir_ajuste_sp(cg, -(long)simbolo_funcion.tamano_marco))
        return;

    funcion_anterior = cg->funcion_actual;
    dentro_anterior = cg->dentro_funcion;
    snprintf(retorno_anterior, sizeof(retorno_anterior), "%s",
             cg->etiqueta_retorno_funcion);

    cg->funcion_actual = nombre->valor;
    cg->dentro_funcion = 1;
    snprintf(cg->etiqueta_retorno_funcion,
             sizeof(cg->etiqueta_retorno_funcion),
             "%s", etiqueta_retorno);

    generar_nodo(cg, bloque);

    if (!asm_emitir_etiqueta(&cg->emisor, etiqueta_retorno)) {
        cg->errores++;
    } else {
        /*
         * Epilogo:
         *   SP = FP                  descarta locales
         *   x3 = 0(FP)              FP anterior
         *   x13 = 4(FP)             direccion de retorno
         *   SP = FP + 8             descarta cabecera
         *   FP = x3
         *   PC = x13                RET por software
         */
        emitir_instruccion(cg, "ALU",
                           "ADD x%d, x%d, x0", REG_SP, REG_FP);
        emitir_instruccion(cg, "LSU",
                           "LOAD x%d, 0(x%d)", REG_AUXILIAR, REG_FP);
        emitir_instruccion(cg, "LSU",
                           "LOAD x%d, 4(x%d)", REG_DIRECCION, REG_FP);
        emitir_instruccion(cg, "ALU",
                           "ADDI x%d, x%d, 8", REG_SP, REG_FP);
        emitir_instruccion(cg, "ALU",
                           "ADD x%d, x%d, x0", REG_FP, REG_AUXILIAR);
        emitir_instruccion(cg, "ALU",
                           "ADD x1, x%d, x0", REG_DIRECCION);
    }

    cg->funcion_actual = funcion_anterior;
    cg->dentro_funcion = dentro_anterior;
    snprintf(cg->etiqueta_retorno_funcion,
             sizeof(cg->etiqueta_retorno_funcion),
             "%s", retorno_anterior);
}

static void generar_principal(Codegen *cg, NodoAST *nodo)
{
    CodegenSimbolo simbolo_principal;

    cg->encontro_principal = 1;

    if (!resolver(cg, nodo, "principal", &simbolo_principal) ||
        simbolo_principal.categoria != CODEGEN_CAT_FUNCION) {
        error_codegen(cg, nodo,
                      "no se encontro informacion de memoria de principal");
        return;
    }

    if (!asm_emitir_etiqueta(&cg->emisor, "principal")) {
        cg->errores++;
        return;
    }

    cg->funcion_actual = "principal";
    cg->dentro_funcion = 0;

    /* principal no tiene caller: FP comienza igual a SP. */
    if (!emitir_instruccion(cg, "ALU",
                            "ADD x%d, x%d, x0", REG_FP, REG_SP) ||
        !emitir_verificacion_pila(cg, nodo,
                                  simbolo_principal.tamano_marco) ||
        !emitir_ajuste_sp(cg, -(long)simbolo_principal.tamano_marco))
        return;

    generar_nodo(cg, hijo_n(nodo, 0));

    /* Descartar locales de principal y finalizar. */
    emitir_instruccion(cg, "ALU",
                       "ADD x%d, x%d, x0", REG_SP, REG_FP);
    emitir_instruccion(cg, "BRU", "JMP fin_programa");
}

static void generar_programa(Codegen *cg, NodoAST *nodo)
{
    NodoAST *actual;

    asm_emitir_comentario(&cg->emisor,
                          "convencion: x2=retorno, x14=SP, x15=FP");
    asm_emitir_comentario(&cg->emisor,
                          "pila: 0x8000..0xFFFF, SP inicial=0x10000");

    if (!emitir_constante(cg, REG_SP, (int32_t)PILA_INICIAL_DEFECTO) ||
        !emitir_instruccion(cg, "ALU",
                            "ADD x%d, x%d, x0", REG_FP, REG_SP))
        return;

    /* Primera pasada: inicializaciones globales antes de principal. */
    for (actual = nodo->hijo; actual != NULL; actual = actual->siguiente) {
        if (actual->tipo != AST_FUNCION &&
            actual->tipo != AST_PRINCIPAL &&
            actual->tipo != AST_IMPORTACION)
            generar_nodo(cg, actual);
    }

    if (!emitir_instruccion(cg, "BRU", "JMP principal"))
        return;

    /* Segunda pasada: funciones normales. */
    for (actual = nodo->hijo; actual != NULL; actual = actual->siguiente)
        if (actual->tipo == AST_FUNCION)
            generar_funcion(cg, actual);

    /* Tercera pasada: punto de entrada. */
    for (actual = nodo->hijo; actual != NULL; actual = actual->siguiente) {
        if (actual->tipo == AST_PRINCIPAL) {
            generar_principal(cg, actual);
            break;
        }
    }
}

static void generar_nodo(Codegen *cg, NodoAST *nodo)
{
    if (nodo == NULL)
        return;

    switch (nodo->tipo) {
    case AST_PROGRAMA:
        generar_programa(cg, nodo);
        break;

    case AST_BLOQUE:
        generar_lista_hermanos(cg, nodo->hijo);
        break;

    case AST_PRINCIPAL:
        generar_principal(cg, nodo);
        break;

    case AST_DECLARACION:
        generar_declaracion(cg, nodo);
        break;

    case AST_ASIGNACION:
        generar_asignacion(cg, nodo);
        break;

    case AST_IMPORTACION:
        /* Las importaciones deben haberse resuelto antes de Fase 4. */
        break;

    case AST_IF:
        generar_if(cg, nodo);
        break;

    case AST_WHILE:
        generar_while(cg, nodo);
        break;

    case AST_FOR:
        generar_for(cg, nodo);
        break;

    case AST_BREAK:
        generar_break(cg, nodo);
        break;

    case AST_DECL_LISTA:
        generar_declaracion_lista(cg, nodo);
        break;

    case AST_DECL_MATRIZ:
        generar_declaracion_matriz(cg, nodo);
        break;

    case AST_FUNCION:
        generar_funcion(cg, nodo);
        break;

    case AST_RETURN:
        generar_return(cg, nodo);
        break;

    case AST_LLAMADA:
        if (generar_llamada(cg, nodo, 0) < 0)
            return;
        break;

    default:
        /* Expresiones no deben aparecer solas como sentencias aquí. */
        break;
    }
}

int codegen_generar_archivo(NodoAST *raiz,
                            const char *ruta_salida,
                            const CodegenConfig *config)
{
    Codegen cg;

    if (raiz == NULL || ruta_salida == NULL || config == NULL)
        return 0;

    memset(&cg, 0, sizeof(cg));
    cg.config = *config;
    cg.raiz = raiz;

    if (cg.config.nops_dependencia == 0)
        cg.config.nops_dependencia = NOPS_POR_DEFECTO;

    if (!asm_emitter_abrir(&cg.emisor, ruta_salida)) {
        perror("No se pudo crear el archivo ensamblador");
        return 0;
    }

    asm_emitir_comentario(&cg.emisor,
                          "Ensamblador intermedio generado por cdff");
    asm_emitir_comentario(&cg.emisor,
                          "Cada instruccion emitida ocupa un bundle de 16 bytes");

    generar_nodo(&cg, raiz);

    if (!cg.encontro_principal)
        error_codegen(&cg, raiz, "no se encontro la funcion principal");

    asm_emitir_etiqueta(&cg.emisor, "fin_programa");
    /* Etiqueta pendiente de resolver por la Fase 5. */
    asm_emitir_bundle(&cg.emisor, "BRU: JMP fin_programa");

    if (cg.usa_error_pila) {
        asm_emitir_etiqueta(&cg.emisor, "error_pila");
        asm_emitir_bundle(&cg.emisor, "BRU: JMP error_pila");
    }

    asm_emitter_cerrar(&cg.emisor);

    return cg.errores == 0 && !cg.emisor.error;
}
