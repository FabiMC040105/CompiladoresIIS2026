/* CDFF Studio - detector de clones Tipo I y II ENTRE ARCHIVOS en C11.
 * Tipo I: mismos tokens, ignorando espacios y comentarios ^ / ^^...^^.
 * Tipo II: mismos tokens al normalizar identificadores y literales; se
 * exige una sustitucion de identificadores coherente dentro del fragmento.
 * Se comparan fragmentos de >=3 lineas de codigo y >=18 tokens.
 * Uso: ide_clones lexer.l etiqueta1 ruta1 etiqueta2 ruta2 [...]
 * Salida TSV: tipo, etiquetaA, inicioA, finA, etiquetaB, inicioB, finB
 * No es detector de clones semanticos ni reconoce codigo reordenado.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_FILES 25
#define MAX_LINES 650
#define MAX_TOKENS 120
#define WORD 72
#define MAX_MATCHES 60

typedef struct {char val[WORD];unsigned char kind;} Token;
typedef struct {int lineno,n;Token t[MAX_TOKENS];} Line;
typedef struct {char label[100];Line *lines;int n;} File;
static char keywords[100][WORD];static int nkeywords;
static int is_ident_char(int c){return isalnum((unsigned char)c)||c=='_';}
static int is_word(const char *s){if(!(islower((unsigned char)s[0])||s[0]=='#'))return 0;for(const char *p=s+1;*p;p++)if(!is_ident_char(*p))return 0;return 1;}
static void keywords_from(const char *path){
 FILE *f=fopen(path,"r");if(!f)return;char buf[4096];
 while(fgets(buf,sizeof buf,f)){
   char *p=buf;while(*p==' '||*p=='\t')p++;
   if(*p!='"')continue;
   p++;char w[WORD];int k=0;
   while(*p&&*p!='"'&&k<WORD-1)w[k++]=*p++;
   w[k]=0;if(*p!='"'||!is_word(w)||nkeywords>=100)continue;
   strcpy(keywords[nkeywords++],w);
 }
 fclose(f);
}
static int keyword(const char *w){for(int i=0;i<nkeywords;i++)if(!strcmp(w,keywords[i]))return 1;return 0;}
static void append_tok(Line *line,const char *w,int kind){
 if(line->n>=MAX_TOKENS)return;
 Token *t=&line->t[line->n++];snprintf(t->val,sizeof t->val,"%s",w);t->kind=kind;
}
static void tokenize(Line *l,char *buf,int *block){
 size_t i=0, n=strlen(buf);
 while(i<n){
   char c=buf[i];if(isspace((unsigned char)c)){i++;continue;}
   if(*block){if(c=='^'&&i+1<n&&buf[i+1]=='^'){*block=0;i+=2;}else i++;continue;}
   if(c=='^'){if(i+1<n&&buf[i+1]=='^'){*block=1;i+=2;}else break;continue;}
   if(isalpha((unsigned char)c)||c=='_'||(c=='#'&&i+1<n&&isalpha((unsigned char)buf[i+1]))){
     char w[WORD];int k=0;
     while(i<n && (is_ident_char(buf[i])||(k==0&&buf[i]=='#'))){if(k<WORD-1)w[k++]=buf[i];i++;}
     w[k]=0;append_tok(l,w,keyword(w)?0:1);continue;
   }
   if(isdigit((unsigned char)c)){
     char w[WORD];int k=0;
     while(i<n&&isdigit((unsigned char)buf[i])){if(k<WORD-1)w[k++]=buf[i];i++;}
     w[k]=0;append_tok(l,w,2);continue;
   }
   if(c=='"'){
     char w[WORD];int k=0;w[k++]=buf[i++];
     while(i<n && k<WORD-2){char x=buf[i++];w[k++]=x;if(x=='"'&&w[k-2]!='\\')break;}
     w[k]=0;append_tok(l,w,2);continue;
   }
   char op[4]={c,0,0,0};
   if(i+1<n && ((c==':'&&buf[i+1]==')') || (c=='='&&buf[i+1]=='=') ||
      (c=='!'&&buf[i+1]=='=') || (c=='>'&&buf[i+1]=='=') ||
      (c=='<'&&buf[i+1]=='=') || (c=='*'&&buf[i+1]=='*') ||
      (c=='$'&&buf[i+1]=='$') || (c=='-'&&buf[i+1]=='-'))){op[1]=buf[++i];}
   append_tok(l,op,0);i++;
 }
}
static int load(File *f,const char *label,const char *path){
 snprintf(f->label,sizeof f->label,"%s",label);
 f->lines=calloc(MAX_LINES,sizeof(Line));if(!f->lines)return 0;
 FILE *fp=fopen(path,"r");if(!fp)return 0;
 char buf[8192];int block=0,line=0;
 while(fgets(buf,sizeof buf,fp)&&line<MAX_LINES){
   line++;Line tmp={0};tmp.lineno=line;tokenize(&tmp,buf,&block);
   if(tmp.n)f->lines[f->n++]=tmp;
 }
 fclose(fp);return 1;
}
/* Compare fragmentos de lineas y preserve una asignacion coherente de nombres.
 * kind=0 fijo, 1 identificador, 2 literal. */
static int compatible(const Line *a,const Line *b,char mapsA[][WORD],char mapsB[][WORD],int *mcount,int *diff){
 if(a->n!=b->n)return 0;
 for(int i=0;i<a->n;i++){
   const Token *x=&a->t[i],*y=&b->t[i];if(x->kind!=y->kind)return 0;
   if(x->kind==0){if(strcmp(x->val,y->val))return 0;continue;}
   if(strcmp(x->val,y->val))(*diff)++;
   if(x->kind==2)continue;
   int match=0;
   for(int k=0;k<*mcount;k++){
     if(!strcmp(mapsA[k],x->val)||!strcmp(mapsB[k],y->val)){
       if(strcmp(mapsA[k],x->val)||strcmp(mapsB[k],y->val))return 0;
       match=1;break;
     }
   }
   if(!match){if(*mcount>=400)return 0;
      snprintf(mapsA[*mcount],WORD,"%s",x->val);
      snprintf(mapsB[*mcount],WORD,"%s",y->val);(*mcount)++;
   }
 }
 return 1;
}
static int match(const File *a,int i,const File *b,int j,int *lines,int *total,int *diff){
 char ma[400][WORD],mb[400][WORD];int nmap=0,count=0,tok=0,d=0;
 while(i+count<a->n && j+count<b->n && count<80){
   if(!compatible(&a->lines[i+count],&b->lines[j+count],ma,mb,&nmap,&d))break;
   tok+=a->lines[i+count].n;count++;
 }
 *lines=count;*total=tok;*diff=d;return count>=3&&tok>=18;
}
int main(int argc,char **argv){
 if(argc<6||(argc-2)%2 || (argc-2)/2>MAX_FILES)return 2;
 keywords_from(argv[1]);
 File files[MAX_FILES]={0};int nf=(argc-2)/2;
 for(int f=0;f<nf;f++)if(!load(&files[f],argv[2+2*f],argv[3+2*f]))return 3;
 int output=0;
 for(int a=0;a<nf;a++)for(int b=a+1;b<nf;b++){
   File *fa=&files[a],*fb=&files[b];
   for(int i=0;i<fa->n;i++)for(int j=0;j<fb->n;j++){
     int len,nt,diff;
     if(!match(fa,i,fb,j,&len,&nt,&diff))continue;
     /* Emitir solo maxima secuencia y evitar ventanas solapadas. */
     if(i>0&&j>0){int prev,toks,delta;if(match(fa,i-1,fb,j-1,&prev,&toks,&delta)&&prev>len)continue;}
     printf("%s\t%s\t%d\t%d\t%s\t%d\t%d\n",
       diff?"II":"I",fa->label,fa->lines[i].lineno,
       fa->lines[i+len-1].lineno,fb->label,fb->lines[j].lineno,
       fb->lines[j+len-1].lineno);
     if(++output>=MAX_MATCHES)goto done;
   }
 }
done:
 for(int f=0;f<nf;f++)free(files[f].lines);
 return 0;
}
