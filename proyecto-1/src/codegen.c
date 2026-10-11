#include "codegen.h"
#include "asm_emitter.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_TEMP_PRIMERO 4
#define REG_TEMP_ULTIMO 12
#define REG_DIRECCION 13
#define NOPS_POR_DEFECTO 4
#define MAX_NIVELES_BUCLE 32

typedef struct {
    AsmEmitter emisor;
    CodegenConfig config;
    unsigned char temp_ocupado[16];
    int errores;
    int encontro_principal;
    unsigned contador_etiquetas;
    struct {
        char etiqueta_break[64];
        char etiqueta_continue[64];
    } pila_bucles[MAX_NIVELES_BUCLE];
    unsigned profundidad_bucle;
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
    if (cg->config.resolver_simbolo == NULL) {
        error_codegen(cg, nodo,
                      "no hay un resolvedor de simbolos conectado para '%s'",
                      nombre);
        return 0;
    }

    if (!cg->config.resolver_simbolo(cg->config.contexto_simbolos,
                                     nombre,
                                     simbolo)) {
        error_codegen(cg, nodo, "simbolo no encontrado: %s", nombre);
        return 0;
    }

    return 1;
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

    if (!emitir_constante(cg, REG_DIRECCION, (int32_t)simbolo.direccion) ||
        !emitir_instruccion(cg, "LSU",
                            "LOAD x%d, 0(x%d)", reg, REG_DIRECCION)) {
        liberar_temp(cg, reg);
        return -1;
    }

    return reg;
}

static int guardar_variable(Codegen *cg,
                            NodoAST *nodo,
                            const char *nombre,
                            int reg_valor)
{
    CodegenSimbolo simbolo;

    if (!resolver(cg, nodo, nombre, &simbolo))
        return 0;

    if (!emitir_constante(cg, REG_DIRECCION, (int32_t)simbolo.direccion))
        return 0;

    return emitir_instruccion(cg, "LSU",
                              "STORE x%d, 0(x%d)",
                              reg_valor, REG_DIRECCION);
}


static int generar_expresion(Codegen *cg, NodoAST *nodo);

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
        else {
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

    guardar_variable(cg, nodo, variable->valor, reg);
    liberar_temp(cg, reg);
}

static void generar_asignacion(Codegen *cg, NodoAST *nodo)
{
    NodoAST *destino = hijo_n(nodo, 0);
    NodoAST *expresion = hijo_n(nodo, 1);
    int reg;

    if (destino == NULL || destino->tipo != AST_VARIABLE) {
        error_codegen(cg, nodo,
                      "por ahora solo se implementan asignaciones escalares");
        return;
    }

    reg = generar_expresion(cg, expresion);
    if (reg < 0)
        return;

    guardar_variable(cg, nodo, destino->valor, reg);
    liberar_temp(cg, reg);
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

static void generar_nodo(Codegen *cg, NodoAST *nodo)
{
    if (nodo == NULL)
        return;

    switch (nodo->tipo) {
    case AST_PROGRAMA:
        generar_lista_hermanos(cg, nodo->hijo);
        break;

    case AST_BLOQUE:
        generar_lista_hermanos(cg, nodo->hijo);
        break;

    case AST_PRINCIPAL:
        cg->encontro_principal = 1;
        asm_emitir_etiqueta(&cg->emisor, "principal");
        generar_nodo(cg, hijo_n(nodo, 0));
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

    case AST_FUNCION:
    case AST_RETURN:
    case AST_LLAMADA:
    case AST_DECL_LISTA:
    case AST_DECL_MATRIZ:
        asm_emitir_comentario(&cg->emisor,
                              "TODO Fase 4: nodo tipo %d linea %d",
                              (int)nodo->tipo, nodo->linea);
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

    asm_emitter_cerrar(&cg.emisor);

    return cg.errores == 0 && !cg.emisor.error;
}
