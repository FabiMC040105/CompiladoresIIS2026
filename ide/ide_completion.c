/*
 * CDFF Studio: sugerencias de palabras reservadas, implementadas en C.
 * Los candidatos se leen del archivo lexer.l REAL del proyecto.
 * No modifica el compilador ni el código que edita el usuario.
 *
 * Uso: ide_completion /ruta/proyecto-1/src/lexer.l prefijo
 * Salida TSV: palabra<TAB>tipo<TAB>explicacion_del_token
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define LINE_CAP 4096
#define WORD_CAP 80
#define MAX_RESULTS 12

static int valid_prefix(const char *prefix) {
    size_t n = strlen(prefix);
    if (n < 2 || n >= WORD_CAP) return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)prefix[i];
        if ((i == 0 && c == '#') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || (i > 0 && (c == '_' || isdigit(c)))) {
            continue;
        }
        return 0;
    }
    return 1;
}

static int valid_word(const char *word) {
    size_t n = strlen(word);
    if (n < 2 || n >= WORD_CAP) return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)word[i];
        if ((i == 0 && c == '#') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || (i > 0 && (c == '_' || isdigit(c)))) {
            continue;
        }
        return 0;
    }
    return 1;
}

/* Reconoce literales entre comillas en el inicio de una regla de Flex.
 * No analiza comentarios ni fabrica palabras ajenas al lexer real. */
static int candidate_from_rule(const char *line, char *word) {
    const unsigned char *p = (const unsigned char *)line;
    while (*p == ' ' || *p == '\t') ++p;
    if (*p++ != '"') return 0;
    size_t used = 0;
    while (*p && *p != '"') {
        if (used >= WORD_CAP - 1 || *p == '\\' || *p == '\n' || *p == '\r') return 0;
        word[used++] = (char)*p++;
    }
    if (*p != '"' || used == 0) return 0;
    word[used] = '\0';
    return valid_word(word);
}

/* Nombre público del token leído de la acción de Flex en la misma línea o
 * las siguientes, hasta una nueva regla (solo para documentación visual). */
static void token_label(const char *line, char *out, size_t cap) {
    const char *p = strstr(line, "return ");
    if (!p) { snprintf(out, cap, "Palabra del lenguaje"); return; }
    p += 7;
    size_t i = 0;
    while ((isupper((unsigned char)*p) || *p == '_') && i + 1 < cap) out[i++] = *p++;
    out[i] = '\0';
    if (i == 0) snprintf(out, cap, "Palabra del lenguaje");
}

int main(int argc, char **argv) {
    if (argc != 3 || !valid_prefix(argv[2])) return 2;
    FILE *f = fopen(argv[1], "r");
    if (!f) return 3;
    const char *prefix = argv[2];
    size_t prefix_len = strlen(prefix);
    char line[LINE_CAP], word[WORD_CAP], token[WORD_CAP];
    int shown = 0;
    while (fgets(line, sizeof line, f)) {
        if (!candidate_from_rule(line, word)) continue;
        if (strncmp(word, prefix, prefix_len) != 0 || !strcmp(word, prefix)) continue;
        token_label(line, token, sizeof token);
        printf("%s\t%s\n", word, token);
        if (++shown >= MAX_RESULTS) break;
    }
    fclose(f);
    return 0;
}
