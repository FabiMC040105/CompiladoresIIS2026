#include "ast.h"
#include "codegen.h"
#include "codegen_tabla.h"
#include "tabla_simbolos.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylineno = 1;

static Ambito *amb_fact = NULL;
static Ambito *amb_main = NULL;

static Ambito *ambito_de_nodo(const NodoAST *nodo)
{
    if (nodo == NULL)
        return NULL;
    return nodo->linea < 100 ? amb_fact : amb_main;
}

static NodoAST *nodo(TipoNodo tipo, const char *valor, int linea)
{
    NodoAST *n = ast_crear(tipo, valor);
    n->linea = linea;
    return n;
}

static NodoAST *var(const char *nombre, int linea) { return nodo(AST_VARIABLE, nombre, linea); }
static NodoAST *num(const char *valor, int linea) { return nodo(AST_NUMERO, valor, linea); }

static NodoAST *op(const char *operador, NodoAST *a, NodoAST *b, int linea)
{
    NodoAST *n = nodo(AST_OPERACION, operador, linea);
    ast_agregar_hijo(n, a);
    ast_agregar_hijo(n, b);
    return n;
}

static NodoAST *decl(const char *nombre, NodoAST *expr, int linea)
{
    NodoAST *n = nodo(AST_DECLARACION, "entero", linea);
    ast_agregar_hijo(n, var(nombre, linea));
    if (expr) ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *asig(const char *nombre, NodoAST *expr, int linea)
{
    NodoAST *n = nodo(AST_ASIGNACION, "=", linea);
    ast_agregar_hijo(n, var(nombre, linea));
    ast_agregar_hijo(n, expr);
    return n;
}

static NodoAST *ret(NodoAST *expr, int linea)
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

static void anexar(NodoAST *a, NodoAST *b)
{
    while (a->siguiente) a = a->siguiente;
    a->siguiente = b;
}

static int construir_tabla(TablaSimbolos **salida)
{
    TablaSimbolos *t = ts_crear();
    Simbolo *n = NULL, *resultado = NULL, *i = NULL;
    Simbolo *fn = NULL, *main_fn = NULL;
    Simbolo *main_n = NULL, *main_r = NULL;
    Ambito *guardado;

    if (!t) return 0;

    amb_fact = ts_entrar_ambito(t, "factorial", AMBITO_FUNCION);
    if (!amb_fact) return 0;
    if (ts_insertar(t, "n", SIM_PARAMETRO, TIPO_ENTERO, FORMA_ESCALAR, 11, &n) != TS_OK ||
        ts_insertar(t, "resultado", SIM_VARIABLE, TIPO_ENTERO, FORMA_ESCALAR, 20, &resultado) != TS_OK ||
        ts_insertar(t, "i", SIM_VARIABLE, TIPO_ENTERO, FORMA_ESCALAR, 21, &i) != TS_OK)
        return 0;
    ts_salir_ambito(t);

    guardado = t->ambito_actual;
    t->ambito_actual = t->ambito_global;
    if (ts_insertar(t, "factorial", SIM_FUNCION, TIPO_ENTERO, FORMA_ESCALAR, 10, &fn) != TS_OK)
        return 0;
    fn->ambito_funcion = amb_fact;
    if (ts_agregar_parametro(fn, n) != TS_OK) return 0;
    t->ambito_actual = guardado;

    amb_main = ts_entrar_ambito(t, "principal", AMBITO_FUNCION);
    if (!amb_main) return 0;
    if (ts_insertar(t, "n", SIM_VARIABLE, TIPO_ENTERO, FORMA_ESCALAR, 100, &main_n) != TS_OK ||
        ts_insertar(t, "resultado", SIM_VARIABLE, TIPO_ENTERO, FORMA_ESCALAR, 101, &main_r) != TS_OK)
        return 0;
    ts_salir_ambito(t);

    guardado = t->ambito_actual;
    t->ambito_actual = t->ambito_global;
    if (ts_insertar(t, "principal", SIM_FUNCION, TIPO_VACIO, FORMA_ESCALAR, 100, &main_fn) != TS_OK)
        return 0;
    main_fn->ambito_funcion = amb_main;
    t->ambito_actual = guardado;

    if (ts_asignar_direcciones_globales(t) != TS_OK)
        return 0;

    if (n->direccion != 8 || resultado->direccion != -4 || i->direccion != -8 ||
        fn->tamano_parametros != 4 || fn->tamano_marco != 8 ||
        main_n->direccion != -4 || main_r->direccion != -8 ||
        main_fn->tamano_marco != 8) {
        fprintf(stderr, "FALLO: memoria de tabla no coincide con la convencion.\n");
        return 0;
    }

    *salida = t;
    return 1;
}

static NodoAST *construir_ast(void)
{
    NodoAST *programa = nodo(AST_PROGRAMA, "programa", 1);
    NodoAST *funcion = nodo(AST_FUNCION, "entero", 10);
    NodoAST *params = nodo(AST_PARAMETROS, "parametros", 11);
    NodoAST *param = nodo(AST_PARAMETRO, "entero", 11);
    NodoAST *s1, *s2, *s3, *s4, *s5, *w1, *w2;
    NodoAST *ifn, *wh;
    NodoAST *principal, *m1, *m2, *llamada;

    ast_agregar_hijo(funcion, var("factorial", 10));
    ast_agregar_hijo(param, var("n", 11));
    ast_agregar_hijo(params, param);
    ast_agregar_hijo(funcion, params);

    s1 = decl("resultado", num("1", 20), 20);
    s2 = decl("i", num("1", 21), 21);

    ifn = nodo(AST_IF, "si", 22);
    ast_agregar_hijo(ifn, op("==", var("n", 22), num("0", 22), 22));
    ast_agregar_hijo(ifn, bloque(ret(num("1", 23), 23), 23));
    s3 = ifn;

    wh = nodo(AST_WHILE, "mientras", 30);
    ast_agregar_hijo(wh, op("<=", var("i", 30), var("n", 30), 30));
    w1 = asig("resultado", op("*", var("resultado", 31), var("i", 31), 31), 31);
    w2 = asig("i", op("+", var("i", 32), num("1", 32), 32), 32);
    anexar(w1, w2);
    ast_agregar_hijo(wh, bloque(w1, 31));
    s4 = wh;
    s5 = ret(var("resultado", 40), 40);

    anexar(s1, s2); anexar(s1, s3); anexar(s1, s4); anexar(s1, s5);
    ast_agregar_hijo(funcion, bloque(s1, 20));

    principal = nodo(AST_PRINCIPAL, "principal", 100);
    m1 = decl("n", num("5", 100), 100);
    llamada = nodo(AST_LLAMADA, "factorial", 101);
    ast_agregar_hijo(llamada, var("n", 101));
    m2 = decl("resultado", llamada, 101);
    anexar(m1, m2);
    ast_agregar_hijo(principal, bloque(m1, 100));

    ast_agregar_hijo(programa, funcion);
    anexar(funcion, principal);
    return programa;
}

static char *leer(const char *ruta)
{
    FILE *f = fopen(ruta, "rb");
    long n;
    char *s;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); n = ftell(f); rewind(f);
    s = malloc((size_t)n + 1);
    if (!s) { fclose(f); return NULL; }
    fread(s, 1, (size_t)n, f); s[n] = '\0'; fclose(f); return s;
}

int main(void)
{
    TablaSimbolos *tabla = NULL;
    NodoAST *ast;
    CodegenTablaContexto contexto;
    CodegenConfig cfg = {0};
    char *asm_txt;

    if (!construir_tabla(&tabla)) return 1;
    ast = construir_ast();

    contexto.tabla = tabla;
    contexto.obtener_ambito_nodo = ambito_de_nodo;
    cfg.resolver_simbolo_nodo = codegen_resolver_desde_tabla;
    cfg.contexto_simbolos = &contexto;

    if (!codegen_generar_archivo(ast, "salida_codegen_integracion.asm", &cfg)) {
        fprintf(stderr, "FALLO: codegen integrado rechazo factorial.\n");
        return 1;
    }

    asm_txt = leer("salida_codegen_integracion.asm");
    if (!asm_txt) return 1;

#define EXIGIR(x) do { if (!strstr(asm_txt, (x))) { \
    fprintf(stderr, "FALLO: falta: %s\n", (x)); return 1; } } while (0)

    EXIGIR("convencion: x2=retorno, x14=SP, x15=FP");
    EXIGIR("func_factorial:");
    EXIGIR("STORE x15, 0(x14)");
    EXIGIR("LOAD x4, 8(x15)");
    EXIGIR("STORE x4, -4(x15)");
    EXIGIR("LOAD x3, 0(x15)");
    EXIGIR("LOAD x13, 4(x15)");
    EXIGIR("ADD x1, x13, x0");
    EXIGIR("ADD x2, x0");
    EXIGIR("error_pila:");
    EXIGIR("BRU: JMP func_factorial");
    EXIGIR("ret_call_");
    EXIGIR("scheduler RAW:");
    EXIGIR("barrera control x1: 1 NOP");

    if (!strstr(asm_txt,
                "ALU: ADD x1, x13, x0\n"
                "# barrera control x1: 1 NOP\n"
                "ALU: NOP\n")) {
        fprintf(stderr,
                "FALLO: RET por x1 no quedo protegido por un bundle NOP.\n");
        return 1;
    }

    if (strstr(asm_txt, "CALL") || strstr(asm_txt, "RET ") || strstr(asm_txt, "ALU: MUL")) {
        fprintf(stderr, "FALLO: aparecio una instruccion inexistente.\n");
        return 1;
    }

    printf("PASS: codegen usa la tabla de simbolos real.\n");
    printf("PASS: x14=SP, x15=FP, x2=retorno y offsets FP correctos.\n");
    printf("PASS: factorial ya usa un marco compatible con recursion.\n");
    printf("PASS: RET por x1 incluye barrera de control de 1 NOP.\n");

    free(asm_txt);
    ast_liberar(ast);
    ts_destruir(tabla);
    return 0;
}
