/*
 * CDFF Studio - sugerencias conservadoras, C11.
 * NO reemplaza el parser Flex/Bison ni modifica archivos.
 * Formato TSV: linea, columna(0-based), codigo, texto_esperado,
 * texto_nuevo, descripcion. Los reemplazos son ASCII sin tabs ni saltos.
 * NO_FIX nunca es una edicion. Se emite a lo sumo una propuesta por linea.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SOURCE (1024u*1024u)
#define MAX_LINES 16000
#define MAX_LINE 16384
#define MAX_STACK 4096
#define MAX_ISSUES 80

typedef struct { char kind; int line, col; } Open;
typedef struct {
    char text[MAX_LINE]; /* copia de linea, strings y comentarios enmascarados */
    int len, start, end, unmatched_paren, unmatched_paren_col;
    int has_term, has_unmatched_brace, ambiguous, in_multiline, multiline_open;
    char last;
} Line;
typedef struct { int line,col; char kind; } Candidate;
static Line *lines;
static Open stack[MAX_STACK];
static int sp=0, issues=0, line_count=1;
static unsigned char *line_reported;
static Candidate candidates[MAX_LINES];
static int candidate_count=0;

static void report(int line, int col, const char *code, const char *old,
                   const char *replacement, const char *message) {
    if (line<1 || line>line_count || issues>=MAX_ISSUES || line_reported[line]) return;
    if (strchr(old,'\t') || strchr(replacement,'\t') || strchr(message,'\t')) return;
    line_reported[line]=1;
    issues++;
    printf("%d\t%d\t%s\t%s\t%s\t%s\n",line,col,code,old,replacement,message);
}
static void hint(int line, int col, const char *message) {
    report(line,col,"NO_FIX","","",message);
}
static int word(const char *s, const char *w) {
    size_t n=strlen(w);
    return strncmp(s,w,n)==0 && (s[n]==' '||s[n]=='\t'||s[n]=='('||s[n]=='\0');
}
static const char *trimmed(const Line *l) { return l->text+l->start; }
static int exact(const Line *l, const char *s) {
    return (int)strlen(s)==l->end-l->start && !strncmp(l->text+l->start,s,(size_t)(l->end-l->start));
}
static int suffix(const Line *l, const char *s) {
    int len=(int)strlen(s);
    return l->end-l->start>=len && !strncmp(l->text+l->end-len,s,(size_t)len);
}
static int is_header(const Line *l) {
    const char *s=trimmed(l);
    return word(s,"defina")||word(s,"si")||word(s,"sino")||word(s,"sinosi")||
           word(s,"mientras")||word(s,"para")||word(s,"principal")||
           word(s,"#traigase");
}
static int statement(const Line *l) {
    const char *s=trimmed(l);
    if (!*s || *s=='#' || is_header(l) || strchr(s,'{')) return 0;
    if (*s=='}') return 1;
    if (word(s,"entero")||word(s,"vof")||word(s,"devuelva")||
        word(s,"rompa")||word(s,"lista")||word(s,"matriz")||word(s,"vacio")||
        word(s,"imprima")||word(s,"lea")) return 1;
    /* Asignaciones y llamadas como foo(...): la comprobación es orientativa. */
    if (strchr(s,'=')||strchr(s,'(')) return 1;
    if (isalpha((unsigned char)*s)||*s=='_') return 1;
    return 0;
}
static int ends_operator(const Line *l) {
    if(l->end<=l->start)return 0;
    char c=l->last;
    return strchr("=+-*/%^,!<>&|([{",c)!=NULL;
}
static int next_token_is_fin(int line) {
    for(int j=line+1;j<=line_count;j++) {
        Line *l=&lines[j];
        if(l->end==l->start)continue;
        return l->end-l->start>=2 && l->text[l->start]==':' && l->text[l->start+1]==')';
    }
    return 0;
}
static int previous_ended(int line) {
    for(int j=line-1;j>0;j--) {
        Line *l=&lines[j];
        if(l->end==l->start)continue;
        return suffix(l,":)");
    }
    return 0;
}
static int previous_was_brace(int line) {
    for(int j=line-1;j>0;j--) {
        Line *l=&lines[j];
        if(l->end==l->start)continue;
        return suffix(l,"}") && !suffix(l,"} :)");
    }
    return 0;
}

int main(int argc,char **argv) {
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb"); if(!f)return 2;
    char *src=malloc(MAX_SOURCE+2);
    if(!src){fclose(f);return 2;}
    size_t n=fread(src,1,MAX_SOURCE+1,f);fclose(f);
    if(n>MAX_SOURCE){free(src);return 2;}
    src[n]='\0';
    int total_lines=1;
    for(size_t i=0;i<n;i++)if(src[i]=='\n')total_lines++;
    if(total_lines>MAX_LINES){free(src);return 2;}
    lines=calloc((size_t)total_lines+1,sizeof(Line));
    line_reported=calloc((size_t)total_lines+1,1);
    if(!lines||!line_reported){free(src);free(lines);free(line_reported);return 2;}
    /* Primera pasada: enmascarar comentarios y strings, conservando columnas. */
    int in_string=0,block_comment=0,line_comment=0,escape=0, col=0;
    for(size_t i=0;i<n;i++) {
        char ch=src[i],next=(i+1<n)?src[i+1]:'\0';
        if(ch=='\n') {
            if(line_count>=MAX_LINES){free(src);free(lines);free(line_reported);return 2;}
            line_count++; col=0;line_comment=0;continue;
        }
        if(col>=MAX_LINE-2){free(src);free(lines);free(line_reported);return 2;}
        Line *l=&lines[line_count];
        char visible=ch;
        if(block_comment) {
            visible=' ';
            if(ch=='^' && next=='^') {
                l->text[col++]=' ';l->len++;
                ++i;ch=src[i];visible=' ';block_comment=0;
            }
        } else if(line_comment)visible=' ';
        else if(in_string) {
            visible=' ';
            if(escape)escape=0;
            else if(ch=='\\')escape=1;
            else if(ch=='"'){in_string=0;visible='V';}
        } else if(ch=='^' && next=='^') {
            l->text[col++]=' ';l->len++;++i;ch=src[i];visible=' ';block_comment=1;
        } else if(ch=='^') {visible=' ';line_comment=1;}
        else if(ch=='"') {visible='V';in_string=1;}
        if(ch=='\r')visible=' ';
        l->text[col++]=visible;l->len++;
    }
    /* El texto limpio solo sirve para ubicaciones; la sustitucion actua sobre el original. */
    for(int ln=1;ln<=line_count;ln++) {
        Line *l=&lines[ln];
        l->text[l->len]='\0';
        l->start=0;l->end=l->len;
        while(l->start<l->end && isspace((unsigned char)l->text[l->start]))l->start++;
        while(l->end>l->start && isspace((unsigned char)l->text[l->end-1]))l->end--;
        l->last=l->end>l->start?l->text[l->end-1]:'\0';
        l->unmatched_paren_col=-1;
        l->in_multiline=(sp>0 && (stack[sp-1].kind=='('||stack[sp-1].kind=='['));
        /* No colgar el ')' de una instruccion nueva si faltaba en
           una instruccion anterior. */
        if(sp>0 && stack[sp-1].kind=='(' && stack[sp-1].line<ln &&
           statement(l) && statement(&lines[stack[sp-1].line]) &&
           !lines[stack[sp-1].line].has_term &&
           !ends_operator(&lines[stack[sp-1].line])) {
            int prev=stack[sp-1].line;
            report(prev,lines[prev].end,"APPEND_RPAREN_FIN","",") :)",
                   "Faltan ')' y ':)' al final de la instrucción anterior.");
            sp--;
        }
        for(int col2=l->start;col2<l->end;col2++) {
            char c=l->text[col2],next=(col2+1<l->end)?l->text[col2+1]:'\0';
            if(c==':'&&next==')') {
                l->has_term=1;
                if(sp>0 && stack[sp-1].kind=='(') {
                    if(stack[sp-1].line==ln) {
                        int at=col2;
                        while(at>stack[sp-1].col+1 &&
                              isspace((unsigned char)l->text[at-1]))at--;
                        report(ln,at,"INSERT_RPAREN_BEFORE_FIN","",
                               at==col2?") ":")",
                               "Falta cerrar ')' antes de ':)'.");
                        sp--;
                    } else {
                        /* Multilínea: no asumir que este FIN_LINEA cierra
                           un paréntesis de una instrucción anterior. */
                        l->ambiguous=1;
                    }
                }
                col2++;continue;
            }
            if(c=='('||c=='{'||c=='[') {
                if(c=='{'&&sp>0&&stack[sp-1].kind=='(' && stack[sp-1].line==ln) {
                    report(ln,col2,"INSERT_RPAREN","",") ","Falta cerrar ')' antes de '{'.");
                    sp--;
                }
                if(sp<MAX_STACK)stack[sp++]=(Open){c,ln,col2};
                else hint(ln,col2,"Hay demasiados delimitadores abiertos para una corrección segura.");
            } else if(c==']') {
                if(sp>0&&stack[sp-1].kind=='[')sp--;
                else hint(ln,col2,"Hay un ']' sin apertura '[', revisa el índice.");
            } else if(c==')') {
                if(sp>0&&stack[sp-1].kind=='(')sp--;
                else {l->unmatched_paren++;l->unmatched_paren_col=col2;}
            } else if(c=='}') {
                int b=sp-1;
                while(b>=0&&stack[b].kind!='{')b--;
                if(b<0){l->has_unmatched_brace=1;hint(ln,col2,"Llave '}' sin apertura; revisa el bloque.");}
                else {
                    if(b!=sp-1) {
                        /* Un unico '(' sin cerrar en la ultima sentencia se
                           puede completar al final de esa misma linea. */
                        if(sp==b+2 && stack[sp-1].kind=='(' &&
                           stack[sp-1].line<ln &&
                           !lines[stack[sp-1].line].has_term &&
                           !line_reported[stack[sp-1].line] &&
                           !ends_operator(&lines[stack[sp-1].line])) {
                            int ol=stack[sp-1].line;
                            report(ol,lines[ol].end,"APPEND_RPAREN_FIN","",") :)",
                                   "Faltan ')' y ':)' al final de esta instrucción.");
                        } else {
                            l->ambiguous=1;
                            hint(ln,col2,"Hay paréntesis abiertos dentro del bloque: revisa sus cierres.");
                        }
                    }
                    sp=b;
                }
            }
        }
        l->multiline_open=(sp>0&&(stack[sp-1].kind=='('||stack[sp-1].kind=='['));
    }
    /* Segunda pasada: detección de ediciones de fin de instrucción.
       Nunca agregar :), si ya existe o empieza la línea siguiente. */
    for(int ln=1;ln<=line_count;ln++) {
        Line *l=&lines[ln];
        if(l->end==l->start||line_reported[ln])continue;
        const int p=l->end;
        /* ')' aislado suele ser una edicion parcial de '} :)', pero solo
           se ofrece la reparación en la pasada final, con contexto. */
        if((exact(l,":)")||exact(l,":")||exact(l,")")) && previous_ended(ln)) {
            if(candidate_count<MAX_LINES)
                candidates[candidate_count++]=(Candidate){ln,l->start,exact(l,":)")?'F':exact(l,":")?'C':'P'};
            continue;
        }
        if(l->has_unmatched_brace)continue;
        /* En llamadas o arreglos repartidos en varias lineas no inventar
           cierres mientras la expresion permanece abierta. */
        if((l->in_multiline || l->multiline_open) &&
           !suffix(l,":)") && l->last!='}')continue;
        /* '}'+terminador incompleto: insertar solo el carácter faltante. */
        if(suffix(l,"} :")||suffix(l,"}:")) {
            report(ln,p,"INSERT_RPAREN_AFTER_COLON","",")",
                   "Falta ')' para completar ':)' después de '}'.");continue;
        }
        if(suffix(l,"} )")||suffix(l,"})")) {
            report(ln,p-1,"INSERT_COLON_BEFORE_RPAREN","",":",
                   "Falta ':' antes de ')' después de '}'.");continue;
        }
        /* Un terminador completo se respeta. */
        if(suffix(l,":)"))continue;
        if(l->last=='}') {
            if(!next_token_is_fin(ln))report(ln,p,"APPEND_FIN_LINEA",""," :)",
                "Parece faltar ':)' después de la llave '}'.");
            continue;
        }
        if(!statement(l))continue;
        if(next_token_is_fin(ln))continue;
        if(l->last==':') {
            if(l->unmatched_paren)hint(ln,p,"Hay un paréntesis sin apertura y un ':' incompleto; revisar manualmente.");
            else report(ln,p,"INSERT_RPAREN_AFTER_COLON","",")",
                "Falta ')' para completar el terminador ':)'.");
            continue;
        }
        if(l->last==')'&&l->unmatched_paren==1) {
            report(ln,p-1,"INSERT_COLON_BEFORE_RPAREN","",":",
                "Hay un ')' final sin apertura; podría faltar ':' del terminador ':)'.");
            continue;
        }
        if(l->unmatched_paren) {
            hint(ln,l->unmatched_paren_col,"Hay un ')' sin apertura; no se corrige automáticamente.");
            continue;
        }
        if(ends_operator(l)) {
            hint(ln,p,"Instrucción incompleta: no hay una corrección segura todavía.");
            continue;
        }
        /* Contar '(' de esta linea aun abiertos (ej.: (2+3 :) / (2+3). */
        int opened=0;
        for(int k=0;k<sp;k++)if(stack[k].kind=='('&&stack[k].line==ln)opened++;
        if(l->has_term) {
            /* Si hay un ':)' anterior pero al final aparece otro token, no
               inventar un terminador: el parser debe aclarar ese caso. */
            hint(ln,p,"Hay texto después de un terminador; revisa la instrucción.");
        } else if(opened==1) {
            /* Solo proponer si la línea tiene un contenido tras '('. */
            report(ln,p,"APPEND_RPAREN_FIN","",") :)",
                "Faltan ')' y ':)' al final de esta instrucción.");
        } else if(opened>1) {
            hint(ln,p,"Hay varios paréntesis abiertos; verifica cómo deben cerrarse.");
        } else if(!l->has_term) {
            report(ln,p,"APPEND_FIN_LINEA",""," :)",
                "Parece faltar ':)' al final de esta instrucción.");
        }
    }
    /* Corrección específica: '(' abierto antes de ':)' dentro de una misma
       instrucción. Se realiza una pasada adicional antes de las llaves EOF. */
    /* Nota: en el análisis anterior, los paréntesis pueden seguir abiertos
       en sp después de leer todo el archivo, así que solo sugerimos en
       casos de línea con terminador y un único '(' abierto. */
    for(int k=sp-1;k>=0;k--) {
        Open o=stack[k];
        if(o.kind=='[' && !line_reported[o.line]) {
            hint(o.line,o.col,"Hay un '[' sin cerrar; revisa la matriz o el índice.");
            continue;
        }
        if(o.kind!='('||line_reported[o.line])continue;
        Line *l=&lines[o.line];
        const char *term=strstr(l->text+o.col,":)");
        if(term) {
            int at=(int)(term-l->text);
            report(o.line,at,"INSERT_RPAREN_BEFORE_FIN","",") ",
                   "Falta cerrar ')' antes del terminador ':)'.");
        } else {
            hint(o.line,o.col,"Paréntesis '(' sin cerrar: revisa dónde termina la expresión.");
        }
    }
    /* Buscar cierres sueltos solo si efectivamente falta una llave. */
    int open_braces=0;
    for(int k=0;k<sp;k++)if(stack[k].kind=='{')open_braces++;
    if(open_braces==1) {
        for(int c=candidate_count-1;c>=0 && open_braces>0;c--) {
            Candidate v=candidates[c];
            if(line_reported[v.line]||!previous_ended(v.line)||previous_was_brace(v.line))continue;
            if(v.kind=='F') {
                report(v.line,v.col,"INSERT_RBRACE_BEFORE_FIN","","} ",
                    "La línea conserva ':)': podría faltar '}' delante.");
            } else if(v.kind=='C') {
                report(v.line,v.col,"REPLACE_ORPHAN_COLON_WITH_BLOCK",":","} :)",
                    "Aquí quedó ':' solo; se puede restaurar '} :)'.");
            } else {
                report(v.line,v.col,"REPLACE_ORPHAN_RPAREN_WITH_BLOCK",")","} :)",
                    "Aquí quedó ')' solo; se puede restaurar '} :)'.");
            }
            open_braces--;
        }
    }
    /* Ningún cierre inventado si faltan varios o hay ambigüedad. */
    if(open_braces==1) {
        int last=line_count;
        while(last>1&&lines[last].end==lines[last].start)last--;
        if(!line_reported[last] && suffix(&lines[last],":)"))
            report(last,lines[last].end,"APPEND_BLOCK","","",
                "Falta una llave de bloque; puedes agregar '} :)' al final.");
        else if(!line_reported[last])
            hint(last,lines[last].end,"Hay una llave '{' sin cerrar; revisa el final del bloque.");
    } else if(open_braces>1) {
        hint(line_count,0,"Hay varios bloques sin cerrar; revisa la estructura antes de aplicar cambios.");
    }
    /* Cierres aislados con todas las llaves equilibradas: no insertar nada.
       Si una línea suelta ':' o ')' queda, se reporta como ambigua. */
    if(open_braces==0) {
        for(int c=0;c<candidate_count;c++) {
            Candidate v=candidates[c];
            if(line_reported[v.line] || previous_was_brace(v.line))continue;
            if(v.kind=='F')
                report(v.line,v.col,"REMOVE_ORPHAN_FIN_LINEA",":)","",
                       "Este ':)' parece estar duplicado después de una instrucción completa.");
            else hint(v.line,v.col,"Cierre aislado: revisa si sobra o falta parte de una instrucción.");
        }
    }
    free(src);free(lines);free(line_reported);
    return 0;
}
