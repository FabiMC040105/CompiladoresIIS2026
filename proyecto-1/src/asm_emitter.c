#include "asm_emitter.h"

#include <stdarg.h>
#include <string.h>

static int escribir_formateado(FILE *archivo,
                               const char *prefijo,
                               const char *formato,
                               va_list args)
{
    if (prefijo != NULL && fputs(prefijo, archivo) == EOF)
        return 0;

    if (vfprintf(archivo, formato, args) < 0)
        return 0;

    if (fputc('\n', archivo) == EOF)
        return 0;

    return 1;
}

int asm_emitter_abrir(AsmEmitter *emisor, const char *ruta)
{
    if (emisor == NULL || ruta == NULL)
        return 0;

    emisor->archivo = fopen(ruta, "w");
    emisor->bundle_actual = 0;
    emisor->error = (emisor->archivo == NULL);

    return !emisor->error;
}

void asm_emitter_cerrar(AsmEmitter *emisor)
{
    if (emisor == NULL)
        return;

    if (emisor->archivo != NULL) {
        if (fclose(emisor->archivo) != 0)
            emisor->error = 1;
        emisor->archivo = NULL;
    }
}

size_t asm_bundle_actual(const AsmEmitter *emisor)
{
    return emisor != NULL ? emisor->bundle_actual : 0;
}

unsigned long asm_pc_actual(const AsmEmitter *emisor)
{
    return (unsigned long)(asm_bundle_actual(emisor) * 16u);
}

int asm_emitir_comentario(AsmEmitter *emisor, const char *formato, ...)
{
    va_list args;
    int ok;

    if (emisor == NULL || emisor->archivo == NULL || formato == NULL)
        return 0;

    va_start(args, formato);
    ok = escribir_formateado(emisor->archivo, "# ", formato, args);
    va_end(args);

    if (!ok)
        emisor->error = 1;

    return ok;
}

int asm_emitir_etiqueta(AsmEmitter *emisor, const char *etiqueta)
{
    if (emisor == NULL || emisor->archivo == NULL || etiqueta == NULL)
        return 0;

    if (fprintf(emisor->archivo, "%s:\n", etiqueta) < 0) {
        emisor->error = 1;
        return 0;
    }

    return 1;
}

int asm_emitir_bundle(AsmEmitter *emisor, const char *formato, ...)
{
    va_list args;
    int ok;

    if (emisor == NULL || emisor->archivo == NULL || formato == NULL)
        return 0;

    va_start(args, formato);
    ok = escribir_formateado(emisor->archivo, NULL, formato, args);
    va_end(args);

    if (!ok) {
        emisor->error = 1;
        return 0;
    }

    emisor->bundle_actual++;
    return 1;
}

int asm_emitir_slot(AsmEmitter *emisor,
                    const char *slot,
                    const char *formato_instruccion,
                    ...)
{
    char instruccion[256];
    va_list args;
    int escritos;

    if (emisor == NULL || slot == NULL || formato_instruccion == NULL)
        return 0;

    va_start(args, formato_instruccion);
    escritos = vsnprintf(instruccion,
                         sizeof(instruccion),
                         formato_instruccion,
                         args);
    va_end(args);

    if (escritos < 0 || (size_t)escritos >= sizeof(instruccion)) {
        emisor->error = 1;
        return 0;
    }

    return asm_emitir_bundle(emisor, "%s: %s", slot, instruccion);
}

int asm_emitir_nops(AsmEmitter *emisor, unsigned cantidad)
{
    unsigned i;

    for (i = 0; i < cantidad; i++) {
        /* Los slots no escritos quedan como NOP en assembler.py. */
        if (!asm_emitir_bundle(emisor, "ALU: NOP"))
            return 0;
    }

    return 1;
}
