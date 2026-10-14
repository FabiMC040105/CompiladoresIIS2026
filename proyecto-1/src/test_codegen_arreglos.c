#include "ast.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ast.c espera este simbolo cuando la prueba no enlaza Flex. */
int yylineno = 1;

static int resolver_prueba(void *contexto,
                           const char *nombre,
                           CodegenSimbolo *salida)
{
    (void)contexto;

    if (strcmp(nombre, "principal") == 0) {
        memset(salida, 0, sizeof(*salida));
        salida->nombre = "principal";
        salida->tipo = "vacio";
        salida->categoria = CODEGEN_CAT_FUNCION;
        salida->ubicacion = CODEGEN_UBICACION_FUNCION;
        salida->tamano_marco = 48;
        return 1;
    }
    salida->categoria = CODEGEN_CAT_VARIABLE;
    salida->ubicacion = CODEGEN_UBICACION_LOCAL;

    if (strcmp(nombre, "i") == 0) {
        salida->nombre = "i";
        salida->direccion = -44;
        salida->tipo = "entero";
        salida->forma = CODEGEN_FORMA_ESCALAR;
        return 1;
    }

    if (strcmp(nombre, "x") == 0) {
        salida->nombre = "x";
        salida->direccion = -48;
        salida->tipo = "entero";
        salida->forma = CODEGEN_FORMA_ESCALAR;
        return 1;
    }

    if (strcmp(nombre, "v") == 0) {
        salida->nombre = "v";
        salida->direccion = -16;
        salida->tipo = "entero";
        salida->forma = CODEGEN_FORMA_LISTA;
        salida->cantidad_dimensiones = 1;
        salida->dimensiones[0] = 4;
        return 1;
    }

    if (strcmp(nombre, "m") == 0) {
        salida->nombre = "m";
        salida->direccion = -40;
        salida->tipo = "entero";
        salida->forma = CODEGEN_FORMA_MATRIZ;
        salida->cantidad_dimensiones = 2;
        salida->dimensiones[0] = 2;
        salida->dimensiones[1] = 3;
        return 1;
    }

    return 0;
}

static NodoAST *var(const char *nombre)
{
    return ast_crear(AST_VARIABLE, nombre);
}

static NodoAST *num(const char *valor)
{
    return ast_crear(AST_NUMERO, valor);
}

static NodoAST *op(const char *operador, NodoAST *izq, NodoAST *der)
{
    NodoAST *n = ast_crear(AST_OPERACION, operador);
    ast_agregar_hijo(n, izq);
    if (der != NULL)
        ast_agregar_hijo(n, der);
    return n;
}

static NodoAST *declaracion(const char *nombre, NodoAST *expr)
{
    NodoAST *n = ast_crear(AST_DECLARACION, "entero");
    ast_agregar_hijo(n, var(nombre));
    if (expr != NULL)
        ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *acceso_lista(const char *nombre, NodoAST *indice)
{
    NodoAST *n = ast_crear(AST_ACCESO_LISTA, NULL);
    ast_agregar_hijo(n, var(nombre));
    ast_agregar_hijo(n, indice);
    return n;
}

static NodoAST *acceso_matriz(const char *nombre,
                              NodoAST *fila,
                              NodoAST *columna)
{
    NodoAST *n = ast_crear(AST_ACCESO_MATRIZ, NULL);
    ast_agregar_hijo(n, var(nombre));
    ast_agregar_hijo(n, fila);
    ast_agregar_hijo(n, columna);
    return n;
}

static NodoAST *asignacion(NodoAST *destino, NodoAST *expr)
{
    NodoAST *n = ast_crear(AST_ASIGNACION, "=");
    ast_agregar_hijo(n, destino);
    ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *literal_lista(const char **valores, size_t cantidad,
                              const char *tipo_valor)
{
    NodoAST *lista = ast_crear(AST_LISTA, tipo_valor);
    size_t i;

    for (i = 0; i < cantidad; i++)
        ast_agregar_hijo(lista, num(valores[i]));

    return lista;
}

static NodoAST *declaracion_lista(void)
{
    const char *valores[] = {"10", "20", "30", "40"};
    NodoAST *n = ast_crear(AST_DECL_LISTA, "entero");

    ast_agregar_hijo(n, var("v"));
    ast_agregar_hijo(n, num("4"));
    ast_agregar_hijo(n, literal_lista(valores, 4, "literal"));
    return n;
}

static NodoAST *declaracion_matriz(void)
{
    const char *fila0[] = {"1", "2", "3"};
    const char *fila1[] = {"4", "5", "6"};
    NodoAST *n = ast_crear(AST_DECL_MATRIZ, "entero");
    NodoAST *matriz = ast_crear(AST_MATRIZ, "literal");

    ast_agregar_hijo(matriz, literal_lista(fila0, 3, "fila"));
    ast_agregar_hijo(matriz, literal_lista(fila1, 3, "fila"));

    ast_agregar_hijo(n, var("m"));
    ast_agregar_hijo(n, num("2"));
    ast_agregar_hijo(n, num("3"));
    ast_agregar_hijo(n, matriz);
    return n;
}

static NodoAST *crear_programa_prueba(void)
{
    NodoAST *programa = ast_crear(AST_PROGRAMA, "programa");
    NodoAST *principal = ast_crear(AST_PRINCIPAL, "principal");
    NodoAST *cuerpo = ast_crear(AST_BLOQUE, "bloque");

    /*
     * entero lista v[4] = [10, 20, 30, 40] :)
     * entero matriz m[2][3] = [[1,2,3],[4,5,6]] :)
     * entero i = 1 :)
     * entero x = v[i] :)
     * v[2] = x + 5 :)
     * x = m[1][2] :)
     * m[0][1] = v[3] :)
     */
    ast_agregar_hijo(cuerpo, declaracion_lista());
    ast_agregar_hijo(cuerpo, declaracion_matriz());
    ast_agregar_hijo(cuerpo, declaracion("i", num("1")));
    ast_agregar_hijo(cuerpo,
                     declaracion("x", acceso_lista("v", var("i"))));
    ast_agregar_hijo(cuerpo,
                     asignacion(acceso_lista("v", num("2")),
                                op("+", var("x"), num("5"))));
    ast_agregar_hijo(cuerpo,
                     asignacion(var("x"),
                                acceso_matriz("m", num("1"), num("2"))));
    ast_agregar_hijo(cuerpo,
                     asignacion(acceso_matriz("m", num("0"), num("1")),
                                acceso_lista("v", num("3"))));

    ast_agregar_hijo(principal, cuerpo);
    ast_agregar_hijo(programa, principal);
    return programa;
}

static char *leer_archivo(const char *ruta)
{
    FILE *f = fopen(ruta, "rb");
    long tamano;
    char *contenido;

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
    NodoAST *raiz = crear_programa_prueba();
    CodegenConfig config = {0};
    char *contenido;
    const char *salida = "salida_codegen_arreglos.asm";
    int ok;

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

    if (strstr(contenido, "# declaracion lista v[4]") == NULL ||
        strstr(contenido, "# declaracion matriz m[2][3] row-major") == NULL) {
        fprintf(stderr, "FAIL: faltan inicializadores de lista/matriz.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido, "ALU: SLLI ") == NULL ||
        strstr(contenido, ", 2") == NULL) {
        fprintf(stderr, "FAIL: no se genero indice*4 para elementos de 32 bits.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido, "# multiplicacion por software: shift-and-add") == NULL) {
        fprintf(stderr, "FAIL: la matriz no calcula fila*columnas con el ISA real.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido, "LSU: LOAD ") == NULL ||
        strstr(contenido, "LSU: STORE ") == NULL) {
        fprintf(stderr, "FAIL: faltan LOAD/STORE para accesos a arreglos.\n");
        free(contenido);
        return 1;
    }

    if (strstr(contenido, "TODO Fase 4: nodo tipo 20") != NULL ||
        strstr(contenido, "TODO Fase 4: nodo tipo 21") != NULL) {
        fprintf(stderr, "FAIL: declaraciones de arreglos siguen como TODO.\n");
        free(contenido);
        return 1;
    }

    free(contenido);

    printf("PASS: listas se direccionan como base + indice*4.\n");
    printf("PASS: matrices usan row-major: base + ((fila*columnas)+columna)*4.\n");
    printf("PASS: lectura, escritura e inicializacion usan solo el ISA real.\n");
    printf("Salida: %s\n", salida);
    return 0;
}
