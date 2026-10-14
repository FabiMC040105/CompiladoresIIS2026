#include "ast.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ast.c usa yylineno; este test construye el AST manualmente. */
int yylineno = 1;

typedef struct {
    const char *nombre;
    unsigned direccion;
    const char *tipo;
} SimboloPrueba;

static const SimboloPrueba simbolos[] = {
    {"base", 0, "entero"},
    {"exp", 4, "entero"},
    {"pot", 8, "entero"},
    {"cero", 12, "entero"},
    {"neg", 16, "entero"}
};

static int resolver_prueba(void *contexto,
                           const char *nombre,
                           CodegenSimbolo *salida)
{
    size_t i;
    (void)contexto;

    if (strcmp(nombre, "principal") == 0) {
        memset(salida, 0, sizeof(*salida));
        salida->nombre = "principal";
        salida->tipo = "vacio";
        salida->categoria = CODEGEN_CAT_FUNCION;
        salida->ubicacion = CODEGEN_UBICACION_FUNCION;
        salida->tamano_marco = 20;
        return 1;
    }
    salida->categoria = CODEGEN_CAT_VARIABLE;
    salida->ubicacion = CODEGEN_UBICACION_LOCAL;

    for (i = 0; i < sizeof(simbolos) / sizeof(simbolos[0]); i++) {
        if (strcmp(simbolos[i].nombre, nombre) == 0) {
            salida->nombre = simbolos[i].nombre;
            salida->direccion = -(long)simbolos[i].direccion - 4;
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

static NodoAST *crear_programa_prueba(void)
{
    NodoAST *programa = ast_crear(AST_PROGRAMA, "programa");
    NodoAST *principal = ast_crear(AST_PRINCIPAL, "principal");
    NodoAST *cuerpo = ast_crear(AST_BLOQUE, "bloque");

    /*
     * entero base = 2 :)
     * entero exp = 5 :)
     * entero pot = base ** exp :)  -> 32
     * entero cero = base ** 0 :)   -> 1
     * entero neg = (-2) ** 3 :)    -> -8
     */
    ast_agregar_hijo(cuerpo, declaracion("base", num("2")));
    ast_agregar_hijo(cuerpo, declaracion("exp", num("5")));
    ast_agregar_hijo(cuerpo,
                     declaracion("pot", op("**", var("base"), var("exp"))));
    ast_agregar_hijo(cuerpo,
                     declaracion("cero", op("**", var("base"), num("0"))));
    ast_agregar_hijo(cuerpo,
                     declaracion("neg", op("**", neg(num("2")), num("3"))));

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
    CodegenConfig config = {0};
    char *contenido;
    int ok;
    const char *salida = "salida_codegen_pow.asm";

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

    /* Nunca se debe inventar una instruccion POW del hardware. */
    if (strstr(contenido, "ALU: POW") != NULL ||
        strstr(contenido, " POW ") != NULL) {
        fprintf(stderr, "FAIL: se emitio POW como instruccion de hardware.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido,
               "# potencia por software: multiplicacion repetida") == NULL ||
        strstr(contenido,
               "# multiplicacion por software: shift-and-add") == NULL) {
        fprintf(stderr,
                "FAIL: la potencia no reutilizo la multiplicacion software.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido, "pow_bucle_") == NULL ||
        strstr(contenido, "pow_fin_") == NULL ||
        strstr(contenido, "BRU: BEQ ") == NULL ||
        strstr(contenido, "ALU: ADDI ") == NULL ||
        strstr(contenido, ", -1") == NULL) {
        fprintf(stderr,
                "FAIL: faltan instrucciones o etiquetas de la potencia.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido,
               "# exponente negativo no soportado: resultado=0") == NULL) {
        fprintf(stderr,
                "FAIL: falta proteccion ante exponentes negativos.\n");
        free(contenido);
        return 1;
    }

    free(contenido);

    printf("PASS: '**' se traduce sin usar una instruccion POW.\n");
    printf("PASS: la potencia reutiliza la multiplicacion por software.\n");
    printf("PASS: exponente 0 produce 1 y hay proteccion para negativos.\n");
    printf("Salida: %s\n", salida);
    return 0;
}
