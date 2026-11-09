/* Tabla de símbolos ligera del IDE (NO la tabla semántica del compilador).
 * El analizador LL(1) debe permitir T_ID antes de llamar a esta función.
 * Solo identifica declaraciones completas anteriores al cursor y visibles.
 */
#ifndef IDE_SYMBOLS_H
#define IDE_SYMBOLS_H
#include <stddef.h>
typedef struct {
    char name[80];
    const char *kind;
    int scope;
} IDESymbol;
/* Devuelve <= capacity nombres distintos, en orden de proximidad de ámbito. */
size_t ide_visible_symbols(const char *code, size_t len, const char *prefix,
                           IDESymbol *out, size_t capacity);
#endif
