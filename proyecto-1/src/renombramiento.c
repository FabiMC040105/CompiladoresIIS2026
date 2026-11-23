#include "renombramiento.h"
#include <stdlib.h>
#include <string.h>

static const unsigned campos[DAG_CAMPOS] = {DEP_ARG1, DEP_ARG2, DEP_RESULT, DEP_EXTRA};

static int iguales(const char *a, const char *b)
{
    return a && b && strcmp(a,b)==0;
}

static int temporal(const char *s)
{
    return s && s[0]=='%' && dep_es_nombre(s);
}

static char **campo_mutable(IRInstruction *i, unsigned campo)
{
    switch (campo) {
        case DEP_ARG1: return &i->arg1;
        case DEP_ARG2: return &i->arg2;
        case DEP_RESULT: return &i->result;
        case DEP_EXTRA: return &i->extra;
        default: return NULL;
    }
}

static int reemplazar(char **campo, const char *texto)
{
    size_t n = strlen(texto)+1;
    char *copia = malloc(n);
    if (!copia) return 0;
    memcpy(copia,texto,n);
    free(*campo);
    *campo=copia;
    return 1;
}

/* Trabajar en una copia: ni los grafos (que prestan cadenas) ni la IR real
   quedan invalidados a mitad del analisis. */
static int clonar(const IRProgram *p, IRProgram *copia)
{
    ir_init(copia);
    copia->next_temp=p->next_temp;
    copia->next_label=p->next_label;
    for (size_t i=0;i<p->count;++i) {
        const IRInstruction *v=&p->instructions[i];
        if (!ir_emit_ex(copia,v->op,v->arg1,v->arg2,v->result,v->extra,v->source_line)) {
            ir_free(copia); return 0;
        }

        /* La copia mantiene el contexto del AST original.
           No somos propietarios de ese nodo. */
        copia->instructions[copia->count - 1].nodo_origen =
            v->nodo_origen;
    }
    return 1;
}

/* Incluso si unrolling construyo %tN a mano y no actualizo next_temp,
   no debemos reutilizar ningun nombre que exista en la IR. */
static char *nuevo_temporal(IRProgram *p)
{
    for (;;) {
        char *nombre=ir_new_temp(p);
        if (!nombre) return NULL;
        int ocupado=0;
        for (size_t i=0;i<p->count && !ocupado;++i)
            for (size_t k=0;k<DAG_CAMPOS;++k)
                if (iguales(dep_campo(&p->instructions[i],campos[k]),nombre)) {
                    ocupado=1; break;
                }
        if (!ocupado) return nombre;
        free(nombre);
    }
}

/* La IR no tiene IDs de simbolo. Solo aceptar locales escalares con una
   unica declaracion textual en todo el programa, antes de la definicion y
   dentro de la misma funcion. No adivinar ambitos ni renombrar globales. */
static int escalar_local_unico(const IRProgram *p, size_t def, const char *nombre)
{
    size_t funcion=DAG_NULO, funcion_decl=DAG_NULO, funcion_def=DAG_NULO;
    size_t cantidad=0, declaracion=DAG_NULO;
    int escalar=0;
    for (size_t i=0;i<p->count;++i) {
        const IRInstruction *v=&p->instructions[i];
        if (v->op==IR_FUNC_BEGIN) funcion=i;
        if (i==def) funcion_def=funcion;
        if ((v->op==IR_DECL || v->op==IR_PARAM_DECL || v->op==IR_ARRAY_DECL ||
             v->op==IR_MATRIX_DECL) && iguales(v->result,nombre)) {
            ++cantidad; declaracion=i; funcion_decl=funcion;
            escalar=(v->op==IR_DECL || (v->op==IR_PARAM_DECL && !v->arg2 && !v->extra)) &&
                    (iguales(v->arg1,"entero") || iguales(v->arg1,"vof"));
        }
        if (v->op==IR_FUNC_END) funcion=DAG_NULO;
    }
    return cantidad==1 && escalar && declaracion<def && funcion_def!=DAG_NULO &&
           funcion_decl==funcion_def;
}

static void contar(const DepGrafo *g, RenConteo *s)
{
    for (size_t k=0;k<g->cantidad;++k) switch (g->aristas[k].tipo) {
        case DEP_RAW: ++s->raw; break;
        case DEP_WAR: ++s->war; break;
        case DEP_WAW: ++s->waw; break;
        case DEP_MEMORIA: ++s->memoria; break;
        case DEP_BARRERA: ++s->barreras; break;
    }
}

/* Restriccion conservadora para renombrar la ultima definicion de un %tN:
   buscar un camino no vacio desde el bloque hasta si mismo. No se calcula
   liveness global. 1 = hay ciclo, 0 = no hay ciclo, -1 = fallo. */
static int vuelve_al_bloque(const BasicBlockCFG *cfg, size_t b)
{
    unsigned char *visitado=calloc(cfg->block_count,1);
    if (!visitado) return -1;
    for (size_t i=0;i<cfg->edge_count;++i)
        if (cfg->edges[i].from==b) visitado[cfg->edges[i].to]=1;
    int cambio=1;
    while (cambio && !visitado[b]) {
        cambio=0;
        for (size_t i=0;i<cfg->edge_count;++i) {
            CFGEdge e=cfg->edges[i];
            if (visitado[e.from] && !visitado[e.to]) {
                visitado[e.to]=1; cambio=1;
            }
        }
    }
    int ciclo=visitado[b]!=0;
    free(visitado);
    return ciclo;
}

static int leido_fuera(const IRProgram *p, const BasicBlock *b, const char *nombre)
{
    for (size_t i=0;i<p->count;++i) {
        if (i>=b->start && i<=b->end) continue;
        DepAccesos a;
        if (!dep_accesos(&p->instructions[i],&a)) return 1;
        for (size_t k=0;k<DAG_CAMPOS;++k)
            if ((a.lecturas & campos[k]) && iguales(dep_campo(&p->instructions[i],campos[k]),nombre))
                return 1;
    }
    return 0;
}

/* El intervalo incluye la escritura que mata el valor viejo. Una barrera
   podria observarlo o cambiar el significado del nombre. */
static int sin_barreras(const DAGBloque *d, size_t desde, size_t hasta)
{
    for (size_t i=desde;i<=hasta;++i)
        if (dag_buscar_instruccion(d,i)->accesos.barrera) return 0;
    return 1;
}

/* Aplicar una definicion y SOLO los campos cuyo uso apunta a esa definicion.
   No comparar IDs de nodos: COPY y CSE estructural pueden compartir nodo.
   Los grafos consultan original; escribir exclusivamente en copia. */
static int aplicar(const IRProgram *p, IRProgram *copia, const DAGBloque *d,
                   size_t def, size_t hasta, const char *nombre,
                   unsigned char *tocadas, RenEstadisticas *s)
{
    char *nuevo=nuevo_temporal(copia);
    if (!nuevo) return 0;
    if (!reemplazar(&copia->instructions[def].result,nuevo)) goto error;
    tocadas[def]=1; ++s->definiciones;
    for (size_t j=def+1;j<=hasta;++j) {
        const DAGInstruccion *i=dag_buscar_instruccion(d,j);
        for (size_t k=0;k<DAG_CAMPOS;++k) {
            unsigned c=campos[k];
            if ((i->accesos.lecturas & c) && i->usos[k].definicion==def &&
                iguales(dep_campo(&p->instructions[j],c),nombre)) {
                if (!reemplazar(campo_mutable(&copia->instructions[j],c),nuevo)) goto error;
                tocadas[j]=1; ++s->usos;
            }
        }
    }
    free(nuevo); return 1;
error:
    free(nuevo); return 0;
}

/* Validacion independiente del nombre nuevo: cada lectura conserva su
   definicion local original, y una entrada sigue siendo entrada. Tambien
   se conservan las restricciones de memoria/control y las RAW por pares. */
static int validar(const IRProgram *p, const IRProgram *copia, const BasicBlock *b,
                   const DAGBloque *antes, const DepGrafo *g, RenConteo *s)
{
    DAGBloque despues={0}; DepGrafo nuevo={0}; int ok=0;
    if (!dag_construir(copia,b,&despues) || !dep_analizar_bloque(copia,b,&nuevo)) goto fin;
    for (size_t k=0;k<antes->cantidad_instrucciones;++k) {
        const DAGInstruccion *a=&antes->instrucciones[k], *d=&despues.instrucciones[k];
        if (a->accesos.lecturas!=d->accesos.lecturas || a->accesos.escrituras!=d->accesos.escrituras) goto fin;
        for (size_t j=0;j<DAG_CAMPOS;++j) if (a->accesos.lecturas & campos[j]) {
            if (a->usos[j].definicion!=d->usos[j].definicion) goto fin;
            if (a->usos[j].definicion==DAG_NULO &&
                !iguales(dep_campo(&p->instructions[a->indice],campos[j]),
                         dep_campo(&copia->instructions[d->indice],campos[j]))) goto fin;
        }
    }
    /* El analizador emite RAW en orden del destino, y de sus campos.
       Comparar secuencia de pares, ignorando nombre (puede cambiar). */
    size_t x=0,y=0;
    for (;;) {
        while (x<g->cantidad && g->aristas[x].tipo!=DEP_RAW) ++x;
        while (y<nuevo.cantidad && nuevo.aristas[y].tipo!=DEP_RAW) ++y;
        if (x==g->cantidad || y==nuevo.cantidad) {
            if (x!=g->cantidad || y!=nuevo.cantidad) goto fin;
            break;
        }
        if (g->aristas[x].origen!=nuevo.aristas[y].origen ||
            g->aristas[x].destino!=nuevo.aristas[y].destino) goto fin;
        ++x; ++y;
    }
    RenConteo a={0},d={0}; contar(g,&a); contar(&nuevo,&d);
    if (a.memoria!=d.memoria || a.barreras!=d.barreras || d.war+d.waw>a.war+a.waw) goto fin;
    contar(&nuevo,s); ok=1;
fin:
    dag_liberar(&despues); dep_liberar(&nuevo); return ok;
}

static int renombrar_bloque(const IRProgram *p, IRProgram *copia,
                            const BasicBlockCFG *cfg, size_t id,
                            unsigned char *tocadas, RenEstadisticas *s)
{
    const BasicBlock *b=&cfg->blocks[id];
    DAGBloque d={0}; DepGrafo g={0}; int ok=0;
    if (!dag_construir(p,b,&d) || !dep_analizar_bloque(p,b,&g)) goto fin;
    contar(&g,&s->antes);
    for (size_t k=0;k<d.cantidad_instrucciones;++k) {
        const DAGInstruccion *i=&d.instrucciones[k];
        size_t def=i->indice;
        if (i->accesos.barrera || i->accesos.escrituras!=DEP_RESULT) continue;
        const char *nombre=p->instructions[def].result;
        size_t siguiente=DAG_NULO; int war=0, waw_previa=0;
        for (size_t j=0;j<g.cantidad;++j) {
            const DepArista *a=&g.aristas[j];
            if (!iguales(a->nombre,nombre)) continue;
            if (a->tipo==DEP_WAW && a->origen==def) siguiente=a->destino;
            if (a->tipo==DEP_WAW && a->destino==def) waw_previa=1;
            if (a->tipo==DEP_WAR && a->destino==def) war=1;
        }
        if (siguiente!=DAG_NULO) {
            /* Regla principal: la definicion vieja muere en otra escritura
               del MISMO bloque, sin efectos observables en medio. */
            if (!temporal(nombre) && !escalar_local_unico(p,def,nombre)) continue;
            if (!sin_barreras(&d,def,siguiente)) continue;
            if (!aplicar(p,copia,&d,def,siguiente,nombre,tocadas,s)) goto fin;
        } else if (war && !waw_previa && temporal(nombre)) {
            /* WAR pura: solo un temporal privado, sin lecturas en otros
               bloques ni caminos de regreso al bloque. Conservar las
               ultimas definiciones de variables del usuario. */
            if (leido_fuera(p,b,nombre)) continue;
            int ciclo=vuelve_al_bloque(cfg,id);
            if (ciclo<0) goto fin;
            if (ciclo) continue;
            if (!aplicar(p,copia,&d,def,b->end,nombre,tocadas,s)) goto fin;
        }
    }
    if (!validar(p,copia,b,&d,&g,&s->despues)) goto fin;
    ++s->bloques; ok=1;
fin:
    dag_liberar(&d); dep_liberar(&g); return ok;
}

int renombrar_programa(IRProgram *p, RenEstadisticas *out)
{
    if (!out) return 0;
    *out=(RenEstadisticas){0};
    if (!p || (p->count && !p->instructions)) return 0;
    if (!p->count) return 1;
    IRProgram copia={0}; RenEstadisticas stats={0}; int ok=0;
    BasicBlockCFG *cfg=bb_build(p);
    unsigned char *tocadas=calloc(p->count,1);
    if (!cfg || !tocadas || !bb_build_cfg(cfg) || !clonar(p,&copia)) goto fin;
    for (size_t k=0;k<cfg->block_count;++k)
        if (!renombrar_bloque(p,&copia,cfg,k,tocadas,&stats)) goto fin;
    for (size_t i=0;i<p->count;++i) stats.instrucciones+=tocadas[i]!=0;
    *out=stats; ok=1;
fin:
    bb_free(cfg); free(tocadas);
    if (ok && stats.definiciones) { ir_free(p); *p=copia; }
    else ir_free(&copia);
    return ok;
}

void ren_imprimir(const RenEstadisticas *s, FILE *out)
{
    if (!s || !out) return;
    fprintf(out,"Bloques=%zu Definiciones=%zu Usos=%zu Instrucciones modificadas=%zu\n",
            s->bloques,s->definiciones,s->usos,s->instrucciones);
    fprintf(out,"RAW: %zu -> %zu\nWAR: %zu -> %zu\nWAW: %zu -> %zu\nMEMORIA: %zu -> %zu\nBARRERA: %zu -> %zu\n",
            s->antes.raw,s->despues.raw,s->antes.war,s->despues.war,s->antes.waw,s->despues.waw,
            s->antes.memoria,s->despues.memoria,s->antes.barreras,s->despues.barreras);
}
