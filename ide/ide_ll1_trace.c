/* Herramienta para DEFENSA: traza paso a paso de M[A,a].
 * Compilar (desde ide): cc -std=c11 -O2 -o ide_ll1_trace ide_ll1_trace.c ll1_predict.c
 * Uso: ./ide_ll1_trace ../proyecto-1/src/test1.e
 * Devuelve 0 si la cadena completa es válida sintácticamente, 1 si no.
 */
#include "ll1_predict.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if(argc!=2){fprintf(stderr,"Uso: %s archivo.e\n",argv[0]);return 2;}
    FILE *f=fopen(argv[1],"rb");
    if(!f){perror(argv[1]);return 2;}
    char *data=malloc(1024*1024+1);
    if(!data){fclose(f);return 2;}
    size_t n=fread(data,1,1024*1024+1,f);
    fclose(f);
    if(n>1024*1024){free(data);return 2;}
    data[n]='\0';
    LL1Result result;
    if(!ll1_analyze(data,n,&result,stdout)){
        fprintf(stderr,"Error interno en el análisis predictivo.\n");free(data);return 2;
    }
    printf("\nRESULTADO\t%s\n", result.ok ? "PREFIJO_VALIDO" : "ERROR");
    printf("TOKENS_CONSUMIDOS\t%d\n",result.consumed);
    printf("POSICION_BYTES\t%zu\n",result.position);
    printf("LOOKAHEAD\t%s\n",result.found<0?"ERROR_LEXICO":LL1_TERMINAL_NAMES[result.found]);
    printf("TOKENS_ESPERADOS\t");
    int first=1;
    for(int i=0;i<T_COUNT;i++)if(result.expected & (UINT64_C(1)<<i)){
        printf("%s%s",first?"":", ",LL1_TERMINAL_NAMES[i]);first=0;
    }
    printf("\n");
    int accepted = result.ok && ll1_expects(&result,T_EOF);
    printf("CADENA_COMPLETA\t%s\n",accepted?"ACEPTAR":"AUN_INCOMPLETA_O_INVALIDA");
    free(data);
    return accepted ? 0 : 1;
}
