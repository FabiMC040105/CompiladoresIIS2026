/* Autocompletado contextual: exclusivamente FIRST(pila) de LL(1).
 * Uso: ide_completion <prefijo> <archivo-con-texto-antes-del-cursor>
 * Los nombres son solamente etiquetas de visualización de los terminales.
 * La decisión de ofrecer un candidato la toma LL1_TABLE en ll1_predict.c.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ll1_predict.h"
#include "ide_symbols.h"
#define MAX_SOURCE (1024u*1024u)
#define MAX_RESULTS 12

static const struct { const char *literal; int token; } displayed_keywords[] = {
    {"#traigase",T_TRAIGASE}, {"defina",T_DEFINA}, {"entero",T_ENTERO},
    {"vof",T_VOF}, {"vacio",T_VACIO}, {"principal",T_PRINCIPAL},
    {"lista",T_LISTA}, {"matriz",T_MATRIZ}, {"si",T_SI},
    {"sino",T_SINO}, {"sinosi",T_SINOSI}, {"mientras",T_MIENTRAS},
    {"para",T_PARA}, {"devuelva",T_DEVUELVA}, {"rompa",T_ROMPA},
    {"verdadero",T_TRUE}, {"falso",T_FALSE}
};
static int valid_prefix(const char *s) {
    size_t n=strlen(s);
    if(n<2 || n>39) return 0;
    for(size_t i=0;i<n;i++) {
        unsigned char c=(unsigned char)s[i];
        if((i==0 && c=='#') || isalpha(c) || (i && (isdigit(c)||c=='_')))continue;
        return 0;
    }
    return 1;
}
int main(int argc,char **argv) {
    if(argc!=3 || !valid_prefix(argv[1]))return 2;
    FILE *f=fopen(argv[2],"rb");if(!f)return 3;
    char *context=malloc(MAX_SOURCE+1);if(!context){fclose(f);return 3;}
    size_t len=fread(context,1,MAX_SOURCE+1,f);fclose(f);
    if(len>MAX_SOURCE){free(context);return 3;}
    LL1Result r;
    int ran=ll1_analyze(context,len,&r,NULL);
    if(!ran){free(context);return 3;}
    if(!r.ok){free(context);return 0;} /* Nunca caer al antiguo autocompletado por prefijo. */
    int shown=0;
    for(size_t i=0;i<sizeof(displayed_keywords)/sizeof(displayed_keywords[0]);i++) {
        int token=displayed_keywords[i].token;
        const char *word=displayed_keywords[i].literal;
        if(!ll1_expects(&r,token) || strncmp(word,argv[1],strlen(argv[1])) ||
           !strcmp(word,argv[1]))continue;
        printf("%s\t%s\n",word,LL1_TERMINAL_NAMES[token]);
        if(++shown>=MAX_RESULTS)break;
    }
    /* Solo cuando FIRST(pila) contiene ID consultamos nombres declarados.
     * La tabla de símbolos NUNCA genera sugerencias por sí sola. */
    if(shown<MAX_RESULTS && ll1_expects(&r,T_ID)) {
        IDESymbol names[MAX_RESULTS];
        size_t n=ide_visible_symbols(context,len,argv[1],names,MAX_RESULTS-shown);
        for(size_t i=0;i<n;i++)printf("%s\t%s\n",names[i].name,names[i].kind);
    }
    free(context);
    return 0;
}
