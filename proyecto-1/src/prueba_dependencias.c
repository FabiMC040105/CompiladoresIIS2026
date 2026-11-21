#include "dependencias.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* El lexer no participa en esta prueba; ast.c usa yylineno. */
int yylineno = 1;

#define A DEP_ARG1
#define B DEP_ARG2
#define R DEP_RESULT
#define E DEP_EXTRA

static void clasificar_todas(void)
{
    struct Caso { IRInstruction inst; unsigned r,w,d; int lm,em,bar; } casos[] = {
        {{IR_DECL,"entero",NULL,"x",NULL,0},0,0,R,0,0,1},
        {{IR_COPY,"a",NULL,"x",NULL,0},A,R,0,0,0,0},
        {{IR_ADD,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_SUB,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_MUL,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_DIV,"a","b","x",NULL,0},A|B,R,0,0,0,1},
        {{IR_MOD,"a","b","x",NULL,0},A|B,R,0,0,0,1},
        {{IR_POW,"a","b","x",NULL,0},A|B,R,0,0,0,1},
        {{IR_NEG,"a",NULL,"x",NULL,0},A,R,0,0,0,0},
        {{IR_EQ,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_NE,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_LT,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_LE,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_GT,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_GE,"a","b","x",NULL,0},A|B,R,0,0,0,0},
        {{IR_LABEL,NULL,NULL,"@L0",NULL,0},0,0,0,0,0,1},
        {{IR_GOTO,NULL,NULL,"@L0",NULL,0},0,0,0,0,0,1},
        {{IR_IF_FALSE,"cond",NULL,"@L0",NULL,0},A,0,0,0,0,1},
        {{IR_IF_TRUE,"cond",NULL,"@L0",NULL,0},A,0,0,0,0,1},
        {{IR_FUNC_BEGIN,NULL,NULL,"foo",NULL,0},0,0,0,0,0,1},
        {{IR_FUNC_END,NULL,NULL,"foo",NULL,0},0,0,0,0,0,1},
        {{IR_PARAM_DECL,"matriz","n","x","m",0},B|E,R,R,0,0,1},
        {{IR_PARAM,"a",NULL,NULL,NULL,0},A,0,0,0,0,1},
        {{IR_CALL,"foo","2","%t1",NULL,0},0,R,0,1,1,1},
        {{IR_RETURN,"a",NULL,NULL,NULL,0},A,0,0,0,0,1},
        {{IR_ARRAY_DECL,"n",NULL,"arr",NULL,0},A,0,R,0,1,1},
        {{IR_ARRAY_LOAD,"arr","i","%t0",NULL,0},A|B,R,0,1,0,1},
        {{IR_ARRAY_STORE,"i","v","arr",NULL,0},A|B|R,0,0,0,1,1},
        {{IR_MATRIX_DECL,"n","m","mat",NULL,0},A|B,0,R,0,1,1},
        {{IR_MATRIX_LOAD,"mat","i","%t0","j",0},A|B|E,R,0,1,0,1},
        {{IR_MATRIX_STORE,"i","j","mat","v",0},A|B|R|E,0,0,0,1,1},
    };
    assert(sizeof(casos)/sizeof(casos[0]) == IR_MATRIX_STORE + 1u);
    for (size_t k=0; k<sizeof(casos)/sizeof(casos[0]); ++k) {
        DepAccesos d;
        assert((size_t)casos[k].inst.op == k);
        assert(dep_accesos(&casos[k].inst,&d));
        assert(d.lecturas==casos[k].r && d.escrituras==casos[k].w && d.declaracion==casos[k].d);
        assert(d.lee_memoria==casos[k].lm && d.escribe_memoria==casos[k].em && d.barrera==casos[k].bar);
    }
    puts("PASS: 31 IROp clasificados");
}

static int existe(const DepGrafo *g,size_t i,size_t j,DepTipo t,const char *n)
{
    for(size_t k=0;k<g->cantidad;++k) {
        const DepArista *a=&g->aristas[k];
        if(a->origen==i && a->destino==j && a->tipo==t &&
           ((!n&&!a->nombre)||(n&&a->nombre&&!strcmp(n,a->nombre)))) return 1;
    }
    return 0;
}

static void dependencias_directas(void)
{
    IRProgram p; ir_init(&p); DepGrafo g;
    assert(ir_emit(&p,IR_ADD,"b","c","a",1));
    assert(ir_emit(&p,IR_MUL,"a","a","d",2));
    assert(ir_emit(&p,IR_ADD,"f","g","a",3));
    assert(ir_emit(&p,IR_SUB,"a","i","h",4));
    BasicBlockCFG *cfg=bb_build(&p);
    assert(cfg && cfg->block_count==1 && bb_build_cfg(cfg));
    assert(dep_analizar_bloque(&p,&cfg->blocks[0],&g));
    assert(g.cantidad==4);
    assert(existe(&g,0,1,DEP_RAW,"a"));
    assert(existe(&g,0,2,DEP_WAW,"a"));
    assert(existe(&g,1,2,DEP_WAR,"a"));
    assert(existe(&g,2,3,DEP_RAW,"a"));
    assert(!existe(&g,0,3,DEP_RAW,"a")); /* Redefinicion: no alcanza. */
    puts("PASS: RAW/WAR/WAW, sin RAW obsoleta ni duplicados");
    ir_print(&p,stdout); dep_imprimir(&g,stdout);
    dep_liberar(&g); bb_free(cfg); ir_free(&p);
}

static void pares_y_constantes(void)
{
    IRInstruction a={IR_ADD,"x","1","x",NULL,0};
    IRInstruction b={IR_ADD,"x","2","x",NULL,0};
    IRInstruction c={IR_ADD,"x","-1","%t4",NULL,0};
    DepAccesos d;
    assert(dep_tipos_par(&a,&b)==(DEP_RAW|DEP_WAR|DEP_WAW));
    assert(dep_accesos(&c,&d)&&d.lecturas==A&&d.escrituras==R);
    IRInstruction call={IR_CALL,"foo","2",NULL,NULL,0};
    assert(dep_accesos(&call,&d)&&!d.lecturas&&!d.escrituras&&d.barrera);
    IRInstruction ret={IR_RETURN,NULL,NULL,NULL,NULL,0};
    assert(dep_accesos(&ret,&d)&&!d.lecturas);
    IRInstruction p={IR_PARAM_DECL,"entero",NULL,"x",NULL,0};
    assert(dep_accesos(&p,&d)&&!d.lecturas&&d.escrituras==R);
    assert(!dep_es_nombre("@L0")&&!dep_es_nombre("3")&&!dep_es_nombre("verdadero"));
    assert(dep_es_nombre("a_2")&&dep_es_nombre("%t34"));
    IRInstruction read1={IR_ADD,"x","1","a",NULL,0};
    IRInstruction read2={IR_ADD,"x","2","b",NULL,0};
    assert(!dep_tipos_par(&read1,&read2)); /* RAR */
    puts("PASS: multiples riesgos, RAR, constantes y operandos opcionales");
}

static void memoria_y_control(void)
{
    IRProgram p; ir_init(&p); DepGrafo g;
    assert(ir_emit(&p,IR_COPY,"1",NULL,"i",1));
    assert(ir_emit_ex(&p,IR_MATRIX_STORE,"i","j","mat","v",2));
    assert(ir_emit(&p,IR_ARRAY_LOAD,"arr","k","%t0",3));
    assert(ir_emit(&p,IR_CALL,"foo","0",NULL,4));
    assert(ir_emit(&p,IR_IF_TRUE,"%t0",NULL,"@L0",5));
    assert(ir_emit(&p,IR_COPY,"2",NULL,"i",6));
    assert(ir_emit(&p,IR_LABEL,NULL,NULL,"@L0",7));
    assert(ir_emit(&p,IR_RETURN,"i",NULL,NULL,8));
    BasicBlockCFG *cfg=bb_build(&p);
    assert(cfg && cfg->block_count==3 && bb_build_cfg(cfg));
    assert(dep_analizar_bloque(&p,&cfg->blocks[0],&g));
    assert(existe(&g,0,1,DEP_RAW,"i"));
    assert(existe(&g,1,2,DEP_MEMORIA,NULL)); /* Alias conservador incluso con nombres distintos. */
    assert(existe(&g,2,3,DEP_MEMORIA,NULL));
    assert(existe(&g,0,3,DEP_BARRERA,NULL));
    assert(existe(&g,2,4,DEP_RAW,"%t0"));
    assert(existe(&g,2,4,DEP_BARRERA,NULL));
    for(size_t k=0;k<g.cantidad;++k) assert(g.aristas[k].destino<5 && g.aristas[k].origen<g.aristas[k].destino);
    dep_liberar(&g);
    assert(dep_analizar_bloque(&p,&cfg->blocks[1],&g)&&g.cantidad==0);
    dep_liberar(&g); bb_free(cfg); ir_free(&p);
    puts("PASS: memoria, llamadas, saltos y limites de bloque");
}

static void redefiniciones(void)
{
    IRProgram p; ir_init(&p); DepGrafo g;
    assert(ir_emit(&p,IR_COPY,"x",NULL,"a",1));
    assert(ir_emit(&p,IR_COPY,"x",NULL,"b",2));
    assert(ir_emit(&p,IR_ADD,"x","1","x",3));
    assert(ir_emit(&p,IR_ADD,"x","2","x",4));
    assert(ir_emit(&p,IR_COPY,"x",NULL,"c",5));
    BasicBlock b={0,0,4};
    assert(dep_analizar_bloque(&p,&b,&g));
    assert(g.cantidad==5);
    assert(existe(&g,0,2,DEP_WAR,"x")&&existe(&g,1,2,DEP_WAR,"x"));
    assert(existe(&g,2,3,DEP_RAW,"x")&&existe(&g,2,3,DEP_WAW,"x"));
    assert(existe(&g,3,4,DEP_RAW,"x"));
    assert(!existe(&g,0,3,DEP_WAR,"x")&&!existe(&g,2,4,DEP_RAW,"x"));
    dep_liberar(&g); ir_free(&p);
    puts("PASS: WAR de valor de entrada y redefiniciones consecutivas");
}

static void agregar_asignacion(NodoAST *bloque,const char *dest,const char *a,const char *op,const char *b)
{
    NodoAST *asignacion=ast_crear(AST_ASIGNACION,NULL);
    NodoAST *expr=ast_crear(AST_OPERACION,op);
    ast_agregar_hijo(asignacion,ast_crear(AST_VARIABLE,dest));
    ast_agregar_hijo(expr,ast_crear(AST_VARIABLE,a));
    ast_agregar_hijo(expr,ast_crear(AST_VARIABLE,b));
    ast_agregar_hijo(asignacion,expr);
    ast_agregar_hijo(bloque,asignacion);
}

static void integracion_ast_ir(void)
{
    /* Construye un AST directamente, sin Flex/Bison, y llama al generador real. */
    NodoAST *raiz=ast_crear(AST_PROGRAMA,NULL);
    NodoAST *principal=ast_crear(AST_PRINCIPAL,NULL);
    NodoAST *bloque=ast_crear(AST_BLOQUE,NULL);
    ast_agregar_hijo(raiz,principal); ast_agregar_hijo(principal,bloque);
    agregar_asignacion(bloque,"a","b","+","c");
    agregar_asignacion(bloque,"d","a","*","e");
    agregar_asignacion(bloque,"a","f","+","g");
    IRProgram p; IRError err; DepGrafo g; ir_init(&p);
    assert(ir_generate(raiz,&p,&err)&&p.count==8);
    BasicBlockCFG *cfg=bb_build(&p);
    assert(cfg&&cfg->block_count==1&&bb_build_cfg(cfg));
    assert(dep_analizar_bloque(&p,&cfg->blocks[0],&g));
    assert(existe(&g,1,2,DEP_RAW,"%t0"));
    assert(existe(&g,2,3,DEP_RAW,"a"));
    assert(existe(&g,2,6,DEP_WAW,"a"));
    assert(existe(&g,3,6,DEP_WAR,"a"));
    assert(!strcmp(p.instructions[2].result,"a")&&p.next_temp==3);
    dep_liberar(&g); bb_free(cfg); ir_free(&p); ast_liberar(raiz);
    puts("PASS: AST construido -> ir_generate real -> CFG -> dependencias");
}

static void errores(void)
{
    DepAccesos d; DepGrafo g; IRProgram p; ir_init(&p);
    IRInstruction invalid={(IROp)999,NULL,NULL,NULL,NULL,0};
    assert(!dep_accesos(&invalid,&d));
    assert(!dep_accesos(NULL,&d));
    invalid=(IRInstruction){IR_ADD,"a",NULL,"x",NULL,0};
    assert(!dep_accesos(&invalid,&d));
    assert(ir_emit(&p,(IROp)999,NULL,NULL,NULL,1));
    BasicBlock b={0,0,0};
    assert(!dep_analizar_bloque(&p,&b,&g)&&!g.aristas&&!g.cantidad);
    dep_liberar(&g); b.end=1;
    assert(!dep_analizar_bloque(&p,&b,&g)); ir_free(&p);
    puts("PASS: errores e IR invalida");
}

int main(void)
{
    clasificar_todas(); dependencias_directas(); pares_y_constantes();
    memoria_y_control(); redefiniciones(); integracion_ast_ir(); errores();
    puts("PASS: todas las pruebas de dependencias"); return 0;
}
