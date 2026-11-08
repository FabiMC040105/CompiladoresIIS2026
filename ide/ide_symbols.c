/* Extracción léxica de símbolos para autocompletado contextual.
 * No sustituye el análisis semántico: la compilación sigue siendo autoridad.
 * No cambia la gramática ni la tabla LL(1).
 */
#include "ide_symbols.h"
#include "ll1_predict.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKENS 65536
#define MAX_SYMBOLS 4096
#define MAX_SCOPES 2048

typedef struct { int type; size_t start, end; } Tok;
typedef struct { char name[80]; const char *kind; int scope; } Decl;
typedef struct { int parent; int is_for; } Scope;

static int token_is(const Tok *ts, size_t n, size_t at, int type) {
    return at<n && ts[at].type==type;
}
static int copy_name(char *out, const char *code, Tok token) {
    size_t n=token.end-token.start;
    if (n<1 || n>=80) return 0;
    memcpy(out,code+token.start,n);out[n]=0;return 1;
}
static void add(Decl *d, size_t *nd, const char *code, Tok token,
                const char *kind, int scope) {
    if (*nd>=MAX_SYMBOLS) return;
    char name[80];
    if (!copy_name(name,code,token)) return;
    /* Dentro del mismo bloque conservar la declaración más reciente. */
    for(size_t i=0;i<*nd;i++) if(d[i].scope==scope && !strcmp(d[i].name,name))return;
    strcpy(d[*nd].name,name);d[*nd].kind=kind;d[*nd].scope=scope;(*nd)++;
}
static int visible(int ancestor,int current,const Scope *scopes) {
    while(current>=0){if(current==ancestor)return 1;current=scopes[current].parent;}
    return 0;
}
static const char *decl_kind(const Tok *ts,size_t n,size_t i) {
    if (token_is(ts,n,i,T_ENTERO)) {
        if (token_is(ts,n,i+1,T_LISTA))return "lista";
        if (token_is(ts,n,i+1,T_MATRIZ))return "matriz";
        return "entero";
    }
    return "vof";
}
static size_t decl_name_idx(const Tok *ts,size_t n,size_t i) {
    size_t j=i+1;
    if(token_is(ts,n,j,T_LISTA)||token_is(ts,n,j,T_MATRIZ))j++;
    return token_is(ts,n,j,T_ID)?j:n;
}

size_t ide_visible_symbols(const char *code,size_t len,const char *prefix,
                           IDESymbol *out,size_t capacity) {
    if (!code||!prefix||!out||!capacity)return 0;
    Tok *ts=calloc(MAX_TOKENS,sizeof(*ts));
    Decl *decl=calloc(MAX_SYMBOLS,sizeof(*decl));
    Scope *scopes=calloc(MAX_SCOPES,sizeof(*scopes));
    if(!ts||!decl||!scopes){free(ts);free(decl);free(scopes);return 0;}
    size_t nt=0,off=0,start=0,nd=0;
    while(nt<MAX_TOKENS){
        int type=ll1_next_token(code,len,&off,&start);
        if(type==T_EOF||type<0)break;
        ts[nt++]=(Tok){type,start,off};
    }
    /* Scope 0 global. Los bloques abiertos crean scopes hijos. */
    int current=0, nsc=1, pending_function=0;
    Tok param_tokens[128];const char *param_kinds[128];size_t nparams=0;
    scopes[0].parent=-1;
    for(size_t i=0;i<nt;i++) {
        int t=ts[i].type;
        if(t==T_RC){
            if(current>0)current=scopes[current].parent;
            /* El ámbito de inicialización `para` dura hasta cerrar su bloque. */
            if(current>0 && scopes[current].is_for)current=scopes[current].parent;
            continue;
        }
        if(t==T_PARA && nsc<MAX_SCOPES){
            scopes[nsc].parent=current;scopes[nsc].is_for=1;current=nsc++;
        }
        if(t==T_LC){
            if(nsc<MAX_SCOPES){scopes[nsc].parent=current;current=nsc++;}
            if(pending_function){
                for(size_t p=0;p<nparams;p++)add(decl,&nd,code,param_tokens[p],param_kinds[p],current);
                nparams=0;pending_function=0;
            }
            continue;
        }
        /* defina tipo nombre (parametros) { ... } :)
         * Cargar los parámetros solo al abrir el bloque de SU función.
         * Las funciones son visibles desde el ámbito global. */
        if(t==T_DEFINA && i+3<nt &&
           (token_is(ts,nt,i+1,T_ENTERO)||token_is(ts,nt,i+1,T_VOF)||token_is(ts,nt,i+1,T_VACIO)) &&
           (token_is(ts,nt,i+2,T_ID)||token_is(ts,nt,i+2,T_PRINCIPAL)) &&
           token_is(ts,nt,i+3,T_LP)) {
            if(token_is(ts,nt,i+2,T_ID))add(decl,&nd,code,ts[i+2],"función",0);
            nparams=0;
            /* Solo reconocer parámetros del encabezado: TIPO [lista/matriz] ID ... */
            size_t j=i+4;
            for(;j<nt && ts[j].type!=T_RP && ts[j].type!=T_LC;j++){
                if((ts[j].type==T_ENTERO||ts[j].type==T_VOF) && nparams<128){
                    size_t k=decl_name_idx(ts,nt,j);
                    if(k<nt){param_tokens[nparams]=ts[k];param_kinds[nparams]="parámetro";nparams++;j=k;}
                }
            }
            /* Un encabezado sin cierre no introduce variables fantasma. */
            pending_function=(j<nt && ts[j].type==T_RP);
            continue;
        }
        /* Declaraciones, excluyendo la parte `defina TIPO` y firmas de función.
         * Las variables ya declaradas pueden aparecer en expresiones siguientes. */
        if(t==T_ENTERO||t==T_VOF){
            if(i>0 && ts[i-1].type==T_DEFINA)continue;
            size_t k=decl_name_idx(ts,nt,i);
            if(k==nt)continue;
            /* No confundir parámetro de encabezado con variable local. */
            int in_signature=0;
            for(size_t b=i;b>0 && i-b<80;b--){
                if(ts[b-1].type==T_LC||ts[b-1].type==T_FIN||ts[b-1].type==T_RC)break;
                if(ts[b-1].type==T_DEFINA){in_signature=1;break;}
            }
            if(!in_signature)add(decl,&nd,code,ts[k],decl_kind(ts,nt,i),current);
        }
    }
    /* Recorrer de la declaración más cercana hacia atrás y ocultar nombres.
     * No sugerir declaraciones de un scope hermano ya cerrado. */
    size_t count=0,plen=strlen(prefix);
    for(size_t i=nd;i>0 && count<capacity;i--){
        Decl *d=&decl[i-1];
        if(!visible(d->scope,current,scopes))continue;
        if(strncmp(d->name,prefix,plen)||!strcmp(d->name,prefix))continue;
        int duplicate=0;
        for(size_t j=0;j<count;j++)if(!strcmp(out[j].name,d->name)){duplicate=1;break;}
        if(duplicate)continue;
        strcpy(out[count].name,d->name);out[count].kind=d->kind;out[count].scope=d->scope;
        count++;
    }
    free(ts);free(decl);free(scopes);return count;
}
