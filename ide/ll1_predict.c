/* ALGORITMO PREDICTIVO NO RECURSIVO (diapositiva 5 del profesor).
 * 1) Buffer: tokenizador propio que replica las reglas de lexer.l.
 * 2) Pila: símbolos terminales y no terminales, desde $ PROGRAM.
 * 3) Tabla: LL1_TABLE[A][a] generada offline usando FIRST/FOLLOW.
 * 4) Se expande el no terminal más a la izquierda, sin backtracking.
 * 5) Al llegar al cursor calcula PRIMERO de la pila pendiente: sugerencias.
 */
#include "ll1_predict.h"
#include <ctype.h>
#include <string.h>

#define MAX_PREDICT_STACK 32768
#define MAX_PREDICT_STEPS 250000

/* El tokenizador NO ejecuta acciones de lexer.l / parser.y.
 * Comentarios ^ ... y ^^ ... ^^, cadenas de importación como comillas
 * independientes, operadores maximal-munch, identificadores [a-z]...
 */
int ll1_next_token(const char *s, size_t len, size_t *offset, size_t *start) {
    size_t i = *offset;
    for (;;) {
        while (i < len && (s[i]==' ' || s[i]=='\t' || s[i]=='\n' || s[i]=='\r')) ++i;
        if (i + 1 < len && s[i]=='^' && s[i+1]=='^') {
            i += 2;
            while (i + 1 < len && !(s[i]=='^' && s[i+1]=='^')) ++i;
            if (i + 1 >= len) { *start=i; *offset=len; return -1; }
            i += 2;
            continue;
        }
        if (i < len && s[i]=='^') {
            while (i < len && s[i]!='\n') ++i;
            continue;
        }
        break;
    }
    *start=i;
    if (i == len) { *offset=i; return T_EOF; }
    static const struct {const char *text; int token;} multi[] = {
        {"#traigase",T_TRAIGASE}, {".e",T_EXT_E}, {":)",T_FIN},
        {"==",T_COMP}, {"!=",T_COMP}, {"<=",T_COMP}, {">=",T_COMP},
        {"$$",T_AND}, {"--",T_OR}, {"**",T_POW}
    };
    for (size_t j=0; j<sizeof(multi)/sizeof(multi[0]); ++j) {
        size_t n=strlen(multi[j].text);
        if (i+n<=len && !strncmp(s+i,multi[j].text,n)) {
            *offset=i+n; return multi[j].token;
        }
    }
    if (s[i] >= 'a' && s[i] <= 'z') {
        size_t j=i+1;
        while (j<len && ((s[j]>='a' && s[j]<='z') ||
               (s[j]>='A' && s[j]<='Z') || (s[j]>='0' && s[j]<='9') || s[j]=='_')) ++j;
        static const struct {const char *word; int token;} words[] = {
            {"defina",T_DEFINA},{"entero",T_ENTERO},{"vof",T_VOF},{"vacio",T_VACIO},
            {"principal",T_PRINCIPAL},{"lista",T_LISTA},{"matriz",T_MATRIZ},
            {"si",T_SI},{"sino",T_SINO},{"sinosi",T_SINOSI},
            {"mientras",T_MIENTRAS},{"para",T_PARA},{"devuelva",T_DEVUELVA},
            {"rompa",T_ROMPA},{"verdadero",T_TRUE},{"falso",T_FALSE}
        };
        for (size_t k=0;k<sizeof(words)/sizeof(words[0]);k++)
            if (strlen(words[k].word)==j-i && !strncmp(s+i,words[k].word,j-i)) {
                *offset=j; return words[k].token;
            }
        *offset=j; return T_ID;
    }
    if (s[i]>='0' && s[i]<='9') {
        size_t j=i+1;
        while (j<len && isdigit((unsigned char)s[j])) ++j;
        *offset=j;
        if (s[i]=='0' && j-i>1) return -1;
        return T_NUM;
    }
    *offset=i+1;
    switch(s[i]) {
        case '"':return T_QUOTE;
        case '(':return T_LP;case ')':return T_RP;
        case '[':return T_LB;case ']':return T_RB;
        case '{':return T_LC;case '}':return T_RC;
        case ',':return T_COMMA;case '=':return T_ASSIGN;
        case '<':case '>':return T_COMP;
        case '+':return T_PLUS;case '-':return T_MINUS;
        case '*':return T_MUL;case '/':return T_DIV;case '%':return T_MOD;
        default:return -1;
    }
}

/* Esta es la predicción concreta para autocompletado.
 * Calcula PRIMERO(resto de pila), con ε y el siguiente símbolo.
 * La pila contiene el derivado más a la izquierda pendiente.
 */
static uint64_t expected_from_stack(const int *stack, int sp) {
    uint64_t result=0;
    for (int i=sp-1; i>=0; --i) {
        int symbol=stack[i];
        if (symbol < T_COUNT) { result |= UINT64_C(1)<<symbol; break; }
        int nt=symbol-T_COUNT;
        result |= LL1_FIRST[nt];
        if (!LL1_NULLABLE[nt]) break;
    }
    return result;
}
static const char *symbol_name(int id) {
    if (id<T_COUNT) return LL1_TERMINAL_NAMES[id];
    return LL1_NONTERMINAL_NAMES[id-T_COUNT];
}

int ll1_analyze(const char *source, size_t length, LL1Result *out, FILE *trace) {
    if (!source || !out) return 0;
    *out=(LL1Result){0};
    int stack[MAX_PREDICT_STACK],sp=0;
    stack[sp++]=T_EOF;
    stack[sp++]=T_COUNT+N_PROGRAM;
    size_t cursor=0,start=0;
    int token=ll1_next_token(source,length,&cursor,&start);
    unsigned steps=0;
    if (trace) fprintf(trace,"PASO\tTOPE\tLOOKAHEAD\tACCION\n");
    while (steps++<MAX_PREDICT_STEPS) {
        if (token==-1) {
            out->position=start;out->found=-1;out->token_end=cursor;
            out->expected=expected_from_stack(stack,sp);return 1;
        }
        /* No simular $ cuando el usuario está editando: devolver FIRST de
         * símbolos pendientes. Este es el punto de predicción del IDE. */
        if (token==T_EOF) {
            out->ok=1;out->position=start;out->found=token;out->token_end=cursor;
            out->expected=expected_from_stack(stack,sp);
            if (trace) fprintf(trace,"%u\t%s\t$\tPREDICCION (cursor)\n",steps,
                               sp?symbol_name(stack[sp-1]):"$");
            return 1;
        }
        if (sp==0) {
            out->position=start;out->found=token;out->token_end=cursor;return 1;
        }
        int top=stack[sp-1];
        if (top<T_COUNT) {
            if (top!=token) {
                out->position=start;out->found=token;out->token_end=cursor;
                out->expected=expected_from_stack(stack,sp);
                if(trace)fprintf(trace,"%u\t%s\t%s\tERROR: se esperaba %s\n",steps,
                                 symbol_name(top),symbol_name(token),symbol_name(top));
                return 1;
            }
            if(trace)fprintf(trace,"%u\t%s\t%s\tCOINCIDIR, avanzar\n",steps,
                             symbol_name(top),symbol_name(token));
            sp--;
            out->last_token_end=cursor;
            out->consumed++;
            token=ll1_next_token(source,length,&cursor,&start);
        } else {
            int nt=top-T_COUNT;
            /* CONSULTA REAL LL(1): M[A,a] selecciona UNA producción. */
            int production=LL1_TABLE[nt][token];
            if (production<0) {
                out->position=start;out->found=token;out->token_end=cursor;
                out->expected=expected_from_stack(stack,sp);
                if(trace)fprintf(trace,"%u\t%s\t%s\tERROR: M[A,a] vacía\n",steps,
                                 symbol_name(top),symbol_name(token));
                return 1;
            }
            const LL1Production *p=&LL1_PRODUCTIONS[production];
            if (sp-1+p->length > MAX_PREDICT_STACK) return 0;
            if(trace)fprintf(trace,"%u\t%s\t%s\tM[%s,%s] = P%d\n",steps,
                             symbol_name(top),symbol_name(token),symbol_name(top),
                             symbol_name(token),production);
            sp--;
            /* Insertar RHS invertido para expandir el NT más izquierdo. */
            for (int i=p->length-1;i>=0;--i) stack[sp++]=p->rhs[i];
        }
    }
    return 0;
}
