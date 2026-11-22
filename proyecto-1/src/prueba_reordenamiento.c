#include "reordenamiento.h"
#include "renombramiento.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylineno=1;
#define EM(op,a,b,r) assert(ir_emit(&p,op,a,b,r,(int)p.count+1))
#define EX(op,a,b,r,e) assert(ir_emit_ex(&p,op,a,b,r,e,(int)p.count+1))
#define I(k) p.instructions[k]

/* Oraculo escalar de pruebas, independiente del algoritmo de reordenamiento.
   No simula ciclos, cache, ISA ni convencion de llamadas del procesador. */
typedef struct { const char *nombres[256]; long valores[256]; size_t n; } Estado;
static void guardar(Estado *e,const char *n,long v)
{
    assert(n); size_t k=0; while(k<e->n && strcmp(e->nombres[k],n)) ++k;
    if(k==e->n) { assert(e->n<256); e->nombres[e->n++]=n; }
    e->valores[k]=v;
}
static long leer(const Estado *e,const char *n)
{
    char *fin; assert(n); long v=strtol(n,&fin,10);
    if(*n && !*fin) return v;
    if(!strcmp(n,"verdadero")) return 1;
    if(!strcmp(n,"falso")) return 0;
    for(size_t k=0;k<e->n;++k) if(!strcmp(e->nombres[k],n)) return e->valores[k];
    fprintf(stderr,"Lectura sin inicializar: %s\n",n); abort();
}
static size_t buscar(const IRProgram *p,IROp op,const char *nombre)
{
    for(size_t j=0;j<p->count;++j)
        if(p->instructions[j].op==op && p->instructions[j].result && !strcmp(p->instructions[j].result,nombre)) return j;
    assert(0 && "destino inexistente"); return 0;
}
/* Las funciones del fixture usan nombres de parametros/locales distintos,
   sin recursion; el oraculo solo modela ese subconjunto en estas pruebas. */
typedef struct { size_t ret; const char *dest; long args[8]; size_t n, siguiente; } Llamada;
static Estado ejecutar(const IRProgram *p)
{
    Estado e={0}; size_t pc=0,pasos=0,prof=0,na=0;
    long args[8]={0}; Llamada pila[8]={0};
    while(pc<p->count) {
        assert(++pasos<10000); const IRInstruction *i=&p->instructions[pc++]; long v=0;
        switch(i->op) {
            case IR_FUNC_BEGIN:
                if(!prof && strcmp(i->result,"principal")) {
                    while(pc<p->count && p->instructions[pc].op!=IR_FUNC_END) ++pc;
                    assert(pc<p->count); ++pc;
                }
                continue;
            case IR_FUNC_END:
                assert(!prof); return e;
            case IR_LABEL: case IR_DECL: continue;
            case IR_PARAM_DECL:
                assert(prof && pila[prof-1].siguiente<pila[prof-1].n);
                guardar(&e,i->result,pila[prof-1].args[pila[prof-1].siguiente++]); continue;
            case IR_PARAM: assert(na<8); args[na++]=leer(&e,i->arg1); continue;
            case IR_CALL:
                assert(prof<8 && (long)na==strtol(i->arg2,NULL,10));
                pila[prof]=(Llamada){.ret=pc,.dest=i->result,.n=na};
                memcpy(pila[prof].args,args,na*sizeof(*args)); ++prof; na=0;
                pc=buscar(p,IR_FUNC_BEGIN,i->arg1); continue;
            case IR_RETURN:
                v=i->arg1?leer(&e,i->arg1):0;
                if(!prof) { guardar(&e,"#retorno",v); return e; }
                --prof; pc=pila[prof].ret;
                if(pila[prof].dest) guardar(&e,pila[prof].dest,v);
                continue;
            case IR_COPY: v=leer(&e,i->arg1); break;
            case IR_NEG: v=-leer(&e,i->arg1); break;
            case IR_ADD: v=leer(&e,i->arg1)+leer(&e,i->arg2); break;
            case IR_SUB: v=leer(&e,i->arg1)-leer(&e,i->arg2); break;
            case IR_MUL: v=leer(&e,i->arg1)*leer(&e,i->arg2); break;
            case IR_LT: v=leer(&e,i->arg1)<leer(&e,i->arg2); break;
            case IR_EQ: v=leer(&e,i->arg1)==leer(&e,i->arg2); break;
            case IR_IF_TRUE: case IR_IF_FALSE: case IR_GOTO:
                if(i->op==IR_IF_TRUE && !leer(&e,i->arg1)) continue;
                if(i->op==IR_IF_FALSE && leer(&e,i->arg1)) continue;
                pc=buscar(p,IR_LABEL,i->result); continue;
            default: assert(0 && "opcode fuera del oraculo");
        }
        guardar(&e,i->result,v);
    }
    return e;
}
static void clonar(const IRProgram *p,IRProgram *q)
{
    ir_init(q);
    for(size_t k=0;k<p->count;++k) {
        const IRInstruction *i=&p->instructions[k];
        assert(ir_emit_ex(q,i->op,i->arg1,i->arg2,i->result,i->extra,i->source_line));
    }
    q->next_temp=p->next_temp; q->next_label=p->next_label;
}
static int iguales(const char *a,const char *b) { return a==b || (a && b && !strcmp(a,b)); }
static int misma(const IRInstruction *a,const IRInstruction *b)
{
    return a->op==b->op && a->source_line==b->source_line && iguales(a->arg1,b->arg1) &&
        iguales(a->arg2,b->arg2) && iguales(a->result,b->result) && iguales(a->extra,b->extra);
}
/* Validador de pruebas: reconstruye CFG y verifica las dos direcciones del
   grafo de dependencias para distinguir aristas por nombre, tipo e instruccion. */
static int tiene(const DepGrafo *g,size_t a,size_t b,DepTipo t,const char *nombre)
{
    for(size_t k=0;k<g->cantidad;++k) {
        const DepArista *e=&g->aristas[k];
        if(e->origen==a && e->destino==b && e->tipo==t && iguales(e->nombre,nombre)) return 1;
    }
    return 0;
}
static ReoEstadisticas comprobar(IRProgram *p,int ejecutar_oraculo)
{
    IRProgram q; clonar(p,&q); IRInstruction *ptr=p->instructions;
    /* Dar una identidad unica a cada cuadruplo mediante sus punteros propios,
       incluso si dos instrucciones tienen el mismo texto o linea fuente. */
    IRInstruction *old=malloc(p->count*sizeof(*old)); assert(old || !p->count);
    if(p->count) memcpy(old,p->instructions,p->count*sizeof(*old));
    BasicBlockCFG *a=bb_build(&q); assert(a && bb_build_cfg(a));
    ReoEstadisticas s; assert(reordenar_programa(p,&s));
    assert(p->instructions==ptr && p->count==q.count && p->next_temp==q.next_temp && p->next_label==q.next_label);
    BasicBlockCFG *b=bb_build(p); assert(b && bb_build_cfg(b));
    assert(a->block_count==b->block_count && a->edge_count==b->edge_count);
    for(size_t i=0;i<a->edge_count;++i) assert(a->edges[i].from==b->edges[i].from && a->edges[i].to==b->edges[i].to);
    size_t *pos=malloc(p->count*sizeof(*pos)), *orig=malloc(p->count*sizeof(*orig)); assert((pos && orig)||!p->count);
    for(size_t k=0;k<p->count;++k) pos[k]=(size_t)-1;
    size_t movidas=0;
    for(size_t k=0;k<p->count;++k) {
        const IRInstruction *i=&p->instructions[k]; size_t j=0;
        for(;j<p->count;++j) if(pos[j]==(size_t)-1 && misma(i,&old[j]) && i->arg1==old[j].arg1 && i->arg2==old[j].arg2 && i->result==old[j].result && i->extra==old[j].extra) break;
        assert(j<p->count); pos[j]=k; orig[k]=j; movidas+=j!=k;
        DepAccesos acc; assert(dep_accesos(i,&acc)); if(acc.barrera) assert(j==k);
    }
    assert(movidas==s.instrucciones && s.esperas_despues<=s.esperas_antes);
    for(size_t k=0;k<a->block_count;++k) {
        const BasicBlock *x=&a->blocks[k], *y=&b->blocks[k];
        assert(x->start==y->start && x->end==y->end);
        for(size_t j=x->start;j<=x->end;++j) assert(pos[j]>=y->start && pos[j]<=y->end);
        DepGrafo g={0}, h={0}; assert(dep_analizar_bloque(&q,x,&g)&&dep_analizar_bloque(p,y,&h));
        assert(g.cantidad==h.cantidad);
        for(size_t j=0;j<g.cantidad;++j) { const DepArista *e=&g.aristas[j];
            assert(pos[e->origen]<pos[e->destino]); assert(tiene(&h,pos[e->origen],pos[e->destino],e->tipo,e->nombre)); }
        for(size_t j=0;j<h.cantidad;++j) { const DepArista *e=&h.aristas[j];
            assert(tiene(&g,orig[e->origen],orig[e->destino],e->tipo,e->nombre)); }
        dep_liberar(&g); dep_liberar(&h);
    }
    if(ejecutar_oraculo) {
        Estado e=ejecutar(&q), f=ejecutar(p);
        assert(e.n==f.n);
        for(size_t j=0;j<e.n;++j) assert(e.valores[j]==leer(&f,e.nombres[j]));
    }
    free(pos); free(orig); free(old); bb_free(a); bb_free(b); ir_free(&q); return s;
}
static void esperas(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_ADD,"2","3","a"); EM(IR_SUB,"a","1","d");
    EM(IR_ADD,"7","4","b"); EM(IR_SUB,"8","2","c");
    puts("IR antes:"); ir_print(&p,stdout);
    ReoEstadisticas s=comprobar(&p,1);
    assert(s.instrucciones==3 && s.esperas_antes==2 && s.esperas_despues==0);
    assert(I(0).source_line==1 && I(1).source_line==3 && I(2).source_line==4 && I(3).source_line==2);
    puts("IR despues:"); ir_print(&p,stdout); reo_imprimir(&s,stdout);
    s=comprobar(&p,1); assert(!s.instrucciones); ir_free(&p);
    puts("PASS: separacion RAW, orden determinista y caso estable");
}
static void prioridad(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"9",NULL,"b"); EM(IR_ADD,"2","3","a"); EM(IR_ADD,"a","1","c");
    EM(IR_SUB,"c","1","d"); EM(IR_COPY,"4",NULL,"e");
    ReoEstadisticas s=comprobar(&p,1);
    assert(s.esperas_antes>s.esperas_despues && I(0).source_line==2);
    ir_free(&p); puts("PASS: ruta critica con prioridad sobre una instruccion independiente");
}
static void riesgos_y_presion(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"3",NULL,"a"); EM(IR_ADD,"a","1","d"); EM(IR_COPY,"8",NULL,"a");
    EM(IR_ADD,"a","1","h"); EM(IR_COPY,"2",NULL,"b"); EM(IR_SUB,"5","2","c");
    ReoEstadisticas s=comprobar(&p,1); assert(s.esperas_antes>s.esperas_despues); ir_free(&p);
    EM(IR_COPY,"1",NULL,"a"); EM(IR_ADD,"a","1","d");
    EM(IR_COPY,"2",NULL,"b"); EM(IR_ADD,"b","1","e");
    EM(IR_COPY,"3",NULL,"c"); EM(IR_ADD,"c","1","f");
    s=comprobar(&p,1); assert(s.rechazadas_presion==1 && !s.instrucciones && s.pico_local_antes==1 && s.pico_local_despues==1); ir_free(&p);
    EM(IR_COPY,"1",NULL,"a"); EM(IR_ADD,"a","1","d");
    EM(IR_COPY,"2",NULL,"b"); EM(IR_ADD,"b","1","e");
    s=comprobar(&p,1); assert(s.instrucciones && s.pico_local_antes==1 && s.pico_local_despues==2); ir_free(&p);
    puts("PASS: RAW/WAR/WAW restantes, limite de presion y dos cadenas independientes");
}
static void control(void)
{
    for(int rama=0;rama<2;++rama) {
        IRProgram p; ir_init(&p);
        EM(IR_COPY,rama?"1":"0",NULL,"cond"); EM(IR_IF_FALSE,"cond",NULL,"@L0");
        EM(IR_ADD,"2","3","a"); EM(IR_MUL,"a","2","b"); EM(IR_ADD,"1","4","c"); EM(IR_COPY,"7",NULL,"d");
        EM(IR_GOTO,NULL,NULL,"@L1"); EM(IR_LABEL,NULL,NULL,"@L0");
        EM(IR_ADD,"8","3","a"); EM(IR_MUL,"a","2","b"); EM(IR_ADD,"2","4","c"); EM(IR_COPY,"9",NULL,"d");
        EM(IR_LABEL,NULL,NULL,"@L1"); EM(IR_ADD,"b","d","salida"); EM(IR_RETURN,"salida",NULL,NULL);
        ReoEstadisticas s=comprobar(&p,1); assert(s.regiones_optimizadas>=2);
        Estado e=ejecutar(&p); assert(leer(&e,"#retorno")== (rama?17:31)); ir_free(&p);
    }
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"0",NULL,"i"); EM(IR_COPY,"0",NULL,"suma"); EM(IR_LABEL,NULL,NULL,"@L0");
    EM(IR_ADD,"i","1","a"); EM(IR_ADD,"suma","a","suma"); EM(IR_COPY,"4",NULL,"b"); EM(IR_COPY,"5",NULL,"c");
    EM(IR_ADD,"i","1","i"); EM(IR_LT,"i","3","cond"); EM(IR_IF_TRUE,"cond",NULL,"@L0"); EM(IR_RETURN,"suma",NULL,NULL);
    ReoEstadisticas s=comprobar(&p,1); assert(s.instrucciones); Estado e=ejecutar(&p); assert(leer(&e,"#retorno")==6); ir_free(&p);
    puts("PASS: ambos caminos condicionales, ciclo y CFG identico");
}
static void funciones(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_DECL,"entero",NULL,"global"); EM(IR_COPY,"0",NULL,"global");
    EM(IR_FUNC_BEGIN,NULL,NULL,"foo"); EM(IR_PARAM_DECL,"entero",NULL,"n");
    EM(IR_ADD,"n","1","x"); EM(IR_MUL,"x","2","y"); EM(IR_COPY,"y",NULL,"global");
    EM(IR_COPY,"3",NULL,"z"); EM(IR_COPY,"4",NULL,"w"); EM(IR_RETURN,"y",NULL,NULL); EM(IR_FUNC_END,NULL,NULL,"foo");
    EM(IR_FUNC_BEGIN,NULL,NULL,"principal"); EM(IR_PARAM,"5",NULL,NULL); EM(IR_CALL,"foo","1","r");
    EM(IR_ADD,"r","global","salida"); EM(IR_RETURN,"salida",NULL,NULL); EM(IR_FUNC_END,NULL,NULL,"principal");
    ReoEstadisticas s=comprobar(&p,1); assert(s.instrucciones); Estado e=ejecutar(&p); assert(leer(&e,"#retorno")==24 && leer(&e,"global")==12); ir_free(&p);
    puts("PASS: funciones, argumentos, retorno y efecto sobre global (oraculo escalar)");
}
static void barreras(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_ADD,"2","3","a"); EM(IR_COPY,"6",NULL,"b"); EM(IR_DIV,"a","b","d");
    EM(IR_ADD,"2","3","x"); EM(IR_ADD,"x","1","y"); EM(IR_COPY,"7",NULL,"z"); EM(IR_COPY,"8",NULL,"w");
    EX(IR_MATRIX_STORE,"1","2","m","y"); EX(IR_MATRIX_LOAD,"m","1","q","2");
    EM(IR_ARRAY_STORE,"1","q","arr"); EM(IR_ARRAY_LOAD,"arr","1","v"); EM(IR_PARAM,"v",NULL,NULL); EM(IR_CALL,"foo","1","r");
    EM(IR_POW,"2","3","t"); EM(IR_MOD,"t","2","u"); EM(IR_RETURN,"r",NULL,NULL);
    ReoEstadisticas s=comprobar(&p,0); assert(s.instrucciones==3);
    ir_free(&p); puts("PASS: barreras de memoria, division/modulo/potencia, llamada, parametros y retorno");
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
    assert(ir_generate(raiz,&p,&er)); RenEstadisticas r;
    assert(renombrar_programa(&p,&r) && r.definiciones==1);
    ReoEstadisticas s=comprobar(&p,1); assert(s.instrucciones && s.esperas_despues<s.esperas_antes);
    Estado e=ejecutar(&p); assert(leer(&e,"a")==11 && leer(&e,"d")==10 && leer(&e,"h")==10);
    ir_free(&p); ast_liberar(raiz); puts("PASS: AST -> IR real -> renombramiento -> reordenamiento -> equivalencia");
}
static unsigned aleatorio(unsigned *s) { *s=*s*1664525u+1013904223u; return *s>>16; }
static void muchos(void)
{
    unsigned seed=19; const char *n[]={"a","b","c","d","e","f"}; size_t movidas=0;
    for(size_t t=0;t<180;++t) {
        IRProgram p; ir_init(&p);
        for(size_t j=0;j<6;++j) EM(IR_COPY,"2",NULL,n[j]);
        for(size_t j=0;j<25;++j) {
            unsigned v=aleatorio(&seed); size_t dst=v%6, x=aleatorio(&seed)%6, y=aleatorio(&seed)%6;
            EM(v%2?IR_ADD:IR_SUB,n[x],n[y],n[dst]);
        }
        ReoEstadisticas s=comprobar(&p,1); movidas+=s.instrucciones; ir_free(&p);
    }
    assert(movidas>0); puts("PASS: 180 bloques generados, equivalencia y aristas verificados");
}
static void sin_ventaja(void)
{
    IRProgram p; ir_init(&p);
    EM(IR_COPY,"1",NULL,"a"); EM(IR_ADD,"2","3","b"); EM(IR_SUB,"7","4","c");
    ReoEstadisticas s=comprobar(&p,1); assert(!s.instrucciones && !s.esperas_antes); ir_free(&p);
    EM(IR_COPY,"1",NULL,"a"); EM(IR_ADD,"a","1","b"); EM(IR_SUB,"b","1","c");
    s=comprobar(&p,1); assert(!s.instrucciones && s.esperas_antes==s.esperas_despues); ir_free(&p);
    puts("PASS: sin dependencia y cadena sin alternativa, orden original conservado");
}
static void errores(void)
{
    IRProgram p; ir_init(&p); ReoEstadisticas s;
    assert(reordenar_programa(&p,&s) && !s.instrucciones); assert(!reordenar_programa(NULL,&s));
    EM(IR_ADD,"2","3","a"); EM(IR_ADD,"a","1","b"); EM(IR_COPY,"4",NULL,"c"); EM(IR_COPY,"5",NULL,"d");
    EM(IR_GOTO,NULL,NULL,"@L0"); EM(IR_LABEL,NULL,NULL,"@L0"); EM(IR_ADD,"a",NULL,"e");
    IRInstruction antes[7]; memcpy(antes,p.instructions,sizeof(antes));
    assert(!reordenar_programa(&p,&s) && !s.instrucciones && !memcmp(antes,p.instructions,sizeof(antes)));
    ir_free(&p); puts("PASS: programa vacio y rollback ante IR invalida en bloque posterior");
}
int main(void)
{
    esperas(); prioridad(); riesgos_y_presion(); control(); funciones(); barreras(); integracion(); muchos(); sin_ventaja(); errores();
    puts("PASS: todas las pruebas de reordenamiento"); return 0;
}
