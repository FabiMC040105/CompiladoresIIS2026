#ifndef ASM_EMITTER_H
#define ASM_EMITTER_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    FILE *archivo;
    size_t bundle_actual;
    int error;
} AsmEmitter;

int asm_emitter_abrir(AsmEmitter *emisor, const char *ruta);
void asm_emitter_cerrar(AsmEmitter *emisor);

size_t asm_bundle_actual(const AsmEmitter *emisor);
unsigned long asm_pc_actual(const AsmEmitter *emisor);

int asm_emitir_comentario(AsmEmitter *emisor, const char *formato, ...);
int asm_emitir_etiqueta(AsmEmitter *emisor, const char *etiqueta);
int asm_emitir_bundle(AsmEmitter *emisor, const char *formato, ...);
int asm_emitir_slot(AsmEmitter *emisor,
                    const char *slot,
                    const char *formato_instruccion,
                    ...);
int asm_emitir_nops(AsmEmitter *emisor, unsigned cantidad);

#endif
