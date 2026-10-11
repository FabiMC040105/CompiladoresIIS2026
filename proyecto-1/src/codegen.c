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

typedef struct {
    AsmEmitter emisor;
    CodegenConfig config;
    unsigned char temp_ocupado[16];
    int errores;
    int encontro_principal;
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
        else if (strcmp(nodo->valor, "AND") == 0)
            reg = emitir_instruccion(cg, "ALU",
                                     "AND x%d, x%d, x%d", izq, izq, der);
        else if (strcmp(nodo->valor, "OR") == 0)
            reg = emitir_instruccion(cg, "ALU",
                                     "OR x%d, x%d, x%d", izq, izq, der);
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

    case AST_FUNCION:
    case AST_IF:
    case AST_WHILE:
    case AST_FOR:
    case AST_RETURN:
    case AST_BREAK:
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
