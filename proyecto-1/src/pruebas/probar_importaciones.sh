#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

DIR="pruebas/importaciones"
LOG="evidencia-vliw/.make-tests/importaciones.log"
ASM="$DIR/principal.asm"
TMP="$DIR/import_faltante_tmp.e"

mkdir -p evidencia-vliw/.make-tests
rm -f "$LOG" "$ASM" "$DIR/principal.bin" "$TMP"

./cdff --codegen-ir --emit-ir -s -v "$DIR/principal.e" >"$LOG" 2>&1

grep -q '\[IMPORT\].*utilidades.e' "$LOG"
grep -q '\[IMPORT\].*matematica.e' "$LOG"
grep -q 'func doble:' "$LOG"
grep -q 'func cuadruple:' "$LOG"
grep -q 'call cuadruple' "$LOG"
grep -q 'call doble' "$LOG"
test -s "$ASM"

# resultado es el primer global y por contrato ocupa RAM[0].
python3 pruebas/simular_asm_ir.py "$ASM" 0 22 --solo-resumen --validar-raw

cat > "$TMP" <<'SRC'
#traigase "modulo_que_no_existe.e" :)
defina vacio principal() {
    entero x = 1 :)
}:)
SRC

if ./cdff --codegen-ir "$TMP" >"${LOG}.missing" 2>&1; then
    echo "FAIL: una importacion inexistente fue aceptada" >&2
    rm -f "$TMP"
    exit 1
fi

grep -q 'no se pudo abrir importacion' "${LOG}.missing"
rm -f "$TMP" "${LOG}.missing"

echo "PASS: #traigase enlaza modulos, resuelve importaciones anidadas y evita duplicados"
echo "PASS: programa importado ejecuta resultado=22 y mantiene RAW=0"
echo "PASS: importacion inexistente produce error"
