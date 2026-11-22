#include "renombramiento.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylineno=1;
#define EM(op,a,b,r) assert(ir_emit(&p,op,a,b,r,1))
#define EX(op,a,b,r,e) assert(ir_emit_ex(&p,op,a,b,r,e,1))
#define I(k) p.instructions[k]
#define EQ(a,b) assert((a) && (b) && !strcmp((a),(b)))

/* Oraculo pequeno e independiente para IR escalar entera y saltos.
   No es simulador ISA/VLIW. Las pruebas de barreras usan aserciones estructurales. */
typedef struct { const char *nombres[128]; long valores[128]; size_t n; } Estado;
static void guardar(Estado *e,const char *n,long v)
{
    size_t k=0; while(k<e->n && strcmp(e->nombres[k],n)) ++k;
    if(k==e->n) { assert(e->n<128); e->nombres[e->n++]=n; }
    e->valores[k]=v;
}
static long leer(const Estado *e,const char *n)
{
    char *fin; assert(n); long v=strtol(n,&fin,10);
    if (*n && !*fin) return v;
    for(size_t k=0;k<e->n;++k) if(!strcmp(e->nombres[k],n)) return e->valores[k];
    fprintf(stderr,"Lectura sin inicializar: %s\n",n); abort();
}
static Estado ejecutar(const IRProgram *p)
{
    Estado e={0}; size_t pc=0,pasos=0;
    while(pc<p->count) {
        assert(++pasos<1000); const IRInstruction *i=&p->instructions[pc++]; long v=0;
        switch(i->op) {
            case IR_FUNC_BEGIN: case IR_FUNC_END: case IR_LABEL: case IR_DECL: continue;
            case IR_COPY: v=leer(&e,i->arg1); break;
            case IR_ADD: v=leer(&e,i->arg1)+leer(&e,i->arg2); break;
            case IR_SUB: v=leer(&e,i->arg1)-leer(&e,i->arg2); break;
            case IR_MUL: v=leer(&e,i->arg1)*leer(&e,i->arg2); break;
            case IR_LT: v=leer(&e,i->arg1)<leer(&e,i->arg2); break;
            case IR_IF_TRUE: case IR_IF_FALSE: case IR_GOTO:
                if(i->op==IR_IF_TRUE && !leer(&e,i->arg1)) continue;
                if(i->op==IR_IF_FALSE && leer(&e,i->arg1)) continue;
                for(pc=0;pc<p->count;++pc)
                    if(p->instructions[pc].op==IR_LABEL && !strcmp(p->instructions[pc].result,i->result)) break;
                assert(pc<p->count); continue;
            case IR_RETURN: if(i->arg1) guardar(&e,"#retorno",leer(&e,i->arg1)); return e;
            default: assert(0 && "opcode no soportado por oraculo de prueba");
        }
        guardar(&e,i->result,v);
    }
    return e;
}
static RenEstadisticas comprobar(IRProgram *p,int equivalencia)
{
    IRProgram original; ir_init(&original);
    for(size_t k=0;k<p->count;++k) {
        IRInstruction *i=&p->instructions[k];
        assert(ir_emit_ex(&original,i->op,i->arg1,i->arg2,i->result,i->extra,i->source_line));
    }
    RenEstadisticas s; size_t count=p->count, labels=p->next_label;
    assert(renombrar_programa(p,&s));
    assert(p->count==count && p->next_label==labels && s.antes.raw==s.despues.raw);
    assert(s.antes.memoria==s.despues.memoria && s.antes.barreras==s.despues.barreras);
    for(size_t k=0;k<count;++k) assert(p->instructions[k].op==original.instructions[k].op && p->instructions[k].source_line==original.instructions[k].source_line);
    if(equivalencia) {
        Estado antes=ejecutar(&original), despues=ejecutar(p);
        for(size_t k=0;k<antes.n;++k) if(antes.nombres[k][0]!='%')
            assert(antes.valores[k]==leer(&despues,antes.nombres[k]));
    }
    ir_free(&original); return s;
}
static void waw(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"2",NULL,"%t0"); EM(IR_COPY,"7",NULL,"%t0"); EM(IR_COPY,"%t0",NULL,"salida");
    RenEstadisticas s=comprobar(&p,1);
    assert(s.definiciones==1 && s.usos==0 && s.antes.waw==1 && !s.despues.waw);
    EQ(I(0).result,"%t1"); EQ(I(1).result,"%t0");
    ir_free(&p); puts("PASS: WAW pura, colision %t0 y ultima definicion conservada");
}
static void war(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"2",NULL,"%t0"); EM(IR_GOTO,NULL,NULL,"@L0");
    EM(IR_LABEL,NULL,NULL,"@L0"); EM(IR_COPY,"%t0",NULL,"viejo");
    EM(IR_ADD,"4","5","%t0"); EM(IR_MUL,"%t0","2","nuevo");
    EM(IR_RETURN,"nuevo",NULL,NULL);
    RenEstadisticas s=comprobar(&p,1);
    assert(s.definiciones==1 && s.usos==1 && s.antes.war==1 && !s.despues.war && !s.antes.waw);
    EQ(I(0).result,"%t0"); EQ(I(3).arg1,"%t0"); EQ(I(4).result,"%t1"); EQ(I(5).arg1,"%t1");
    ir_free(&p); puts("PASS: WAR pura de temporal entrante, sin uso fuera ni ciclo");
}
static void combinadas(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_FUNC_BEGIN,NULL,NULL,"principal"); EM(IR_DECL,"entero",NULL,"a");
    EM(IR_DECL,"entero",NULL,"d"); EM(IR_DECL,"entero",NULL,"h");
    EM(IR_ADD,"2","3","a"); EM(IR_MUL,"a","a","d");
    EM(IR_ADD,"a","4","a"); EM(IR_SUB,"a","2","h");
    EM(IR_FUNC_END,NULL,NULL,"principal");
    puts("IR antes:"); ir_print(&p,stdout);
    RenEstadisticas s=comprobar(&p,1);
    assert(s.definiciones==1 && s.usos==3 && s.instrucciones==3);
    assert(s.antes.war==1 && s.antes.waw==1 && !s.despues.war && !s.despues.waw);
    EQ(I(4).result,"%t0"); EQ(I(5).arg1,"%t0"); EQ(I(5).arg2,"%t0"); EQ(I(6).arg1,"%t0"); EQ(I(6).result,"a"); EQ(I(7).arg1,"a");
    puts("IR despues:"); ir_print(&p,stdout); ren_imprimir(&s,stdout);
    s=comprobar(&p,1); assert(!s.definiciones && !s.usos);
    ir_free(&p); puts("PASS: WAR/WAW combinadas, doble lectura, a=a+4 e idempotencia");
}
static void varias_definiciones(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"3",NULL,"%t0"); EM(IR_COPY,"%t0",NULL,"%t7");
    EM(IR_COPY,"%t0",NULL,"a"); EM(IR_ADD,"%t0","1","%t0");
    EM(IR_COPY,"%t0",NULL,"b"); EM(IR_ADD,"%t0","1","%t0");
    EM(IR_ADD,"%t0","%t7","c");
    RenEstadisticas s=comprobar(&p,1);
    assert(s.definiciones==2 && !s.despues.war && !s.despues.waw);
    EQ(I(0).result,"%t1"); EQ(I(1).result,"%t7"); EQ(I(1).arg1,"%t1"); EQ(I(3).result,"%t2"); EQ(I(3).arg1,"%t1"); EQ(I(5).arg1,"%t2"); EQ(I(5).result,"%t0");
    ir_free(&p); puts("PASS: varias versiones, copias que comparten nodo y nombres no colisionan");
}
static void fronteras(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"2",NULL,"%t0"); EM(IR_GOTO,NULL,NULL,"@L0");
    EM(IR_LABEL,NULL,NULL,"@L0"); EM(IR_COPY,"%t0",NULL,"viejo");
    EM(IR_ADD,"4","5","%t0"); EM(IR_GOTO,NULL,NULL,"@L1");
    EM(IR_LABEL,NULL,NULL,"@L1"); EM(IR_RETURN,"%t0",NULL,NULL);
    RenEstadisticas s=comprobar(&p,1); assert(!s.definiciones && s.antes.war==1); ir_free(&p);
    EM(IR_COPY,"0",NULL,"%t0"); EM(IR_LABEL,NULL,NULL,"@L0");
    EM(IR_COPY,"%t0",NULL,"viejo"); EM(IR_ADD,"viejo","1","%t0");
    EM(IR_LT,"%t0","3","cond"); EM(IR_IF_TRUE,"cond",NULL,"@L0");
    s=comprobar(&p,1); assert(!s.definiciones && s.antes.war==1); ir_free(&p);
    EM(IR_COPY,"1",NULL,"%t0"); EM(IR_IF_TRUE,"1",NULL,"@L0");
    EM(IR_COPY,"2",NULL,"%t0"); EM(IR_GOTO,NULL,NULL,"@L1");
    EM(IR_LABEL,NULL,NULL,"@L0"); EM(IR_COPY,"3",NULL,"%t0");
    EM(IR_LABEL,NULL,NULL,"@L1"); EM(IR_RETURN,"%t0",NULL,NULL);
    s=comprobar(&p,1); assert(!s.definiciones); ir_free(&p);
    puts("PASS: salida viva, retroceso de ciclo y union de ramas preservados");
}
static void ciclo_local(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_FUNC_BEGIN,NULL,NULL,"principal");
    EM(IR_DECL,"entero",NULL,"a"); EM(IR_DECL,"entero",NULL,"i");
    EM(IR_DECL,"entero",NULL,"suma"); EM(IR_COPY,"0",NULL,"i");
    EM(IR_COPY,"0",NULL,"suma"); EM(IR_LABEL,NULL,NULL,"@L0");
    EM(IR_ADD,"i","1","a"); EM(IR_ADD,"suma","a","suma");
    EM(IR_ADD,"i","2","a"); EM(IR_ADD,"i","1","i");
    EM(IR_LT,"i","3","%t6"); EM(IR_IF_TRUE,"%t6",NULL,"@L0");
    EM(IR_ADD,"suma","a","salida"); EM(IR_FUNC_END,NULL,NULL,"principal");
    RenEstadisticas s=comprobar(&p,1);
    assert(s.definiciones==1 && s.antes.war>s.despues.war && s.antes.waw>s.despues.waw);
    EQ(I(7).result,"%t0"); EQ(I(8).arg2,"%t0"); EQ(I(9).result,"a"); EQ(I(13).arg2,"a");
    Estado e=ejecutar(&p); assert(leer(&e,"salida")==10);
    ir_free(&p); puts("PASS: renombramiento dentro de ciclo, valor final usado en otro bloque");
}
static void barreras_y_ambitos(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_DECL,"entero",NULL,"global"); EM(IR_FUNC_BEGIN,NULL,NULL,"foo");
    EM(IR_DECL,"entero",NULL,"a"); EM(IR_DECL,"entero",NULL,"duplicada");
    EM(IR_COPY,"1",NULL,"a"); EM(IR_CALL,"bar","0",NULL); EM(IR_COPY,"2",NULL,"a");
    EM(IR_COPY,"1",NULL,"global"); EM(IR_COPY,"2",NULL,"global");
    EM(IR_COPY,"1",NULL,"duplicada"); EM(IR_COPY,"2",NULL,"duplicada");
    EM(IR_COPY,"1",NULL,"%t0"); EX(IR_MATRIX_STORE,"1","1","m","%t0"); EM(IR_COPY,"2",NULL,"%t0");
    EM(IR_COPY,"1",NULL,"%t1"); EM(IR_DIV,"2","0","%t9"); EM(IR_COPY,"2",NULL,"%t1");
    EM(IR_FUNC_END,NULL,NULL,"foo"); EM(IR_FUNC_BEGIN,NULL,NULL,"bar");
    EM(IR_DECL,"entero",NULL,"duplicada"); EM(IR_FUNC_END,NULL,NULL,"bar");
    RenEstadisticas s=comprobar(&p,0); assert(!s.definiciones); ir_free(&p);
    puts("PASS: llamadas, memoria, division, globales y nombres ambiguos");
}
static void declaracion_y_uso_entrada(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_FUNC_BEGIN,NULL,NULL,"principal"); EM(IR_DECL,"entero",NULL,"a");
    EM(IR_COPY,"1",NULL,"a"); EM(IR_DECL,"entero",NULL,"b");
    EM(IR_COPY,"a",NULL,"b"); EM(IR_COPY,"2",NULL,"a");
    EM(IR_FUNC_END,NULL,NULL,"principal");
    RenEstadisticas s=comprobar(&p,1); assert(!s.definiciones); ir_free(&p);
    puts("PASS: barrera de declaracion y ultima variable de usuario conservada");
}
static NodoAST *declarar(const char *n)
{
    NodoAST *d=ast_crear(AST_DECLARACION,"entero"); ast_agregar_hijo(d,ast_crear(AST_VARIABLE,n)); return d;
}
static void asig(NodoAST *b,const char *dest,const char *x,const char *op,const char *y)
{
    NodoAST *a=ast_crear(AST_ASIGNACION,NULL),*e=ast_crear(AST_OPERACION,op);
    ast_agregar_hijo(a,ast_crear(AST_VARIABLE,dest));
    ast_agregar_hijo(e,ast_crear(*x>='0'&&*x<='9'?AST_NUMERO:AST_VARIABLE,x));
    ast_agregar_hijo(e,ast_crear(*y>='0'&&*y<='9'?AST_NUMERO:AST_VARIABLE,y));
    ast_agregar_hijo(a,e); ast_agregar_hijo(b,a);
}
static void integracion(void)
{
    IRProgram p; ir_init(&p); IRError er;
    NodoAST *raiz=ast_crear(AST_PROGRAMA,NULL),*principal=ast_crear(AST_PRINCIPAL,NULL),*b=ast_crear(AST_BLOQUE,NULL);
    ast_agregar_hijo(raiz,principal); ast_agregar_hijo(principal,b);
    ast_agregar_hijo(b,declarar("a")); ast_agregar_hijo(b,declarar("d")); ast_agregar_hijo(b,declarar("h"));
    asig(b,"a","2","+","3"); asig(b,"d","a","*","2"); asig(b,"a","7","+","4"); asig(b,"h","a","-","1");
    assert(ir_generate(raiz,&p,&er));
    RenEstadisticas s=comprobar(&p,1); assert(s.definiciones==1 && !s.despues.war && !s.despues.waw);
    EQ(I(5).result,"%t4"); EQ(I(6).arg1,"%t4"); EQ(I(9).result,"a");
    ir_free(&p); ast_liberar(raiz); puts("PASS: AST -> IR real -> CFG/DAG -> renombramiento y equivalencia escalar");
}
static void errores(void)
{
    IRProgram p; ir_init(&p); RenEstadisticas s;
    assert(renombrar_programa(&p,&s)&&!s.definiciones); assert(!renombrar_programa(NULL,&s));
    EM(IR_COPY,"2",NULL,"%t0"); EM(IR_COPY,"3",NULL,"%t0");
    EM(IR_GOTO,NULL,NULL,"@L0"); EM(IR_LABEL,NULL,NULL,"@L0"); EM(IR_ADD,"x",NULL,"a");
    char *anterior=I(0).result;
    assert(!renombrar_programa(&p,&s)&&!s.definiciones); assert(I(0).result==anterior && p.next_temp==0); EQ(I(0).result,"%t0");
    ir_free(&p); puts("PASS: error en bloque posterior, IR intacta (rollback)");
}
int main(void)
{
    waw(); war(); combinadas(); varias_definiciones(); fronteras(); ciclo_local(); barreras_y_ambitos(); declaracion_y_uso_entrada(); integracion(); errores();
    puts("PASS: todas las pruebas de renombramiento"); return 0;
}
