/* Prueba del backend IR sin Flex/Bison ni repositorio RTL.
   Construye los cuadruplos que genera ir_ast.c para ciclos y llamadas. */
#include "../codegen_ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylineno = 1;
static const char *caso_actual;

static int simbolo(void *ctx, const NodoAST *nodo, const char *funcion,
                   const char *nombre, CodegenSimbolo *s) {
    (void)ctx; (void)nodo;
    memset(s, 0, sizeof(*s));
    s->nombre = nombre; s->forma = CODEGEN_FORMA_ESCALAR;
    if (!strcmp(nombre,"obtener")) {
        s->categoria = CODEGEN_CAT_FUNCION; s->ubicacion = CODEGEN_UBICACION_FUNCION;
        s->tamano_parametros = 4; s->tamano_marco = 0; return 1;
    }
    if (!strcmp(nombre,"arr")) {
        s->categoria = CODEGEN_CAT_VARIABLE; s->ubicacion = CODEGEN_UBICACION_GLOBAL;
        s->forma = CODEGEN_FORMA_LISTA; s->cantidad_dimensiones = 1;
        s->dimensiones[0] = 4; s->direccion=0; return 1;
    }
    if (funcion && !strcmp(funcion,"obtener") && !strcmp(nombre,"lista")) {
        s->categoria = CODEGEN_CAT_PARAMETRO;
        s->ubicacion = CODEGEN_UBICACION_PARAMETRO;
        s->forma = CODEGEN_FORMA_LISTA; s->cantidad_dimensiones = 1;
        s->dimensiones[0]=4; s->direccion = 8;
        s->parametro_por_referencia=1; return 1;
    }
    if (!strcmp(nombre,"factorial")) {
        s->categoria = CODEGEN_CAT_FUNCION; s->ubicacion = CODEGEN_UBICACION_FUNCION;
        s->tamano_parametros = 4; s->tamano_marco = 8; return 1;
    }
    if (!strcmp(nombre,"principal")) {
        s->categoria = CODEGEN_CAT_FUNCION; s->ubicacion = CODEGEN_UBICACION_FUNCION;
        s->tamano_marco = 8; return 1;
    }
    if (!funcion) return 0;
    if (!strcmp(funcion,"factorial")) {
        if (!strcmp(nombre,"n")) {
            s->categoria = CODEGEN_CAT_PARAMETRO;
            s->ubicacion = CODEGEN_UBICACION_PARAMETRO;
            s->direccion = 8; return 1;
        }
        if (!strcmp(nombre,"resultado")) s->direccion = -4;
        else if (!strcmp(nombre,"i")) s->direccion = -8;
        else return 0;
    } else if (!strcmp(funcion,"principal")) {
        if (!strcmp(nombre,"n")) s->direccion = -4;
        else if (!strcmp(nombre,"resultado")) s->direccion =
            (!strcmp(caso_actual,"para") || !strcmp(caso_actual,"ast_para") ||
             !strcmp(caso_actual,"mientras") ||
             !strcmp(caso_actual,"condicional")) ? -4 : -8;
        else if (!strcmp(nombre,"i")) s->direccion = -8;
        else if (!strcmp(nombre,"contador")) s->direccion = -8;
        else return 0;
    } else return 0;
    s->categoria = CODEGEN_CAT_VARIABLE; s->ubicacion = CODEGEN_UBICACION_LOCAL;
    return 1;
}

static int add(IRProgram *p, IROp op, const char *a, const char *b, const char *r) {
    return ir_emit(p, op, a, b, r, 1);
}
#define EM(OP,A,B,R) do { if (!add(&p, (OP), (A), (B), (R))) return 2; } while (0)

/* Construye AST de prueba para ejercitar el generador AST -> IR real. */
static NodoAST *node(TipoNodo tipo, const char *valor) {
    return ast_crear(tipo, valor);
}
static NodoAST *child2(TipoNodo tipo, const char *valor,
                       NodoAST *a, NodoAST *b) {
    NodoAST *n = node(tipo,valor);
    if (a) ast_agregar_hijo(n,a);
    if (b) ast_agregar_hijo(n,b);
    return n;
}
static NodoAST *op(const char *simbolo, NodoAST *a, NodoAST *b) {
    return child2(AST_OPERACION,simbolo,a,b);
}
static NodoAST *decl(const char *name, NodoAST *value) {
    return child2(AST_DECLARACION,"entero",node(AST_VARIABLE,name),value);
}
static NodoAST *assign(const char *name, NodoAST *value) {
    return child2(AST_ASIGNACION,NULL,node(AST_VARIABLE,name),value);
}
static NodoAST *ast_for_test(void) {
    NodoAST *root=node(AST_PROGRAMA,NULL);
    NodoAST *main=node(AST_PRINCIPAL,NULL);
    NodoAST *body=node(AST_BLOQUE,NULL);
    NodoAST *loop=node(AST_FOR,NULL);
    ast_agregar_hijo(body,decl("resultado",node(AST_NUMERO,"0")));
    ast_agregar_hijo(loop,decl("i",node(AST_NUMERO,"0")));
    ast_agregar_hijo(loop,op("<",node(AST_VARIABLE,"i"),node(AST_NUMERO,"4")));
    ast_agregar_hijo(loop,assign("i",op("+",node(AST_VARIABLE,"i"),node(AST_NUMERO,"1"))));
    NodoAST *inner=node(AST_BLOQUE,NULL);
    ast_agregar_hijo(inner,assign("resultado",op("+",node(AST_VARIABLE,"resultado"),node(AST_NUMERO,"2"))));
    ast_agregar_hijo(loop,inner);
    ast_agregar_hijo(body,loop);
    ast_agregar_hijo(main,body); ast_agregar_hijo(root,main);
    return root;
}
static NodoAST *ast_factorial_test(void) {
    NodoAST *root=node(AST_PROGRAMA,NULL);
    NodoAST *func=node(AST_FUNCION,NULL);
    NodoAST *params=node(AST_PARAMETROS,NULL);
    NodoAST *param=node(AST_PARAMETRO,"entero");
    NodoAST *body=node(AST_BLOQUE,NULL);
    ast_agregar_hijo(func,node(AST_VARIABLE,"factorial"));
    ast_agregar_hijo(param,node(AST_VARIABLE,"n"));
    ast_agregar_hijo(params,param); ast_agregar_hijo(func,params);
    ast_agregar_hijo(body,decl("resultado",node(AST_NUMERO,"1")));
    ast_agregar_hijo(body,decl("i",node(AST_NUMERO,"1")));
    NodoAST *cond=op("==",node(AST_VARIABLE,"n"),node(AST_NUMERO,"0"));
    NodoAST *then=node(AST_BLOQUE,NULL);
    ast_agregar_hijo(then,child2(AST_RETURN,NULL,node(AST_NUMERO,"1"),NULL));
    ast_agregar_hijo(body,child2(AST_IF,NULL,cond,then));
    NodoAST *whilebody=node(AST_BLOQUE,NULL);
    ast_agregar_hijo(whilebody,assign("resultado",op("*",node(AST_VARIABLE,"resultado"),node(AST_VARIABLE,"i"))));
    ast_agregar_hijo(whilebody,assign("i",op("+",node(AST_VARIABLE,"i"),node(AST_NUMERO,"1"))));
    ast_agregar_hijo(body,child2(AST_WHILE,NULL,
              op("<=",node(AST_VARIABLE,"i"),node(AST_VARIABLE,"n")),whilebody));
    ast_agregar_hijo(body,child2(AST_RETURN,NULL,node(AST_VARIABLE,"resultado"),NULL));
    ast_agregar_hijo(func,body); ast_agregar_hijo(root,func);
    NodoAST *main=node(AST_PRINCIPAL,NULL);
    NodoAST *mainbody=node(AST_BLOQUE,NULL);
    ast_agregar_hijo(mainbody,decl("n",node(AST_NUMERO,"5")));
    NodoAST *call=node(AST_LLAMADA,"factorial");
    ast_agregar_hijo(call,node(AST_VARIABLE,"n"));
    ast_agregar_hijo(mainbody,decl("resultado",call));
    ast_agregar_hijo(main,mainbody); ast_agregar_hijo(root,main);
    return root;
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr,"usage: %s caso salida.asm\n",argv[0]); return 2; }
    caso_actual = argv[1];
    IRProgram p; ir_init(&p);
    NodoAST *raiz=NULL;
    if (!strcmp(argv[1],"ast_para") || !strcmp(argv[1],"ast_factorial")) {
        IRError error={0};
        raiz = !strcmp(argv[1],"ast_para") ? ast_for_test() : ast_factorial_test();
        if (!ir_generate(raiz,&p,&error)) {
            fprintf(stderr,"Falló AST -> IR linea %d: %s\n",error.line,error.message);
            ast_liberar(raiz);
            return 2;
        }
    } else if (!strcmp(argv[1],"para")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_DECL,"entero",NULL,"resultado"); EM(IR_COPY,"0",NULL,"resultado");
        EM(IR_DECL,"entero",NULL,"i"); EM(IR_COPY,"0",NULL,"i");
        EM(IR_LABEL,NULL,NULL,"L0");
        EM(IR_LT,"i","4","%t0"); EM(IR_IF_FALSE,"%t0",NULL,"L1");
        EM(IR_ADD,"resultado","2","%t1"); EM(IR_COPY,"%t1",NULL,"resultado");
        EM(IR_ADD,"i","1","%t2"); EM(IR_COPY,"%t2",NULL,"i");
        EM(IR_GOTO,NULL,NULL,"L0"); EM(IR_LABEL,NULL,NULL,"L1");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"mientras")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_DECL,"entero",NULL,"resultado"); EM(IR_COPY,"0",NULL,"resultado");
        EM(IR_DECL,"entero",NULL,"contador"); EM(IR_COPY,"3",NULL,"contador");
        EM(IR_LABEL,NULL,NULL,"L0");
        EM(IR_GT,"contador","0","%t0"); EM(IR_IF_FALSE,"%t0",NULL,"L1");
        EM(IR_ADD,"resultado","contador","%t1"); EM(IR_COPY,"%t1",NULL,"resultado");
        EM(IR_SUB,"contador","1","%t2"); EM(IR_COPY,"%t2",NULL,"contador");
        EM(IR_GOTO,NULL,NULL,"L0"); EM(IR_LABEL,NULL,NULL,"L1");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"condicional")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_DECL,"entero",NULL,"resultado"); EM(IR_COPY,"10",NULL,"resultado");
        EM(IR_EQ,"resultado","10","%t0"); EM(IR_IF_FALSE,"%t0",NULL,"L0");
        EM(IR_ADD,"resultado","32","%t1"); EM(IR_COPY,"%t1",NULL,"resultado");
        EM(IR_LABEL,NULL,NULL,"L0");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"factorial")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"factorial");
        EM(IR_PARAM_DECL,"entero",NULL,"n");
        EM(IR_DECL,"entero",NULL,"resultado"); EM(IR_COPY,"1",NULL,"resultado");
        EM(IR_DECL,"entero",NULL,"i"); EM(IR_COPY,"1",NULL,"i");
        EM(IR_EQ,"n","0","%t0"); EM(IR_IF_FALSE,"%t0",NULL,"L0");
        EM(IR_RETURN,"1",NULL,NULL); EM(IR_LABEL,NULL,NULL,"L0");
        EM(IR_LABEL,NULL,NULL,"L1"); EM(IR_LE,"i","n","%t1");
        EM(IR_IF_FALSE,"%t1",NULL,"L2");
        EM(IR_MUL,"resultado","i","%t2"); EM(IR_COPY,"%t2",NULL,"resultado");
        EM(IR_ADD,"i","1","%t3"); EM(IR_COPY,"%t3",NULL,"i");
        EM(IR_GOTO,NULL,NULL,"L1"); EM(IR_LABEL,NULL,NULL,"L2");
        EM(IR_RETURN,"resultado",NULL,NULL); EM(IR_FUNC_END,NULL,NULL,"factorial");
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_DECL,"entero",NULL,"n"); EM(IR_COPY,"5",NULL,"n");
        EM(IR_DECL,"entero",NULL,"resultado");
        EM(IR_COPY,"n",NULL,"%t4"); EM(IR_PARAM,"%t4",NULL,NULL);
        EM(IR_CALL,"factorial","1","%t5"); EM(IR_COPY,"%t5",NULL,"resultado");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"factorial_recursivo")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"factorial");
        EM(IR_PARAM_DECL,"entero",NULL,"n");
        EM(IR_LE,"n","1","%t0"); EM(IR_IF_FALSE,"%t0",NULL,"Lrec");
        EM(IR_RETURN,"1",NULL,NULL); EM(IR_LABEL,NULL,NULL,"Lrec");
        EM(IR_SUB,"n","1","%t1"); EM(IR_PARAM,"%t1",NULL,NULL);
        EM(IR_CALL,"factorial","1","%t2");
        EM(IR_MUL,"n","%t2","%t3"); EM(IR_RETURN,"%t3",NULL,NULL);
        EM(IR_FUNC_END,NULL,NULL,"factorial");
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_DECL,"entero",NULL,"n"); EM(IR_COPY,"5",NULL,"n");
        EM(IR_PARAM,"n",NULL,NULL);
        EM(IR_CALL,"factorial","1","%t4"); EM(IR_COPY,"%t4",NULL,"resultado");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"lista_parametro")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"obtener");
        EM(IR_PARAM_DECL,"entero","4","lista");
        EM(IR_ARRAY_LOAD,"lista","2","%t1");
        EM(IR_RETURN,"%t1",NULL,NULL);
        EM(IR_FUNC_END,NULL,NULL,"obtener");
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_ARRAY_DECL,"4",NULL,"arr");
        EM(IR_ARRAY_STORE,"2","17","arr");
        EM(IR_COPY,"arr",NULL,"%t0");
        EM(IR_PARAM,"%t0",NULL,NULL);
        EM(IR_CALL,"obtener","1","%t2");
        EM(IR_COPY,"%t2",NULL,"resultado");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"aritmetica")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_DIV,"-17","5","%t0"); EM(IR_MOD,"-17","5","%t1");
        EM(IR_POW,"2","5","%t2"); EM(IR_MUL,"-3","7","%t3");
        EM(IR_ADD,"%t0","%t1","%t4"); EM(IR_ADD,"%t4","%t2","%t5");
        EM(IR_ADD,"%t5","%t3","%t6"); EM(IR_COPY,"%t6",NULL,"resultado");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else if (!strcmp(argv[1],"comparaciones")) {
        EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
        EM(IR_LT,"0","4","%t0"); EM(IR_GT,"0","4","%t1");
        EM(IR_LE,"4","4","%t2"); EM(IR_GE,"3","4","%t3");
        EM(IR_EQ,"3","3","%t4"); EM(IR_NE,"3","3","%t5");
        EM(IR_ADD,"%t0","%t1","%t6"); EM(IR_ADD,"%t6","%t2","%t7");
        EM(IR_ADD,"%t7","%t3","%t8"); EM(IR_ADD,"%t8","%t4","%t9");
        EM(IR_ADD,"%t9","%t5","%t10"); EM(IR_COPY,"%t10",NULL,"resultado");
        EM(IR_FUNC_END,NULL,NULL,"principal");
    } else { fprintf(stderr,"Unknown test\n"); return 2; }
    CodegenIRConfig cfg = {0}; cfg.original.fin_globales =
        !strcmp(caso_actual,"lista_parametro") ? 16 : 0;
    cfg.resolver_ir = simbolo;
    CodegenIREstadisticas stats={0};
    int ok=codegen_ir_generar_archivo(&p,argv[2],&cfg,&stats);
    ir_free(&p);
    ast_liberar(raiz);
    if(!ok) { fprintf(stderr,"FAIL backend %s\n",argv[1]); return 1; }
    printf("PASS codigo %s: %zu instrucciones IR, %zu temporales\n",
           argv[1],stats.instrucciones_ir,stats.temporales);
    return 0;
}
