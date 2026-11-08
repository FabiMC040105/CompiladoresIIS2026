/*
 * Sugerencias sintácticas CDFF basadas ÚNICAMENTE en LL(1), C11.
 *
 * 1. ll1_analyze() procesa la entrada completa con M[A,a] y una pila.
 * 2. Cuando M[A,a] es vacía o hay un terminal inesperado, devuelve
 *    FIRST(pila pendiente): conjunto de tokens ESPERADOS, sin heurísticas.
 * 3. Se prueban pequeñas reparaciones de delimitadores SOLO para tokens
 *    contenidos en ese conjunto. Cada una se vuelve a pasar por LL(1).
 * 4. Una corrección se ofrece si completa el análisis o permite consumir
 *    tokens originales adicionales. La localización visual no decide tokens.
 * 5. Si no existe una reparación única defendible, se muestra NO_FIX.
 *
 * El compilador Bison/Flex permanece intacto. No hay reglas basadas en
 * nombres de instrucciones, saltos de línea ni paréntesis contados a mano.
 * Protocolo: linea TAB columna_utf8 TAB LL1_INSERT|LL1_REPLACE|NO_FIX
 *            TAB texto_original TAB texto_nuevo TAB descripción.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "ll1_predict.h"

#define MAX_SOURCE (1024u * 1024u)
#define MAX_EDITS 20
#define MAX_ISSUES 40
#define EXTRA 256

typedef struct {
    size_t pos, remove;
    const char *insert;
    int tokens_added;
    int expected_token; /* símbolo elegido entre FIRST(pila) */
} Edit;
typedef struct {
    Edit edit;
    LL1Result after;
    int score;
    int ambiguous;
} Choice;
typedef struct {
    size_t original_pos;
    char expected[50], replacement[50], code[20], message[200];
} Issue;

static const char *readable(int tok) {
    switch(tok) {
        case T_EOF: return "fin del archivo";
        case T_FIN: return ":)";
        case T_RP: return ")";
        case T_RB: return "]";
        case T_RC: return "}";
        case T_ID: return "identificador";
        case T_NUM: return "número";
        default: return tok >= 0 && tok < T_COUNT ? LL1_TERMINAL_NAMES[tok] : "token desconocido";
    }
}

/* Adapta una EDICIÓN, no propone símbolos. El conjunto de símbolos
 * permitidos siempre proviene de FIRST(pila) en ll1_analyze(). */
static int apply_edit(const char *src,size_t n,Edit e,char *dest,size_t cap) {
    size_t ins=strlen(e.insert);
    if(e.pos>n || e.remove>n-e.pos || n-e.remove+ins+1>cap)return 0;
    memcpy(dest,src,e.pos);
    memcpy(dest+e.pos,e.insert,ins);
    memcpy(dest+e.pos+ins,src+e.pos+e.remove,n-e.pos-e.remove);
    dest[n-e.remove+ins]='\0';
    return 1;
}

/* La prioridad es una reparacion que permite llegar al estado de
 * aceptación completo. De otro modo debe avanzarse más allá del token
 * ORIGINAL conflictivo; consumir solo el símbolo insertado no basta. */
static int edit_score(const char *src,size_t n,LL1Result before,Edit e,
                      char *scratch,size_t cap,LL1Result *after) {
    if(!apply_edit(src,n,e,scratch,cap))return -1;
    size_t newlen=n-e.remove+strlen(e.insert);
    if(!ll1_analyze(scratch,newlen,after,NULL))return -1;
    if(ll1_is_complete(after))return 100000 + (int)e.tokens_added;
    if(!after->ok && after->consumed <= before.consumed+e.tokens_added)return -1;
    if(after->ok && before.found==T_EOF && after->consumed>before.consumed)
        return 1000 + after->consumed-before.consumed;
    if(after->position <= e.pos && !after->ok)return -1;
    return 10 + after->consumed-before.consumed-e.tokens_added;
}

static int expected_at_error(const LL1Result *r,int tok) {
    return tok>=0 && tok<T_COUNT && (r->expected & (UINT64_C(1)<<tok))!=0;
}

/* Genera alternativas SOLO de los terminales esperados por LL(1).
 * Los fragmentos compuestos son patrones de cierre, no gramática extra:
 * la tabla decide si cada patrón es válido al volver a analizarlo. */
static int propose(const char *src,size_t n,LL1Result before,
                   char *scratch,size_t cap,Choice *winner) {
    Edit options[24];int count=0;
    const size_t at=before.found==T_EOF?before.last_token_end:before.position;
#define ADD(POS,DEL,TEXT,COUNT,TOK) do { options[count++]=(Edit){(POS),(DEL),(TEXT),(COUNT),(TOK)}; } while(0)
    if(expected_at_error(&before,T_FIN)) {
        /* FIN = :) puede aparecer tras una expresión completa, jamás tras '='.
         * Anclarlo tras el token consumido, ANTES de comentarios o salto
         * de línea, evita insertarlo en la instrucción siguiente. */
        size_t fin_at=before.last_token_end;
        ADD(fin_at,0," :)",1,T_FIN);
        if(before.found==T_RP)ADD(before.position,before.token_end-before.position,":)",1,T_FIN);
        if(before.found==-1 && src[before.position]==':')
            ADD(before.position,1,":)",1,T_FIN);
    }
    if(expected_at_error(&before,T_RP)) {
        ADD(at,0,")",1,T_RP);
        ADD(before.last_token_end,0,") :)",2,T_RP);
    }
    if(expected_at_error(&before,T_RB)) {
        ADD(at,0,"]",1,T_RB);
        ADD(before.last_token_end,0,"] :)",2,T_RB);
    }
    if(expected_at_error(&before,T_RC)) {
        ADD(at,0,"} ",1,T_RC);
        if(before.found==T_EOF)ADD(at,0,"} :)",2,T_RC);
        if(before.found==T_RP)
            ADD(before.position,before.token_end-before.position,"} :)",2,T_RC);
        if(before.found==-1 && src[before.position]==':')
            ADD(before.position,1,"} :)",2,T_RC);
    }
    /* Un token adicional después de $ se puede retirar si LL(1) confirma
     * que TODO el resto del programa queda aceptado. */
    if(expected_at_error(&before,T_EOF) && before.found!=T_EOF && before.token_end>before.position)
        ADD(before.position,before.token_end-before.position,"",0,T_EOF);
#undef ADD
    int best=-1,twins=0;
    for(int i=0;i<count;i++) {
        LL1Result after;
        int score=edit_score(src,n,before,options[i],scratch,cap,&after);
        if(score<0)continue;
        /* Favorecer la reparación más pequeña ENTRE opciones con el mismo
         * nivel de validez; no inventar preferencias por palabras clave. */
        if(score>=100000)score-= (int)(strlen(options[i].insert)+options[i].remove);
        if(score>best) {*winner=(Choice){options[i],after,score,0};best=score;twins=0;}
        else if(score==best && (options[i].pos!=winner->edit.pos ||
                                strcmp(options[i].insert,winner->edit.insert) ||
                                options[i].remove!=winner->edit.remove))twins++;
    }
    winner->ambiguous=twins;
    return best>=0 && twins==0;
}

static void explain_expected(const LL1Result *r,char *out,size_t cap) {
    snprintf(out,cap,"LL(1): se esperaba ");
    int seen=0;
    for(int t=0;t<T_COUNT && seen<8;t++) {
        if(!((r->expected>>t)&UINT64_C(1)))continue;
        const char *term=readable(t);
        if(strlen(out)+strlen(term)+5>=cap)break;
        if(seen++)strcat(out,", ");
        strcat(out,term);
    }
    if(!seen)snprintf(out,cap,"LL(1): token inválido; revise la sintaxis.");
}

static void line_and_column(const char *source,size_t n,size_t at,int *line,int *column) {
    if(at>n)at=n;
    *line=1;size_t line_start=0;
    for(size_t i=0;i<at;i++)if(source[i]=='\n'){++*line;line_start=i+1;}
    *column=(int)(at-line_start);
}

static void emit(const char *source,size_t n,const Issue *issue) {
    int line,col;
    line_and_column(source,n,issue->original_pos,&line,&col);
    printf("%d\t%d\t%s\t%s\t%s\t%s\n",line,col,issue->code,
           issue->expected,issue->replacement,issue->message);
}

static int generate(const char *original,size_t length) {
    size_t capacity=length+MAX_EDITS*EXTRA+1;
    char *work=malloc(capacity),*scratch=malloc(capacity);
    size_t *origin=malloc(capacity*sizeof(size_t));
    if(!work||!scratch||!origin){free(work);free(scratch);free(origin);return 2;}
    memcpy(work,original,length);work[length]='\0';
    for(size_t i=0;i<=length;i++)origin[i]=i;
    size_t n=length;
    Issue issues[MAX_ISSUES];int nissues=0;
    int stopped=0;
    for(int round=0;round<MAX_EDITS;round++) {
        LL1Result state;
        if(!ll1_analyze(work,n,&state,NULL)){stopped=1;break;}
        if(ll1_is_complete(&state))break;
        Choice choice={0};
        if(!propose(work,n,state,scratch,capacity,&choice)) {
            if(nissues<MAX_ISSUES) {
                Issue *issue=&issues[nissues++];memset(issue,0,sizeof(*issue));
                issue->original_pos=origin[state.position<=n?state.position:n];
                strcpy(issue->code,"NO_FIX");
                if(choice.ambiguous)snprintf(issue->message,sizeof(issue->message),
                      "LL(1): varias reparaciones posibles; no se modifica automáticamente.");
                else explain_expected(&state,issue->message,sizeof(issue->message));
            }
            stopped=1;break;
        }
        Edit e=choice.edit;
        size_t at=e.pos,orig_at=origin[at],ins=strlen(e.insert),old=e.remove;
        if(at>n || old>n-at || ins>49 || old>49 || orig_at>length ||
           (old && (origin[at+old]-orig_at!=old ||
                    memcmp(work+at,original+orig_at,old)))) {stopped=1;break;}
        /* Emitimos posiciones del documento ORIGINAL. Las ediciones previas
         * sólo están en la copia simulada y no se aplican al usuario. */
        if(nissues<MAX_ISSUES) {
            Issue *issue=&issues[nissues++];memset(issue,0,sizeof(*issue));
            issue->original_pos=orig_at;
            snprintf(issue->code,sizeof(issue->code),"%s",old?"LL1_REPLACE":"LL1_INSERT");
            memcpy(issue->expected,original+orig_at,old);issue->expected[old]='\0';
            strcpy(issue->replacement,e.insert);
            snprintf(issue->message,sizeof(issue->message),
              "LL(1) espera %s; esta edición fue validada con la tabla predictiva.",
              readable(e.expected_token));
        }
        /* Mapeo de cada byte de la simulación a su posición original. */
        memmove(work+at+ins,work+at+old,n-at-old+1);
        memcpy(work+at,e.insert,ins);
        memmove(origin+at+ins,origin+at+old,(n-at-old+1)*sizeof(size_t));
        for(size_t k=0;k<ins;k++)origin[at+k]=orig_at;
        n=n-old+ins;
    }
    if(!stopped && nissues==0) {free(work);free(scratch);free(origin);return 0;}
    /* NO_FIX está subordinado a las reparaciones; no duplicar la misma línea
     * en la interfaz. El usuario aplica una corrección, luego reanaliza. */
    for(int i=0;i<nissues;i++) {
        int li,ci;line_and_column(original,length,issues[i].original_pos,&li,&ci);
        int already=0;
        for(int j=0;j<i;j++){
            int lj,cj;line_and_column(original,length,issues[j].original_pos,&lj,&cj);
            if(li==lj){already=1;break;}
        }
        if(!already)emit(original,length,&issues[i]);
    }
    free(work);free(scratch);free(origin);return 0;
}

int main(int argc,char **argv) {
    if(argc!=2)return 2;
    FILE *file=fopen(argv[1],"rb");if(!file)return 2;
    char *source=malloc(MAX_SOURCE+1);if(!source){fclose(file);return 2;}
    size_t n=fread(source,1,MAX_SOURCE+1,file);fclose(file);
    if(n>MAX_SOURCE){free(source);return 2;}
    source[n]='\0';
    int status=generate(source,n);
    free(source);return status;
}
