#include "ast.h"
#include "codegen.h"

#include <stdio.h>
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
    {"c", 8, "entero"}
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

static NodoAST *crear_declaracion(const char *nombre, NodoAST *expresion)
{
    NodoAST *declaracion = ast_crear(AST_DECLARACION, "entero");
    NodoAST *variable = ast_crear(AST_VARIABLE, nombre);

    ast_agregar_hijo(declaracion, variable);
    if (expresion != NULL)
        ast_agregar_hijo(declaracion, expresion);

    return declaracion;
}

static NodoAST *crear_suma_variables(const char *izquierda,
                                     const char *derecha)
{
    NodoAST *suma = ast_crear(AST_OPERACION, "+");

    ast_agregar_hijo(suma, ast_crear(AST_VARIABLE, izquierda));
    ast_agregar_hijo(suma, ast_crear(AST_VARIABLE, derecha));

    return suma;
}

static NodoAST *crear_programa_prueba(void)
{
    NodoAST *programa = ast_crear(AST_PROGRAMA, "programa");
    NodoAST *principal = ast_crear(AST_PRINCIPAL, "principal");
    NodoAST *bloque = ast_crear(AST_BLOQUE, "bloque");

    NodoAST *decl_a = crear_declaracion(
        "a",
        ast_crear(AST_NUMERO, "5")
    );

    NodoAST *decl_b = crear_declaracion(
        "b",
        ast_crear(AST_NUMERO, "3")
    );

    NodoAST *decl_c = crear_declaracion(
        "c",
        crear_suma_variables("a", "b")
    );

    /* Las tres declaraciones quedan como sentencias hermanas del bloque. */
    ast_agregar_hijo(bloque, decl_a);
    ast_agregar_hijo(bloque, decl_b);
    ast_agregar_hijo(bloque, decl_c);

    ast_agregar_hijo(principal, bloque);
    ast_agregar_hijo(programa, principal);

    return programa;
}

int main(void)
{
    NodoAST *raiz;
    CodegenConfig config;
    int ok;

    raiz = crear_programa_prueba();

    config.resolver_simbolo = resolver_prueba;
    config.contexto_simbolos = NULL;
    config.nops_dependencia = 4;

    ok = codegen_generar_archivo(
        raiz,
        "salida_codegen.asm",
        &config
    );

    ast_liberar(raiz);

    if (!ok) {
        fprintf(stderr, "FAIL: el generador reporto errores.\n");
        return 1;
    }

    printf("PASS: AST convertido a salida_codegen.asm\n");
    printf("Simbolos usados: a@0, b@4, c@8\n");
    return 0;
}
