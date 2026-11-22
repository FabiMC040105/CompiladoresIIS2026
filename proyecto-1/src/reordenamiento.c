#include "reordenamiento.h"
#include "scheduler.h" /* Solo la distancia RAW del contrato existente. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Modelo heuristico: una operacion TAC por intervalo, peso unitario.
   La distancia RAW es orientativa, no una latencia real de TAC. */
#define DIST_RAW ((size_t)SCHEDULER_SEPARACION_RAW)
#define AUMENTO_PICO_LOCAL 1u

static size_t maximo(size_t a, size_t b) { return a > b ? a : b; }

/* Matriz de aristas por tipo: agrupa aristas multiples del mismo par.
   Las dependencias conservan la direccion desde el indice original menor. */
static int construir_matriz(const IRProgram *p, const BasicBlock *r,
                            unsigned char *m, size_t n)
{
    DepGrafo g = {0};
    if (!dep_analizar_bloque(p, r, &g)) return 0;
    int ok = 1;
    for (size_t k = 0; k < g.cantidad; ++k) {
        const DepArista *a = &g.aristas[k];
        if (a->origen < r->start || a->destino > r->end || a->origen >= a->destino) {
            ok = 0; break;
        }
        m[(a->origen-r->start)*n + a->destino-r->start] |= (unsigned char)a->tipo;
    }
    dep_liberar(&g);
    return ok;
}

/* P(i) = 1 + max(P(j)) para todos los sucesores j.
   El orden original ya es topologico, por eso basta el recorrido inverso. */
static void prioridades(const unsigned char *m, size_t n, size_t *p)
{
    for (size_t i = n; i-- > 0;) {
        size_t mayor = 0;
        for (size_t j = i+1; j < n; ++j)
            if (m[i*n+j]) mayor = maximo(mayor, p[j]);
        p[i] = 1 + mayor;
    }
}

/* List scheduling de IR, NO asignacion de slots ni generacion de NOP.
   Una instruccion disponible topologicamente puede tener espera RAW.
   Seleccion: menor espera estimada, mayor prioridad, menor indice original. */
static int seleccionar(const unsigned char *m, size_t n, const size_t *p,
                       size_t *orden)
{
    size_t *grado = calloc(n, sizeof(*grado));
    size_t *lista_desde = calloc(n, sizeof(*lista_desde));
    unsigned char *emitida = calloc(n, 1);
    int ok = 0;
    if (!grado || !lista_desde || !emitida) goto fin;
    for (size_t i=0; i<n; ++i)
        for (size_t j=i+1; j<n; ++j)
            if (m[i*n+j]) ++grado[j];
    size_t reloj = 0;
    for (size_t k=0; k<n; ++k) {
        size_t mejor = SIZE_MAX, espera = SIZE_MAX;
        for (size_t i=0; i<n; ++i) {
            if (emitida[i] || grado[i]) continue;
            size_t e = lista_desde[i] > reloj ? lista_desde[i]-reloj : 0;
            if (mejor == SIZE_MAX || e < espera || (e == espera && p[i] > p[mejor])) {
                mejor = i; espera = e;
            }
        }
        if (mejor == SIZE_MAX) goto fin;
        reloj += espera;
        orden[k] = mejor;
        emitida[mejor] = 1;
        for (size_t j=mejor+1; j<n; ++j) {
            unsigned char tipo = m[mejor*n+j];
            if (!tipo) continue;
            if (!grado[j]) goto fin;
            --grado[j];
            if (tipo & DEP_RAW)
                lista_desde[j] = maximo(lista_desde[j], reloj + DIST_RAW);
        }
        ++reloj;
    }
    ok = 1;
fin:
    free(grado); free(lista_desde); free(emitida);
    return ok;
}

/* Evalua un orden sin modificarlo. Mide esperas de una maquina abstracta
   que emite un TAC por intervalo con distancia DIST_RAW entre def y uso. */
static int medir_esperas(const unsigned char *m, size_t n, const size_t *orden,
                         size_t *total)
{
    size_t *desde = calloc(n, sizeof(*desde));
    if (!desde) return 0;
    size_t reloj = 0;
    *total = 0;
    for (size_t k=0; k<n; ++k) {
        size_t i = orden ? orden[k] : k;
        if (desde[i] > reloj) {
            *total += desde[i]-reloj;
            reloj = desde[i];
        }
        for (size_t j=i+1; j<n; ++j)
            if (m[i*n+j] & DEP_RAW) desde[j] = maximo(desde[j], reloj+DIST_RAW);
        ++reloj;
    }
    free(desde);
    return 1;
}

/* Presion LOCAL aproximada: cantidad de valores producidos en esta region
   que tienen un consumidor RAW posterior dentro de ella. Cada definicion
   aporta una unidad desde su emision hasta su ultimo uso. No incluye valores
   de entrada/salida, memoria ni registros fisicos: NO sustituye a liveness. */
static int pico_local(const unsigned char *m, size_t n, const size_t *orden,
                      size_t *pico)
{
    size_t *pos = malloc(n*sizeof(*pos));
    size_t *altas = calloc(n, sizeof(*altas));
    size_t *bajas = calloc(n, sizeof(*bajas));
    if (!pos || !altas || !bajas) { free(pos); free(altas); free(bajas); return 0; }
    for (size_t k=0; k<n; ++k) pos[orden ? orden[k] : k] = k;
    for (size_t i=0; i<n; ++i) {
        size_t ultimo = pos[i];
        for (size_t j=i+1; j<n; ++j)
            if (m[i*n+j] & DEP_RAW) ultimo = maximo(ultimo, pos[j]);
        if (ultimo > pos[i]) { ++altas[pos[i]]; ++bajas[ultimo]; }
    }
    size_t vivos=0; *pico=0;
    for (size_t k=0; k<n; ++k) {
        vivos -= bajas[k]; /* Consumidores leen antes de producir. */
        vivos += altas[k];
        *pico = maximo(*pico,vivos);
    }
    free(pos); free(altas); free(bajas);
    return 1;
}

/* Probar que la lista es una permutacion topologica, sin perder aristas. */
static int orden_valido(const unsigned char *m, size_t n, const size_t *orden)
{
    size_t *pos=malloc(n*sizeof(*pos));
    if (!pos) return 0;
    for (size_t i=0;i<n;++i) pos[i]=SIZE_MAX;
    int ok=0;
    for (size_t k=0;k<n;++k) {
        if (orden[k]>=n || pos[orden[k]]!=SIZE_MAX) goto fin;
        pos[orden[k]]=k;
    }
    for (size_t i=0;i<n;++i)
        for (size_t j=i+1;j<n;++j)
            if (m[i*n+j] && pos[i]>=pos[j]) goto fin;
    ok=1;
fin:
    free(pos); return ok;
}

/* Solo aceptar si disminuye la espera estimada y la presion local no crece
   mas de una unidad. En empate se conserva el orden de entrada. */
static int plan_region(const IRProgram *p, const BasicBlock *r,
                       size_t *perm, ReoEstadisticas *s)
{
    size_t n=r->end-r->start+1;
    ++s->regiones;
    if (n<2) return 1;
    if (n>SIZE_MAX/n || n>SIZE_MAX/sizeof(size_t) || n>SIZE_MAX/(DIST_RAW+1)) return 0;
    unsigned char *m=calloc(n*n,1);
    size_t *prior=malloc(n*sizeof(*prior)), *orden=malloc(n*sizeof(*orden));
    size_t ea=0, ed=0, pa=0, pd=0;
    int ok=0;
    if (!m || !prior || !orden || !construir_matriz(p,r,m,n)) goto fin;
    prioridades(m,n,prior);
    if (!seleccionar(m,n,prior,orden) || !orden_valido(m,n,orden) ||
        !medir_esperas(m,n,NULL,&ea) || !medir_esperas(m,n,orden,&ed) ||
        !pico_local(m,n,NULL,&pa) || !pico_local(m,n,orden,&pd)) goto fin;
    if (ed<ea && pd>pa && pd-pa>AUMENTO_PICO_LOCAL) ++s->rechazadas_presion;
    else if (ed<ea) {
        ++s->regiones_optimizadas;
        for (size_t k=0;k<n;++k) {
            perm[r->start+k]=r->start+orden[k];
            s->instrucciones += orden[k]!=k;
        }
        goto registrar;
    }
    ed=ea; pd=pa;
registrar:
    s->esperas_antes+=ea; s->esperas_despues+=ed;
    s->pico_local_antes=maximo(s->pico_local_antes,pa);
    s->pico_local_despues=maximo(s->pico_local_despues,pd);
    ok=1;
fin:
    free(m); free(prior); free(orden); return ok;
}

/* Barreras inmoviles: optimizar cada region escalar entre ellas, dentro del
   bloque basico. Validar tambien todas las instrucciones que no se mueven. */
static int plan_bloque(const IRProgram *p, const BasicBlock *b,
                       size_t *perm, ReoEstadisticas *s)
{
    size_t inicio=b->start;
    for (size_t i=b->start;i<=b->end;++i) {
        DepAccesos a;
        if (!dep_accesos(&p->instructions[i],&a)) return 0;
        if (!a.barrera) continue;
        if (inicio<i) {
            BasicBlock r={b->id,inicio,i-1};
            if (!plan_region(p,&r,perm,s)) return 0;
        }
        inicio=i+1;
    }
    if (inicio<=b->end) {
        BasicBlock r={b->id,inicio,b->end};
        if (!plan_region(p,&r,perm,s)) return 0;
    }
    ++s->bloques;
    return 1;
}

/* Verificar el plan completo contra TODAS las aristas originales, incluidas
   las que atraviesan regiones. Ninguna barrera puede cambiar de posicion. */
static int validar_bloque(const IRProgram *p, const BasicBlock *b,
                          const size_t *perm, size_t *pos)
{
    for (size_t i=b->start;i<=b->end;++i) pos[i]=SIZE_MAX;
    for (size_t i=b->start;i<=b->end;++i) {
        size_t original=perm[i]; DepAccesos a;
        if (original<b->start || original>b->end || pos[original]!=SIZE_MAX ||
            !dep_accesos(&p->instructions[original],&a) || (a.barrera && i!=original)) return 0;
        pos[original]=i;
    }
    DepGrafo g={0};
    if (!dep_analizar_bloque(p,b,&g)) return 0;
    int ok=1;
    for (size_t k=0;k<g.cantidad;++k) {
        const DepArista *a=&g.aristas[k];
        if (pos[a->origen]>=pos[a->destino]) { ok=0; break; }
    }
    dep_liberar(&g); return ok;
}

int reordenar_programa(IRProgram *p, ReoEstadisticas *out)
{
    if (!out) return 0;
    *out=(ReoEstadisticas){0};
    if (!p || (p->count && !p->instructions)) return 0;
    if (!p->count) return 1;
    if (p->count>SIZE_MAX/sizeof(size_t) || p->count>SIZE_MAX/sizeof(IRInstruction) ||
        p->count>SIZE_MAX/(DIST_RAW+1)) return 0;
    BasicBlockCFG *cfg=bb_build(p);
    size_t *perm=malloc(p->count*sizeof(*perm)), *pos=malloc(p->count*sizeof(*pos));
    IRInstruction *copia=malloc(p->count*sizeof(*copia));
    ReoEstadisticas stats={0}; int ok=0;
    if (!cfg || !perm || !pos || !copia || !bb_build_cfg(cfg)) goto fin;
    for (size_t i=0;i<p->count;++i) perm[i]=i;
    for (size_t k=0;k<cfg->block_count;++k)
        if (!plan_bloque(p,&cfg->blocks[k],perm,&stats) ||
            !validar_bloque(p,&cfg->blocks[k],perm,pos)) goto fin;
    /* Copia superficial: cada cuadruplo completo conserva sus cadenas y
       source_line; no duplicar ni liberar cadenas. Commit solo al final. */
    if (stats.instrucciones) {
        for (size_t i=0;i<p->count;++i) copia[i]=p->instructions[perm[i]];
        memcpy(p->instructions,copia,p->count*sizeof(*copia));
    }
    *out=stats; ok=1;
fin:
    bb_free(cfg); free(perm); free(pos); free(copia); return ok;
}

void reo_imprimir(const ReoEstadisticas *s, FILE *out)
{
    if (!s || !out) return;
    fprintf(out,"Bloques=%zu Regiones=%zu Regiones optimizadas=%zu Instrucciones reordenadas=%zu\n",
            s->bloques,s->regiones,s->regiones_optimizadas,s->instrucciones);
    fprintf(out,"Esperas estimadas IR: %zu -> %zu\nPico local aproximado: %zu -> %zu\nRegiones rechazadas por presion: %zu\n",
            s->esperas_antes,s->esperas_despues,s->pico_local_antes,s->pico_local_despues,s->rechazadas_presion);
}
