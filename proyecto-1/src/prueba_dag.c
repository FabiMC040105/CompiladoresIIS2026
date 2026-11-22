#include "dag.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int yylineno = 1; /* ast.c, sin Flex/Bison. */
#define EM(op,a,b,r) assert(ir_emit(&p,op,a,b,r,1))
#define EX(op,a,b,r,e) assert(ir_emit_ex(&p,op,a,b,r,e,1))
#define I(k) (*dag_buscar_instruccion(&g,k))
#define U(k,c) (I(k).usos[dag_indice_campo(c)])

static void verificar_dag(const DAGBloque *g)
{
    for (size_t n=0;n<g->cantidad_nodos;++n)
        for (size_t c=0;c<DAG_CAMPOS;++c)
            assert(g->nodos[n].operandos[c]==DAG_NULO || g->nodos[n].operandos[c]<n);
    for (size_t n=0;n<g->cantidad_instrucciones;++n) {
        const DAGInstruccion *i=&g->instrucciones[n];
        assert(i->nodo<g->cantidad_nodos);
        for (size_t c=0;c<DAG_CAMPOS;++c) {
            const DAGUso *u=&i->usos[c];
            assert(u->nodo==DAG_NULO || u->nodo<g->cantidad_nodos);
            if (u->definicion!=DAG_NULO) {
                assert(u->definicion<i->indice);
                const DAGInstruccion *d=dag_buscar_instruccion(g,u->definicion);
                assert(d && d->resultado==u->nodo);
            }
        }
    }
}

static void redefiniciones(void)
{
    IRProgram p; ir_init(&p); DAGBloque g;
    EM(IR_ADD,"b","c","a"); EM(IR_MUL,"a","e","d");
    EM(IR_ADD,"f","g","a"); EM(IR_SUB,"a","i","h");
    BasicBlock b={0,0,3};
    assert(dag_construir(&p,&b,&g)); verificar_dag(&g);
    assert(g.cantidad_nodos==10);
    assert(I(0).resultado!=I(2).resultado);
    assert(U(1,DEP_ARG1).definicion==0 && U(1,DEP_ARG1).nodo==I(0).resultado);
    assert(U(3,DEP_ARG1).definicion==2 && U(3,DEP_ARG1).nodo==I(2).resultado);
    assert(dag_buscar_nombre(&g,"a")->definicion==2);
    assert(!strcmp(p.instructions[0].result,"a") && p.count==4 && p.next_temp==0);
    puts("PASS: redefiniciones y usos exactos"); ir_print(&p,stdout); dag_imprimir(&g,stdout);
    dag_liberar(&g); ir_free(&p);
}

static void ejemplo_profesor(void)
{
    IRProgram p; ir_init(&p); DAGBloque g;
    EM(IR_ADD,"b","c","a"); EM(IR_SUB,"a","b","d");
    EM(IR_ADD,"d","c","e"); EM(IR_SUB,"e","d","f");
    BasicBlock b={0,0,3}; assert(dag_construir(&p,&b,&g)); verificar_dag(&g);
    assert(g.cantidad_nodos==6); /* Dos hojas y cuatro operadores. */
    assert(U(3,DEP_ARG1).nodo==I(2).resultado && U(3,DEP_ARG2).nodo==I(1).resultado);
    assert(U(1,DEP_ARG2).nodo==U(0,DEP_ARG1).nodo);
    assert(U(2,DEP_ARG2).nodo==U(0,DEP_ARG2).nodo);
    dag_liberar(&g); ir_free(&p); puts("PASS: ejemplo diapositiva 17");
}

static void copias_y_reuso(void)
{
    IRProgram p; ir_init(&p); DAGBloque g;
    EM(IR_ADD,"b","5","%t0"); EM(IR_COPY,"%t0",NULL,"a");
    EM(IR_ADD,"b","5","%t1"); EM(IR_COPY,"a",NULL,"d");
    EM(IR_ADD,"a","5","a"); EM(IR_SUB,"a","d","%t2");
    BasicBlock b={0,0,5}; assert(dag_construir(&p,&b,&g)); verificar_dag(&g);
    assert(I(0).resultado==I(1).resultado && I(0).resultado==I(2).resultado);
    assert(U(3,DEP_ARG1).definicion==1); /* No confundir copia ni expresion repetida. */
    assert(U(4,DEP_ARG1).definicion==1 && U(4,DEP_ARG2).definicion==DAG_NULO);
    assert(g.nodos[U(4,DEP_ARG2).nodo].tipo==DAG_CONSTANTE);
    assert(U(5,DEP_ARG1).definicion==4 && U(5,DEP_ARG2).definicion==3);
    assert(dag_buscar_nombre(&g,"a")->nodo!=dag_buscar_nombre(&g,"d")->nodo);
    assert(dag_buscar_nombre(&g,"b")->definicion==DAG_NULO);
    assert(p.count==6); /* No CSE ni otras transformaciones. */
    dag_liberar(&g); ir_free(&p); puts("PASS: copias, constantes, reuso estructural y x=x+1");
}

static void barreras(void)
{
    IRProgram p; ir_init(&p); DAGBloque g;
    EM(IR_COPY,"3",NULL,"a"); EM(IR_COPY,"a",NULL,"%t0");
    EM(IR_CALL,"foo","0","%t1"); EM(IR_ADD,"a","%t0","%t2");
    EM(IR_ARRAY_LOAD,"arr","1","%t3"); EM(IR_ARRAY_LOAD,"arr","1","%t4");
    EX(IR_MATRIX_STORE,"i","2","mat","%t3"); EX(IR_MATRIX_LOAD,"mat","i","%t5","2");
    EM(IR_DECL,"entero",NULL,"a"); EM(IR_COPY,"a",NULL,"%t6");
    EM(IR_DIV,"%t0","3","%t7"); EM(IR_DIV,"%t0","3","%t8");
    EM(IR_RETURN,"%t8",NULL,NULL);
    BasicBlock b={0,0,p.count-1}; assert(dag_construir(&p,&b,&g)); verificar_dag(&g);
    assert(U(3,DEP_ARG1).definicion==DAG_NULO && U(3,DEP_ARG1).nodo!=I(0).resultado);
    assert(U(3,DEP_ARG2).definicion==1 && U(3,DEP_ARG2).nodo==I(1).resultado);
    assert(I(2).accesos.barrera && U(2,DEP_ARG1).nodo==DAG_NULO && U(2,DEP_ARG2).nodo==DAG_NULO);
    assert(I(4).resultado!=I(5).resultado && g.nodos[I(4).resultado].tipo==DAG_OPACA);
    assert(U(6,DEP_EXTRA).definicion==4 && U(6,DEP_RESULT).nodo!=DAG_NULO);
    assert(I(6).resultado==DAG_NULO && I(6).accesos.escribe_memoria);
    assert(U(7,DEP_ARG1).nodo!=U(6,DEP_RESULT).nodo);
    assert(U(9,DEP_ARG1).definicion==DAG_NULO);
    assert(I(10).resultado!=I(11).resultado && I(10).accesos.barrera);
    assert(U(12,DEP_ARG1).definicion==11 && I(12).resultado==DAG_NULO);
    dag_liberar(&g); ir_free(&p); puts("PASS: llamadas, memoria, dimensiones, declaracion, division, retorno");
}

static void bloques(void)
{
    IRProgram p; ir_init(&p); DAGBloque g;
    EM(IR_COPY,"1",NULL,"a"); EM(IR_IF_TRUE,"a",NULL,"@L0");
    EM(IR_COPY,"2",NULL,"a"); EM(IR_LABEL,NULL,NULL,"@L0");
    EM(IR_ADD,"a","1","%t0"); EM(IR_RETURN,"%t0",NULL,NULL);
    BasicBlockCFG *cfg=bb_build(&p); assert(cfg&&cfg->block_count==3&&bb_build_cfg(cfg));
    assert(dag_construir(&p,&cfg->blocks[0],&g)); verificar_dag(&g);
    assert(U(1,DEP_ARG1).definicion==0 && U(1,DEP_RESULT).nodo==DAG_NULO);
    dag_liberar(&g);
    assert(dag_construir(&p,&cfg->blocks[2],&g)); verificar_dag(&g);
    assert(g.bloque.start==3 && U(4,DEP_ARG1).definicion==DAG_NULO);
    assert(U(5,DEP_ARG1).definicion==4 && !dag_buscar_instruccion(&g,2));
    assert(!dag_buscar_nombre(&g,"@L0"));
    dag_liberar(&g); bb_free(cfg); ir_free(&p); puts("PASS: CFG, indices absolutos y valores de entrada por bloque");
}

static void agregar_asignacion(NodoAST *bloque,const char *dest,const char *a,const char *op,const char *b)
{
    NodoAST *asig=ast_crear(AST_ASIGNACION,NULL),*expr=ast_crear(AST_OPERACION,op);
    ast_agregar_hijo(asig,ast_crear(AST_VARIABLE,dest));
    ast_agregar_hijo(expr,ast_crear(AST_VARIABLE,a)); ast_agregar_hijo(expr,ast_crear(AST_VARIABLE,b));
    ast_agregar_hijo(asig,expr); ast_agregar_hijo(bloque,asig);
}

static void integracion(void)
{
    IRProgram p; ir_init(&p); DAGBloque g; IRError err;
    NodoAST *raiz=ast_crear(AST_PROGRAMA,NULL),*principal=ast_crear(AST_PRINCIPAL,NULL),*b=ast_crear(AST_BLOQUE,NULL);
    ast_agregar_hijo(raiz,principal); ast_agregar_hijo(principal,b);
    agregar_asignacion(b,"a","b","+","c"); agregar_asignacion(b,"d","a","*","e");
    agregar_asignacion(b,"a","f","+","g");
    assert(ir_generate(raiz,&p,&err)&&p.count==8);
    BasicBlockCFG *cfg=bb_build(&p); assert(cfg&&cfg->block_count==1&&bb_build_cfg(cfg));
    assert(dag_construir(&p,&cfg->blocks[0],&g)); verificar_dag(&g);
    assert(I(1).resultado==I(2).resultado && U(3,DEP_ARG1).definicion==2);
    assert(dag_buscar_nombre(&g,"a")->definicion==6 && I(2).resultado!=I(6).resultado);
    assert(p.count==8 && p.next_temp==3 && !strcmp(p.instructions[2].result,"a"));
    dag_liberar(&g); bb_free(cfg); ir_free(&p); ast_liberar(raiz);
    puts("PASS: AST -> IR real -> CFG -> DAG, sin modificar IR");
}

static void capacidad_y_errores(void)
{
    IRProgram p; ir_init(&p); DAGBloque g;
    for (int k=0;k<100;++k) {
        char dest[32],val[32]; snprintf(dest,sizeof dest,"a%d",k); snprintf(val,sizeof val,"%d",k);
        EM(IR_ADD,"x",val,dest);
    }
    BasicBlock b={0,0,99}; assert(dag_construir(&p,&b,&g)); verificar_dag(&g);
    assert(g.cantidad_nodos==201 && dag_buscar_nombre(&g,"a99")->definicion==99);
    dag_liberar(&g); dag_liberar(&g);
    b.end=100; assert(!dag_construir(&p,&b,&g)&&!g.nodos);
    EM(IR_ADD,"a",NULL,"b"); assert(!dag_construir(&p,&b,&g)&&!g.nodos&&!g.instrucciones&&!g.nombres);
    assert(!dag_construir(NULL,&b,&g)); assert(!dag_construir(&p,&b,NULL));
    ir_free(&p); puts("PASS: crecimiento dinamico, indices invalidos y limpieza al fallar");
}

int main(void)
{
    redefiniciones(); ejemplo_profesor(); copias_y_reuso(); barreras(); bloques(); integracion(); capacidad_y_errores();
    puts("PASS: todas las pruebas DAG"); return 0;
}
