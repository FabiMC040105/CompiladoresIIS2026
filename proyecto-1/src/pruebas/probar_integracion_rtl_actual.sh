#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

ARQ="${ARQ_ROOT:-$HOME/TEC/Arqui1/Proyecto2/Arquitectura1-IIS2026}"
RUN="evidencia-vliw/rtl-integracion-final"
FUENTE="pruebas/integracion_3_opt.e"

fail() { echo "ERROR: $*" >&2; exit 1; }

for cmd in gcc python3 iverilog vvp; do
    command -v "$cmd" >/dev/null 2>&1 || fail "falta $cmd"
done
[[ -d "$ARQ/src" ]] || fail "ARQ_ROOT invalido: $ARQ"
[[ -f "$ARQ/src/top.sv" ]] || fail "falta src/top.sv en Arquitectura"
[[ -f "$FUENTE" ]] || fail "falta $FUENTE"
[[ -x ./cdff ]] || fail "falta ./cdff; ejecute make primero"

rm -rf "$RUN"
mkdir -p "$RUN"
cp "$FUENTE" "$RUN/programa.e"

echo "[1/4] Compilando programa con las optimizaciones e IR backend..."
./cdff --codegen-ir --unroll 2 --dce --renombrar-ir --reordenar-ir --spill-ir \
    --emit-ir -s -v -o "$RUN/programa.bin" "$RUN/programa.e" \
    >"$RUN/compilador.log" 2>&1

[[ -s "$RUN/programa.bin" ]] || fail "no se genero programa.bin"
[[ -s "$RUN/programa.asm" ]] || fail "no se genero programa.asm"

python3 - "$RUN/programa.bin" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1])
rows = p.read_text().splitlines()
if not rows or not all(re.fullmatch(r"[0-9A-Fa-f]{32}", x) for x in rows):
    raise SystemExit("ERROR: binario VLIW invalido")
print(f"PASS formato binario: {len(rows)} bundles de 128 bits")
PY

echo "[2/4] Validando semantica y hazards con el simulador del compilador..."
python3 pruebas/simular_asm_ir.py "$RUN/programa.asm" 0 35 \
    --solo-resumen --validar-raw >"$RUN/simulador_funcional.log"
grep -q 'Resultado:  CORRECTO: RAM\[0\] = 35' "$RUN/simulador_funcional.log" \
    || { cat "$RUN/simulador_funcional.log"; fail "resultado funcional incorrecto"; }
grep -Eq 'Riesgos RAW detectados[[:space:]]+0' "$RUN/simulador_funcional.log" \
    || { cat "$RUN/simulador_funcional.log"; fail "el ASM conserva riesgos RAW"; }
echo "PASS simulador funcional: resultado=35 y RAW=0"

echo "[3/4] Elaborando el RTL actual de Arquitectura..."
mapfile -t ARQ_SRC < <(find "$ARQ/src" -type f -name '*.sv' | sort)
((${#ARQ_SRC[@]} > 0)) || fail "no se encontraron fuentes SystemVerilog"

PROGRAM_ABS="$(realpath "$RUN/programa.bin")"
iverilog -g2012 -Wall -s tb_integracion_rtl_actual \
    -DPROGRAM_FILE=\"$PROGRAM_ABS\" \
    -o "$RUN/integracion_rtl.vvp" \
    pruebas/tb_integracion_rtl_actual.sv "${ARQ_SRC[@]}" \
    >"$RUN/iverilog.log" 2>&1 \
    || { tail -80 "$RUN/iverilog.log" >&2; fail "fallo elaboracion RTL"; }

echo "[4/4] Ejecutando binario del compilador sobre CPU + L1 + L2 + RAM..."
vvp "$RUN/integracion_rtl.vvp" | tee "$RUN/rtl.log"
grep -q 'PASS RTL: binario de CE1108 ejecuto correctamente' "$RUN/rtl.log" \
    || fail "la integracion RTL no produjo PASS"

echo
echo "PASS: INTEGRACION REAL CE1108 -> CE4301"
echo "Evidencias: $RUN"
