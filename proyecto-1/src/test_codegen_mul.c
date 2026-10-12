#include "ast.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ast.c usa yylineno al crear nodos. En este test no usamos Flex. */
int yylineno = 1;

typedef struct {
    const char *nombre;
    unsigned direccion;
    const char *tipo;
} SimboloPrueba;

static const SimboloPrueba simbolos[] = {
    {"a", 0, "entero"},
    {"b", 4, "entero"},
    {"c", 8, "entero"},
    {"d", 12, "entero"}
};

static int resolver_prueba(void *contexto,
                           const char *nombre,
                           CodegenSimbolo *salida)
{
    size_t i;
    (void)contexto;

    for (i = 0; i < sizeof(simbolos) / sizeof(simbolos[0]); i++) {
        if (strcmp(simbolos[i].nombre, nombre) == 0) {
            salida->nombre = simbolos[i].nombre;
            salida->direccion = simbolos[i].direccion;
            salida->tipo = simbolos[i].tipo;
            return 1;
        }
    }

    return 0;
}

static NodoAST *op(const char *operador, NodoAST *izq, NodoAST *der)
{
    NodoAST *n = ast_crear(AST_OPERACION, operador);
    ast_agregar_hijo(n, izq);
    if (der != NULL)
        ast_agregar_hijo(n, der);
    return n;
}

static NodoAST *var(const char *nombre)
{
    return ast_crear(AST_VARIABLE, nombre);
}

static NodoAST *num(const char *valor)
{
    return ast_crear(AST_NUMERO, valor);
}

static NodoAST *neg(NodoAST *valor)
{
    return op("unario -", valor, NULL);
}

static NodoAST *declaracion(const char *nombre, NodoAST *expresion)
{
    NodoAST *n = ast_crear(AST_DECLARACION, "entero");
    ast_agregar_hijo(n, var(nombre));
    if (expresion != NULL)
        ast_agregar_hijo(n, expresion);
    return n;
}

static NodoAST *bloque(void)
{
    return ast_crear(AST_BLOQUE, "bloque");
}

static NodoAST *crear_programa_prueba(void)
{
    NodoAST *programa = ast_crear(AST_PROGRAMA, "programa");
    NodoAST *principal = ast_crear(AST_PRINCIPAL, "principal");
    NodoAST *cuerpo = bloque();

    /*
     * defina vacio principal() {
     *     entero a = -7 :)
     *     entero b = 6 :)
     *     entero c = a * b :)
     *     entero d = c * -3 :)
     * } :)
     *
     * El objetivo no es ejecutar todavia el assembler final, sino
     * comprobar que Fase 4 baje '*' a instrucciones reales del ISA.
     */
    ast_agregar_hijo(cuerpo, declaracion("a", neg(num("7"))));
    ast_agregar_hijo(cuerpo, declaracion("b", num("6")));
    ast_agregar_hijo(cuerpo,
                     declaracion("c", op("*", var("a"), var("b"))));
    ast_agregar_hijo(cuerpo,
                     declaracion("d", op("*", var("c"), neg(num("3")))));

    ast_agregar_hijo(principal, cuerpo);
    ast_agregar_hijo(programa, principal);
    return programa;
}

static char *leer_archivo(const char *ruta)
{
    FILE *f;
    long tamano;
    char *contenido;

    f = fopen(ruta, "rb");
    if (f == NULL)
        return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    tamano = ftell(f);
    if (tamano < 0) {
        fclose(f);
        return NULL;
    }

    rewind(f);
    contenido = malloc((size_t)tamano + 1);
    if (contenido == NULL) {
        fclose(f);
        return NULL;
    }

    if (fread(contenido, 1, (size_t)tamano, f) != (size_t)tamano) {
        free(contenido);
        fclose(f);
        return NULL;
    }

    contenido[tamano] = '\0';
    fclose(f);
    return contenido;
}

int main(void)
{
    NodoAST *raiz;
    CodegenConfig config;
    char *contenido;
    int ok;
    const char *salida = "salida_codegen_mul.asm";

    raiz = crear_programa_prueba();

    config.resolver_simbolo = resolver_prueba;
    config.contexto_simbolos = NULL;
    config.nops_dependencia = 4;

    ok = codegen_generar_archivo(raiz, salida, &config);
    ast_liberar(raiz);

    if (!ok) {
        fprintf(stderr, "FAIL: el generador reporto errores.\n");
        return 1;
    }

    contenido = leer_archivo(salida);
    if (contenido == NULL) {
        fprintf(stderr, "FAIL: no se pudo leer %s.\n", salida);
        return 1;
    }

    /* La ISA no posee MUL: nunca debe aparecer como instruccion emitida. */
    if (strstr(contenido, "ALU: MUL") != NULL ||
        strstr(contenido, "MUL ") != NULL) {
        fprintf(stderr, "FAIL: se emitio una instruccion MUL inexistente.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido,
               "# multiplicacion por software: shift-and-add") == NULL) {
        fprintf(stderr, "FAIL: no se genero la rutina de multiplicacion.\n");
        free(contenido);
        return 1;
    }

    /* Operaciones que deben construir la multiplicacion con el ISA real. */
    if (strstr(contenido, "ALU: AND ") == NULL ||
        strstr(contenido, "ALU: ADD ") == NULL ||
        strstr(contenido, "ALU: SLLI ") == NULL ||
        strstr(contenido, "ALU: SRLI ") == NULL ||
        strstr(contenido, "mul_bucle_") == NULL ||
        strstr(contenido, "mul_fin_bucle_") == NULL) {
        fprintf(stderr,
                "FAIL: faltan instrucciones/etiquetas del shift-and-add.\n");
        free(contenido);
        return 1;
    }

    /* Debe existir manejo de signo para productos negativos. */
    if (strstr(contenido, "mul_izq_positivo_") == NULL ||
        strstr(contenido, "mul_der_positivo_") == NULL ||
        strstr(contenido, "mul_fin_signo_") == NULL) {
        fprintf(stderr, "FAIL: falta el manejo de signo de la multiplicacion.\n");
        free(contenido);
        return 1;
    }

    free(contenido);

    printf("PASS: '*' se traduce usando solamente instrucciones del ISA.\n");
    printf("PASS: no se emitio ninguna instruccion MUL inexistente.\n");
    printf("Salida: %s\n", salida);
    return 0;
}
