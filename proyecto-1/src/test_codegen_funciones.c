#include "ast.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylineno = 1;

static NodoAST *nodo(TipoNodo tipo, const char *valor, int linea)
{
    NodoAST *n = ast_crear(tipo, valor);
    n->linea = linea;
    return n;
}

static NodoAST *var(const char *nombre, int linea)
{
    return nodo(AST_VARIABLE, nombre, linea);
}

static NodoAST *num(const char *valor, int linea)
{
    return nodo(AST_NUMERO, valor, linea);
}

static NodoAST *op(const char *operador, NodoAST *a, NodoAST *b, int linea)
{
    NodoAST *n = nodo(AST_OPERACION, operador, linea);
    ast_agregar_hijo(n, a);
    ast_agregar_hijo(n, b);
    return n;
}

static NodoAST *declaracion(const char *nombre, NodoAST *expr, int linea)
{
    NodoAST *n = nodo(AST_DECLARACION, "entero", linea);
    ast_agregar_hijo(n, var(nombre, linea));
    ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *asignacion(const char *nombre, NodoAST *expr, int linea)
{
    NodoAST *n = nodo(AST_ASIGNACION, "=", linea);
    ast_agregar_hijo(n, var(nombre, linea));
    ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *retorno(NodoAST *expr, int linea)
{
    NodoAST *n = nodo(AST_RETURN, "devuelva", linea);
    ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *bloque(NodoAST *primero, int linea)
{
    NodoAST *n = nodo(AST_BLOQUE, "bloque", linea);
    ast_agregar_hijo(n, primero);
    return n;
}

static void anexar(NodoAST *primero, NodoAST *nuevo)
{
    NodoAST *p = primero;
    while (p->siguiente != NULL)
        p = p->siguiente;
    p->siguiente = nuevo;
}

/*
 * Simula la futura resolucion por nodo/ambito de Fase 3.
 * Las lineas < 100 pertenecen a factorial; >=100 a principal.
 */
static int resolver_por_nodo(void *contexto,
                             const NodoAST *identificador,
                             const char *nombre,
                             CodegenSimbolo *salida)
{
    (void)contexto;

    salida->nombre = nombre;
    salida->tipo = "entero";
    salida->forma = CODEGEN_FORMA_ESCALAR;

    if (identificador != NULL && identificador->linea < 100) {
        if (strcmp(nombre, "n") == 0) {
            salida->direccion = 100;
            return 1;
        }
        if (strcmp(nombre, "resultado") == 0) {
            salida->direccion = 104;
            return 1;
        }
        if (strcmp(nombre, "i") == 0) {
            salida->direccion = 108;
            return 1;
        }
    } else {
        if (strcmp(nombre, "n") == 0) {
            salida->direccion = 200;
            return 1;
        }
        if (strcmp(nombre, "resultado") == 0) {
            salida->direccion = 204;
            return 1;
        }
    }

    return 0;
}

static char *leer_archivo(const char *ruta)
{
    FILE *f = fopen(ruta, "rb");
    long tam;
    char *texto;

    if (f == NULL)
        return NULL;

    fseek(f, 0, SEEK_END);
    tam = ftell(f);
    rewind(f);

    texto = malloc((size_t)tam + 1);
    if (texto == NULL) {
        fclose(f);
        return NULL;
    }

    fread(texto, 1, (size_t)tam, f);
    texto[tam] = '\0';
    fclose(f);
    return texto;
}

int main(void)
{
    NodoAST *programa;
    NodoAST *funcion;
    NodoAST *params;
    NodoAST *param;
    NodoAST *cuerpo_fact;
    NodoAST *s1, *s2, *s3, *s4, *s5;
    NodoAST *if_n;
    NodoAST *while_n;
    NodoAST *principal;
    NodoAST *cuerpo_main;
    NodoAST *m1, *m2;
    NodoAST *llamada;
    CodegenConfig cfg = {0};
    char *salida;
    int ok;

    programa = nodo(AST_PROGRAMA, "programa", 1);

    /* defina entero factorial(entero n) { ... } */
    funcion = nodo(AST_FUNCION, "entero", 10);
    ast_agregar_hijo(funcion, var("factorial", 10));

    params = nodo(AST_PARAMETROS, "parametros", 11);
    param = nodo(AST_PARAMETRO, "entero", 11);
    ast_agregar_hijo(param, var("n", 11));
    ast_agregar_hijo(params, param);
    ast_agregar_hijo(funcion, params);

    s1 = declaracion("resultado", num("1", 20), 20);
    s2 = declaracion("i", num("1", 21), 21);

    if_n = nodo(AST_IF, "si", 22);
    ast_agregar_hijo(if_n,
                     op("==", var("n", 22), num("0", 22), 22));
    ast_agregar_hijo(if_n, bloque(retorno(num("1", 23), 23), 23));
    s3 = if_n;

    while_n = nodo(AST_WHILE, "mientras", 30);
    ast_agregar_hijo(while_n,
                     op("<=", var("i", 30), var("n", 30), 30));
    {
        NodoAST *w1 = asignacion(
            "resultado",
            op("*", var("resultado", 31), var("i", 31), 31),
            31);
        NodoAST *w2 = asignacion(
            "i",
            op("+", var("i", 32), num("1", 32), 32),
            32);
        anexar(w1, w2);
        ast_agregar_hijo(while_n, bloque(w1, 31));
    }
    s4 = while_n;
    s5 = retorno(var("resultado", 40), 40);

    anexar(s1, s2);
    anexar(s1, s3);
    anexar(s1, s4);
    anexar(s1, s5);
    cuerpo_fact = bloque(s1, 20);
    ast_agregar_hijo(funcion, cuerpo_fact);

    /* principal: n=5; resultado=factorial(n); */
    principal = nodo(AST_PRINCIPAL, "principal", 100);
    m1 = declaracion("n", num("5", 100), 100);
    llamada = nodo(AST_LLAMADA, "factorial", 101);
    ast_agregar_hijo(llamada, var("n", 101));
    m2 = declaracion("resultado", llamada, 101);
    anexar(m1, m2);
    cuerpo_main = bloque(m1, 100);
    ast_agregar_hijo(principal, cuerpo_main);

    ast_agregar_hijo(programa, funcion);
    anexar(funcion, principal);

    cfg.resolver_simbolo = NULL;
    cfg.resolver_simbolo_nodo = resolver_por_nodo;
    cfg.nops_dependencia = 4;

    ok = codegen_generar_archivo(programa,
                                 "salida_codegen_factorial.asm",
                                 &cfg);
    if (!ok) {
        fprintf(stderr, "FALLO: codegen rechazo el factorial.\n");
        ast_liberar(programa);
        return 1;
    }

    salida = leer_archivo("salida_codegen_factorial.asm");
    if (salida == NULL) {
        fprintf(stderr, "FALLO: no se pudo leer la salida.\n");
        ast_liberar(programa);
        return 1;
    }

#define EXIGIR(txt) do { \
    if (strstr(salida, (txt)) == NULL) { \
        fprintf(stderr, "FALLO: falta en ASM: %s\n", (txt)); \
        free(salida); ast_liberar(programa); return 1; \
    } \
} while (0)

    EXIGIR("BRU: JMP principal");
    EXIGIR("func_factorial:");
    EXIGIR("ret_factorial:");
    EXIGIR("BRU: JMP func_factorial");
    EXIGIR("STORE x14, 0(x2)");
    EXIGIR("LOAD x14, 0(x2)");
    EXIGIR("ADD x15, x0");
    EXIGIR("ADD x1, x14, x0");
    EXIGIR("multiplicacion por software: shift-and-add");
    EXIGIR("ADDI x13, x0, 100");
    EXIGIR("ADDI x13, x0, 204");

    if (strstr(salida, "ALU: MUL") != NULL ||
        strstr(salida, "BRU: CALL") != NULL ||
        strstr(salida, "BRU: RET") != NULL) {
        fprintf(stderr, "FALLO: se emitio una instruccion inexistente.\n");
        free(salida);
        ast_liberar(programa);
        return 1;
    }

    printf("PASS: factorial usa funcion, parametro y devuelva.\n");
    printf("PASS: CALL/RET se bajan a instrucciones reales del ISA.\n");
    printf("PASS: nombres iguales en distintos ambitos se resuelven por nodo.\n");
    printf("Salida: salida_codegen_factorial.asm\n");

    free(salida);
    ast_liberar(programa);
    return 0;
}
