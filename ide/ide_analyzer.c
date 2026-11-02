/*
 * CDFF Studio - analizador auxiliar del IDE.
 * Implementa en C una sugerencia conservadora para FIN_LINEA.
 * Lee el diagnostico de cdff por stdin y el archivo fuente por argv[1].
 * Imprime: linea<TAB>columna<TAB>codigo<TAB>descripcion
 * No reemplaza el lexer/parser ni modifica el codigo fuente.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIAG_MAX 65536
#define LINE_MAX_BYTES 16384

static int extraer_linea(const char *mensaje) {
    const char *p = strstr(mensaje, "línea ");
    if (p) p += strlen("línea ");
    else {
        p = strstr(mensaje, "linea ");
        if (p) p += strlen("linea ");
        else {
            p = strstr(mensaje, "line ");
            if (!p) return -1;
            p += strlen("line ");
        }
    }
    if (!isdigit((unsigned char)*p)) return -1;
    char *end = NULL;
    long number = strtol(p, &end, 10);
    if (end == p || number <= 1 || number > 1000000) return -1;
    return (int)number;
}

static int empieza_con_palabra(const char *s, const char *palabra) {
    size_t n = strlen(palabra);
    return strncmp(s, palabra, n) == 0 && (s[n] == ' ' || s[n] == '\t' || s[n] == '\0');
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    char *diag = calloc(DIAG_MAX + 1, 1);
    if (!diag) return 2;
    size_t n = fread(diag, 1, DIAG_MAX, stdin);
    diag[n] = '\0';
    if (!strstr(diag, "FIN_LINEA") || !strstr(diag, "syntax error")) {
        free(diag);
        return 0;
    }
    int error_line = extraer_linea(diag);
    free(diag);
    if (error_line < 2) return 0;

    FILE *f = fopen(argv[1], "r");
    if (!f) return 2;
    char buf[LINE_MAX_BYTES];
    for (int line = 1; line < error_line; ++line) {
        if (!fgets(buf, sizeof(buf), f)) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);

    size_t end = strlen(buf);
    while (end > 0 && isspace((unsigned char)buf[end - 1])) --end;
    buf[end] = '\0';
    char *s = buf;
    while (*s == ' ' || *s == '\t') ++s;
    if (*s == '\0' || (s[0] == '/' && (s[1] == '/' || s[1] == '*'))) return 0;
    if (end >= 2 && buf[end - 2] == ':' && buf[end - 1] == ')') return 0;
    if (buf[end - 1] == '{' || buf[end - 1] == '}' || buf[end - 1] == ';') return 0;
    /* Solo hacer una sugerencia cuando la linea anterior parece una instruccion. */
    if (!strchr(s, '=') && !empieza_con_palabra(s, "devuelva") &&
        !empieza_con_palabra(s, "rompa")) return 0;
    /* En esta primera version no tocamos comentarios al final de linea. */
    if (strstr(s, "//") || strstr(s, "/*")) return 0;

    printf("%d\t%zu\tAPPEND_FIN_LINEA\tParece faltar :) al final de esta instrucción.\n", error_line - 1, end);
    return 0;
}
