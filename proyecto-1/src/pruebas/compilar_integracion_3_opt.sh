#!/usr/bin/env bash
set -euo pipefail

# Copiar este script y el .e a proyecto-1/src/pruebas/.
# Ejecutarlo desde proyecto-1/src: bash pruebas/compilar_integracion_3_opt.sh

cd "$(dirname "$0")/.."
FUENTE="pruebas/integracion_3_opt.e"
SALIDA="evidencia-vliw/comparacion_3_opt"

if [[ ! -f "$FUENTE" ]]; then
    echo "ERROR: falta $FUENTE" >&2
    exit 1
fi

mkdir -p "$SALIDA"
cp "$FUENTE" "$SALIDA/base.e"
cp "$FUENTE" "$SALIDA/optimizado.e"

echo "[1/3] Compilando sin optimizaciones con codegen_ir..."
if ! make asm FILE="$SALIDA/base.e" \
        ARGS="--codegen-ir --emit-ir -v" \
        >"$SALIDA/base.log" 2>&1; then
    echo "Falló base. Revisá $SALIDA/base.log" >&2
    tail -35 "$SALIDA/base.log" >&2
    exit 1
fi

echo "[2/3] Compilando con unrolling, DCE, renombramiento y reordenamiento..."
if ! make asm FILE="$SALIDA/optimizado.e" \
        ARGS="--codegen-ir --unroll 2 --dce --renombrar-ir --reordenar-ir --emit-ir -v" \
        >"$SALIDA/optimizado.log" 2>&1; then
    echo "Falló optimizado. Revisá $SALIDA/optimizado.log" >&2
    tail -35 "$SALIDA/optimizado.log" >&2
    exit 1
fi

echo
printf '=== FASES DE OPTIMIZACION ===\n'
grep -E '^\[(OPT IR|UNROLL IR|SPILL IR)\]' "$SALIDA/optimizado.log" || true
printf '\n=== BUNDLES BINARIOS (1 LINEA = 1 BUNDLE) ===\n'
wc -l "evidencia-vliw/salidas-make/base.bin" "evidencia-vliw/salidas-make/optimizado.bin"

if [[ -f pruebas/simular_asm_ir.py ]]; then
    echo
    echo "[3/3] Simulando y comparando el ASM generado..."
    python3 pruebas/simular_asm_ir.py \
        "$SALIDA/base.asm" \
        --comparar "$SALIDA/optimizado.asm" \
        --solo-resumen --guardar --carpeta-salida "$SALIDA"
    echo "NOTA: resultados de memoria NO verificados hasta identificar la direccion de 'resultado'."
    echo "Valor matematico esperado = 35"
else
    echo "[3/3] Simulador no encontrado; compilaciones listas."
fi

echo
printf 'Logs y ASM: %s\n' "$SALIDA"
printf 'Binarios: evidencia-vliw/salidas-make/{base,optimizado}.bin\n'

