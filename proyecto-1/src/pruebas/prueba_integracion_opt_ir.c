#include "optimizaciones_ir.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylineno = 1; /* Necesario para enlazar ast.c sin Flex/Bison. */

/* Oraculo independiente del pipeline, sobre operaciones escalares y slots
   abstractos de spilling. No pretende simular el procesador ni codegen. */
typedef struct { char nombre[96]; long valor; } Variable;
typedef struct { Variable variables[512]; size_t cantidad; long slots[1024]; } Estado;
static long leer(Estado *e, const char *s)
{
    char *fin;
    long v;
    assert(s);
    if (!strcmp(s, "verdadero")) return 1;
    if (!strcmp(s, "falso")) return 0;
    v = strtol(s, &fin, 10);
    if (*s && !*fin) return v;
    for (size_t k=0; k<e->cantidad; ++k)
        if (!strcmp(e->variables[k].nombre, s)) return e->variables[k].valor;
    fprintf(stderr, "Lectura no inicializada: %s\n",s);
    abort();
}
static void escribir(Estado *e, const char *s, long v)
{
    assert(s);
    for (size_t k=0; k<e->cantidad; ++k)
        if (!strcmp(e->variables[k].nombre,s)) { e->variables[k].valor=v; return; }
    assert(e->cantidad<512 && strlen(s)<sizeof(e->variables[0].nombre));
    strcpy(e->variables[e->cantidad].nombre,s);
    e->variables[e->cantidad++].valor=v;
}
static size_t salto(const IRProgram *p, const char *nombre)
{
    for (size_t i=0;i<p->count;++i)
        if (p->instructions[i].op==IR_LABEL && p->instructions[i].result &&
            !strcmp(p->instructions[i].result,nombre)) return i;
    fprintf(stderr,"Etiqueta no encontrada: %s\n",nombre); abort();
}
static long evaluar(const IRProgram *p)
{
    Estado e={0}; size_t pc=0, pasos=0;
    while(pc<p->count) {
        assert(++pasos<100000);
        const IRInstruction *i=&p->instructions[pc++];
        long v=0;
        switch (i->op) {
            case IR_FUNC_BEGIN: case IR_FUNC_END: case IR_DECL: case IR_LABEL:
                continue;
            case IR_GOTO: pc=salto(p,i->result); continue;
            case IR_IF_FALSE:
                if (!leer(&e,i->arg1)) pc=salto(p,i->result);
                continue;
            case IR_IF_TRUE:
                if (leer(&e,i->arg1)) pc=salto(p,i->result);
                continue;
            case IR_RETURN: return i->arg1 ? leer(&e,i->arg1) : 0;
            case IR_ARRAY_STORE:
                assert(i->result && !strcmp(i->result,"cdff_spill_area"));
                v=leer(&e,i->arg1); assert(v>=0 && v<1024);
                e.slots[v]=leer(&e,i->arg2); continue;
            case IR_ARRAY_LOAD:
                assert(i->arg1 && !strcmp(i->arg1,"cdff_spill_area"));
                v=leer(&e,i->arg2); assert(v>=0 && v<1024);
                escribir(&e,i->result,e.slots[v]); continue;
            case IR_COPY: v=leer(&e,i->arg1); break;
            case IR_NEG: v=-leer(&e,i->arg1); break;
            case IR_ADD: v=leer(&e,i->arg1)+leer(&e,i->arg2); break;
            case IR_SUB: v=leer(&e,i->arg1)-leer(&e,i->arg2); break;
            case IR_MUL: v=leer(&e,i->arg1)*leer(&e,i->arg2); break;
            case IR_LT: v=leer(&e,i->arg1)<leer(&e,i->arg2); break;
            case IR_EQ: v=leer(&e,i->arg1)==leer(&e,i->arg2); break;
            default: fprintf(stderr,"Opcode sin oraculo: %d\n",i->op); abort();
        }
        escribir(&e,i->result,v);
    }
    assert(0 && "Falta RETURN"); return 0;
}
#define EM(p,op,a,b,r) assert(ir_emit((p),(op),(a),(b),(r),__LINE__))
static void prueba_dce_renombrar(void)
{
    IRProgram p; OptIREstadisticas s={0}; ir_init(&p);
    EM(&p,IR_FUNC_BEGIN,NULL,NULL,"principal");
    EM(&p,IR_DECL,"entero",NULL,"x");
    EM(&p,IR_COPY,"7",NULL,"x");
    EM(&p,IR_COPY,"x",NULL,"%t0");
    EM(&p,IR_COPY,"10",NULL,"x");
    EM(&p,IR_ADD,"%t0","x","%t1");
    EM(&p,IR_ADD,"99","1","%t2"); /* muerto */
    EM(&p,IR_RETURN,"%t1",NULL,NULL);
    EM(&p,IR_FUNC_END,NULL,NULL,"principal");
    const size_t inicial=p.count;
    assert(evaluar(&p)==17);
    assert(opt_ir_aplicar(&p,OPT_IR_DCE|OPT_IR_RENOMBRAR|OPT_IR_REORDENAR,9,4,&s));
    assert(s.fases_ejecutadas==(OPT_IR_DCE|OPT_IR_RENOMBRAR|OPT_IR_REORDENAR));
    assert(s.dce.eliminadas==1);
    assert(s.renombramiento.definiciones>=1);
    assert(p.count==inicial-1 && evaluar(&p)==17);
    for(size_t i=0;i<p.count;++i)
        assert(!(p.instructions[i].op==IR_ADD && p.instructions[i].arg1 &&
                 !strcmp(p.instructions[i].arg1,"99")));
    printf("PASS DCE + renombramiento + reordenamiento: %zu -> %zu, valor=17\n",inicial,p.count);
    ir_free(&p);
}
static void prueba_spill_fuerte(void)
{
    IRProgram p; OptIREstadisticas s={0}; ir_init(&p);
    EM(&p,IR_FUNC_BEGIN,NULL,NULL,"principal");
    char nombres[14][24], numero[24];
    for(int k=0;k<14;++k) {
        snprintf(nombres[k],sizeof(nombres[k]),"%%t%d",k);
        snprintf(numero,sizeof(numero),"%d",k+1);
        EM(&p,IR_COPY,numero,NULL,nombres[k]);
    }
    /* No permitir que el reordenamiento mueva los consumos al area de defs. */
    EM(&p,IR_LABEL,NULL,NULL,"@L_consumir");
    EM(&p,IR_ADD,nombres[0],nombres[1],"%t14");
    for(int k=2;k<14;++k) {
        snprintf(numero,sizeof(numero),"%%t%d",13+k);
        char anterior[24]; snprintf(anterior,sizeof(anterior),"%%t%d",12+k);
        EM(&p,IR_ADD,anterior,nombres[k],numero);
    }
    EM(&p,IR_RETURN,"%t26",NULL,NULL);
    EM(&p,IR_FUNC_END,NULL,NULL,"principal");
    assert(evaluar(&p)==105);
    assert(opt_ir_aplicar(&p,OPT_IR_TODAS,9,4,&s));
    assert(s.spilling.rondas>0 && s.spilling.stores>0 && s.spilling.loads>0);
    assert(evaluar(&p)==105);
    AsignacionRegistros mapa; ar_init(&mapa);
    assert(opt_ir_asignacion_para_codegen_ir(&p,9,4,&mapa));
    assert(mapa.cantidad_spills==0 && mapa.cantidad>0);
    for(size_t j=0;j<mapa.cantidad;++j)
        assert(mapa.entradas[j].registro>=4 && mapa.entradas[j].registro<=12);
    ar_free(&mapa);
    printf("PASS cuatro fases + spilling real IR: STORE=%zu LOAD=%zu slots=%zu, valor=105\n",
           s.spilling.stores,s.spilling.loads,s.spilling.slots);
    ir_free(&p);
}
static void prueba_ciclo(void)
{
    IRProgram p; OptIREstadisticas s={0}; ir_init(&p);
    EM(&p,IR_FUNC_BEGIN,NULL,NULL,"principal");
    EM(&p,IR_DECL,"entero",NULL,"i");
    EM(&p,IR_DECL,"entero",NULL,"total");
    EM(&p,IR_COPY,"0",NULL,"total");
    EM(&p,IR_COPY,"0",NULL,"i");
    EM(&p,IR_LABEL,NULL,NULL,"@L_bucle");
    EM(&p,IR_LT,"i","5","%t0");
    EM(&p,IR_IF_FALSE,"%t0",NULL,"@L_fin");
    EM(&p,IR_ADD,"total","i","%t1");
    EM(&p,IR_COPY,"%t1",NULL,"total");
    EM(&p,IR_ADD,"i","1","%t2");
    EM(&p,IR_COPY,"%t2",NULL,"i");
    EM(&p,IR_GOTO,NULL,NULL,"@L_bucle");
    EM(&p,IR_LABEL,NULL,NULL,"@L_fin");
    EM(&p,IR_RETURN,"total",NULL,NULL);
    EM(&p,IR_FUNC_END,NULL,NULL,"principal");
    assert(evaluar(&p)==10);
    assert(opt_ir_aplicar(&p,OPT_IR_TODAS,9,4,&s));
    assert(evaluar(&p)==10);
    printf("PASS ciclo/CFG/liveness: valor=10\n");
    ir_free(&p);
}
/* El flujo completo comienza con AST real y termina en IR transformada. */
static NodoAST *declaracion_ast(const char *nombre)
{
    NodoAST *n=ast_crear(AST_DECLARACION,"entero");
    ast_agregar_hijo(n,ast_crear(AST_VARIABLE,nombre));
    return n;
}
static void asignacion_ast(NodoAST *bloque, const char *destino,
                          const char *a, const char *op, const char *b)
{
    NodoAST *asig=ast_crear(AST_ASIGNACION,NULL);
    NodoAST *expr=ast_crear(AST_OPERACION,op);
    ast_agregar_hijo(asig,ast_crear(AST_VARIABLE,destino));
    ast_agregar_hijo(expr,ast_crear(*a>='0' && *a<='9' ? AST_NUMERO : AST_VARIABLE,a));
    ast_agregar_hijo(expr,ast_crear(*b>='0' && *b<='9' ? AST_NUMERO : AST_VARIABLE,b));
    ast_agregar_hijo(asig,expr); ast_agregar_hijo(bloque,asig);
}
static void prueba_ast_a_optimizaciones(void)
{
    IRProgram ir; IRError error={0}; OptIREstadisticas s={0}; ir_init(&ir);
    NodoAST *raiz=ast_crear(AST_PROGRAMA,NULL);
    NodoAST *principal=ast_crear(AST_PRINCIPAL,NULL);
    NodoAST *bloque=ast_crear(AST_BLOQUE,NULL);
    ast_agregar_hijo(raiz,principal); ast_agregar_hijo(principal,bloque);
    ast_agregar_hijo(bloque,declaracion_ast("a"));
    ast_agregar_hijo(bloque,declaracion_ast("d"));
    ast_agregar_hijo(bloque,declaracion_ast("h"));
    ast_agregar_hijo(bloque,declaracion_ast("muerto"));
    asignacion_ast(bloque,"a","2","+","3");
    asignacion_ast(bloque,"d","a","*","2");
    asignacion_ast(bloque,"a","7","+","4");
    asignacion_ast(bloque,"h","a","-","1");
    asignacion_ast(bloque,"muerto","80","+","1");
    NodoAST *ret=ast_crear(AST_RETURN,NULL);
    NodoAST *suma=ast_crear(AST_OPERACION,"+");
    ast_agregar_hijo(suma,ast_crear(AST_VARIABLE,"d"));
    ast_agregar_hijo(suma,ast_crear(AST_VARIABLE,"h"));
    ast_agregar_hijo(ret,suma); ast_agregar_hijo(bloque,ret);
    assert(ir_generate(raiz,&ir,&error));
    assert(evaluar(&ir)==20);
    assert(opt_ir_aplicar(&ir,OPT_IR_TODAS,9,4,&s));
    assert(s.dce.eliminadas>=1);
    assert(s.renombramiento.definiciones>=1);
    assert(evaluar(&ir)==20);
    printf("PASS AST -> IR -> cuatro optimizaciones, eliminadas=%zu, valor=20\n",s.dce.eliminadas);
    ir_free(&ir); ast_liberar(raiz);
}
static void prueba_flags_error_atomico(void)
{
    IRProgram p; OptIREstadisticas s={0}; ir_init(&p);
    EM(&p,IR_COPY,"1",NULL,"%t0");
    assert(!opt_ir_aplicar(&p,OPT_IR_SPILL,-1,4,&s));
    assert(p.count==1 && p.instructions[0].op==IR_COPY);
    assert(opt_ir_aplicar(&p,0,9,4,&s) && p.count==1);
    ir_free(&p);
    ir_init(&p);
    EM(&p,IR_DECL,"entero",NULL,SPILL_IR_AREA);
    EM(&p,IR_COPY,"8",NULL,"%t0");
    assert(!opt_ir_aplicar(&p,OPT_IR_SPILL,9,4,&s));
    assert(p.count==2 && p.instructions[0].op==IR_DECL);
    ir_free(&p);
    puts("PASS API: opciones/errores/colision reservada no destruyen IR");
}
int main(void)
{
    prueba_dce_renombrar();
    prueba_spill_fuerte();
    prueba_ciclo();
    prueba_ast_a_optimizaciones();
    prueba_flags_error_atomico();
    puts("PASS integracion de optimizaciones IR");
    return 0;
}
