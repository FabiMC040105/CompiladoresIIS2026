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
    {"i", 0, "entero"},
    {"total", 4, "entero"}
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

static NodoAST *declaracion(const char *nombre, NodoAST *expresion)
{
    NodoAST *n = ast_crear(AST_DECLARACION, "entero");
    ast_agregar_hijo(n, var(nombre));
    if (expresion != NULL)
        ast_agregar_hijo(n, expresion);
    return n;
}

static NodoAST *asignacion(const char *nombre, NodoAST *expresion)
{
    NodoAST *n = ast_crear(AST_ASIGNACION, "=");
    ast_agregar_hijo(n, var(nombre));
    ast_agregar_hijo(n, expresion);
    return n;
}

static NodoAST *bloque(void)
{
    return ast_crear(AST_BLOQUE, "bloque");
}

static NodoAST *nodo_if(NodoAST *condicion, NodoAST *cuerpo)
{
    NodoAST *n = ast_crear(AST_IF, "si");
    ast_agregar_hijo(n, condicion);
    ast_agregar_hijo(n, cuerpo);
    return n;
}

static NodoAST *nodo_for(NodoAST *inicio,
                         NodoAST *condicion,
                         NodoAST *incremento,
                         NodoAST *cuerpo)
{
    NodoAST *n = ast_crear(AST_FOR, "para");
    ast_agregar_hijo(n, inicio);
    ast_agregar_hijo(n, condicion);
    ast_agregar_hijo(n, incremento);
    ast_agregar_hijo(n, cuerpo);
    return n;
}

static NodoAST *crear_programa_prueba(void)
{
    NodoAST *programa = ast_crear(AST_PROGRAMA, "programa");
    NodoAST *principal = ast_crear(AST_PRINCIPAL, "principal");
    NodoAST *cuerpo_principal = bloque();
    NodoAST *cuerpo_for = bloque();
    NodoAST *cuerpo_if = bloque();

    /*
     * entero total = 0
     *
     * para (entero i = 0, i < 4, i = i + 1) {
     *     si (i == 2) {
     *         rompa
     *     }
     *     total = total + i
     * }
     */
    ast_agregar_hijo(cuerpo_principal,
                     declaracion("total", num("0")));

    ast_agregar_hijo(cuerpo_if, ast_crear(AST_BREAK, "rompa"));
    ast_agregar_hijo(cuerpo_for,
                     nodo_if(op("==", var("i"), num("2")), cuerpo_if));
    ast_agregar_hijo(cuerpo_for,
                     asignacion("total",
                                op("+", var("total"), var("i"))));

    ast_agregar_hijo(
        cuerpo_principal,
        nodo_for(
            declaracion("i", num("0")),
            op("<", var("i"), num("4")),
            asignacion("i", op("+", var("i"), num("1"))),
            cuerpo_for
        )
    );

    ast_agregar_hijo(principal, cuerpo_principal);
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

static int extraer_etiqueta(const char *texto,
                            const char *prefijo,
                            char *salida,
                            size_t capacidad)
{
    const char *p = strstr(texto, prefijo);
    size_t n = 0;

    if (p == NULL || capacidad == 0)
        return 0;

    while (*p != '\0' && *p != ':' && *p != '\n' && n + 1 < capacidad)
        salida[n++] = *p++;

    salida[n] = '\0';
    return n > 0;
}

int main(void)
{
    NodoAST *raiz;
    CodegenConfig config;
    char *contenido;
    char inicio[64];
    char incremento[64];
    char fin[64];
    char esperado[160];
    int ok;
    const char *salida = "salida_codegen_for.asm";

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

    if (!extraer_etiqueta(contenido, "inicio_for_", inicio, sizeof(inicio)) ||
        !extraer_etiqueta(contenido, "incremento_for_", incremento,
                          sizeof(incremento)) ||
        !extraer_etiqueta(contenido, "fin_for_", fin, sizeof(fin))) {
        fprintf(stderr, "FAIL: faltan etiquetas del ciclo para.\n");
        free(contenido);
        return 1;
    }

    /* La condicion debe poder salir al final del for. */
    snprintf(esperado, sizeof(esperado), "%s", fin);
    if (strstr(contenido, esperado) == NULL) {
        fprintf(stderr, "FAIL: la condicion no referencia el final del for.\n");
        free(contenido);
        return 1;
    }

    /* rompa debe apuntar exactamente al final del for. */
    snprintf(esperado, sizeof(esperado), "# rompa -> %s", fin);
    if (strstr(contenido, esperado) == NULL) {
        fprintf(stderr, "FAIL: rompa no apunta al final del for.\n");
        free(contenido);
        return 1;
    }

    /* El incremento debe tener su propia etiqueta antes del salto de regreso. */
    snprintf(esperado, sizeof(esperado), "%s:", incremento);
    if (strstr(contenido, esperado) == NULL ||
        strstr(contenido, "# incremento para") == NULL) {
        fprintf(stderr, "FAIL: no se genero la zona de incremento.\n");
        free(contenido);
        return 1;
    }

    snprintf(esperado, sizeof(esperado), "BRU: JMP %s", inicio);
    if (strstr(contenido, esperado) == NULL) {
        fprintf(stderr, "FAIL: falta el salto de regreso del for.\n");
        free(contenido);
        return 1;
    }

    free(contenido);

    printf("PASS: para y rompa generados correctamente.\n");
    printf("Inicio: %s | Incremento: %s | Fin: %s\n",
           inicio, incremento, fin);
    printf("Salida: %s\n", salida);
    return 0;
}
