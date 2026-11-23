#!/usr/bin/env bash
# Prueba portable del backend IR sin Flex/Bison ni simulador RTL.
# No reemplaza las pruebas completas validar_feist_ir.sh en el entorno del proyecto.
set -Eeuo pipefail
SRC="$(cd "$(dirname "$0")" && pwd)"
cd "$SRC"
OUT="evidencia-vliw/.make-tests"
mkdir -p "$OUT"
cc -std=c11 -Wall -Wextra -Werror -g -O0 \
  -o "$OUT/prueba_codegen_ir_runtime" \
  pruebas/test_codegen_ir_runtime.c codegen_ir.c codegen.c ir.c ir_ast.c \
  ast.c asm_emitter.c scheduler.c memoria.c \
  spill_transform.c asignacion_registros.c grafo_interferencia.c \
  liveness.c basic_blocks.c dependencias.c
cc -std=c11 -Wall -Wextra -Werror -g -O0 \
  -o "$OUT/resolver_asm_ir" resolver_asm_main.c resolucion_etiquetas.c etiquetas.c
for entrada in 'factorial 65528 120' 'para 65532 8' \
               'mientras 65532 6' 'condicional 65532 42' \
               'aritmetica 65528 6' 'comparaciones 65528 3' \
               'factorial_recursivo 65528 120' \
               'lista_parametro 65528 17' \
               'ast_factorial 65528 120' 'ast_para 65532 8' \
               'spill_chaitin 65528 55'; do
    read -r nombre dir esperado <<<"$entrada"
    asm="$OUT/${nombre}_backend_ir.asm"
    "$OUT/prueba_codegen_ir_runtime" "$nombre" "$asm"
    python3 pruebas/simular_asm_ir.py "$asm" "$dir" "$esperado"
    "$OUT/resolver_asm_ir" "$asm" "$OUT/${nombre}_backend_ir_resuelto.asm" \
        > "$OUT/${nombre}_resolucion.log" 2>&1
    echo "PASS: etiquetas ASM $nombre"
done
printf '\nPASS: once pruebas de backend IR, Chaitin/spilling y resolucion de etiquetas\n'
