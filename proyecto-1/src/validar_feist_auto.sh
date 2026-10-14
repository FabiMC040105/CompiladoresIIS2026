#!/usr/bin/env bash
# Empaquetado FEIST automatico + ensamblador real + RTL real.
set -Eeuo pipefail
SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARQ="${ARQ_ROOT:-/mnt/c/Users/fmoreno/Desktop/Arquitectura1-IIS2026}"
for f in "$SRC/empaquetar_vliw_feist.py" "$SRC/demo_feist_sin_empaquetar.asm" "$SRC/tb_cuatro_slots_desde_mem.sv" "$ARQ/python/assembler.py"; do
  [[ -f "$f" ]] || { echo "FALTA: $f"; exit 2; }
done
[[ -d "$ARQ/src" ]] || { echo "FALTA: $ARQ/src"; exit 2; }
DEST="$SRC/evidencia-vliw/feist-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$DEST"
cd "$DEST"
PYTHONPATH="$SRC" python3 "$SRC/probar_empaquetador_feist.py"
python3 "$SRC/empaquetar_vliw_feist.py" "$SRC/demo_feist_sin_empaquetar.asm" -o demo_empaquetada.asm | tee empaquetado.log
python3 "$ARQ/python/assembler.py" demo_empaquetada.asm -o cuatro_slots.mem | tee ensamblado.log
python3 - <<'PY'
from pathlib import Path
import re
original=Path('empaquetado.log').read_text()
assert 'Empaquetados FEIST: 1' in original and 'Bundles con 4 unidades: 1' in original, 'No hubo empaquetado automatico completo'
s=Path('demo_empaquetada.asm').read_text().splitlines()
h=Path('cuatro_slots.mem').read_text().splitlines()
assert len(s)==len(h)==16, 'Se cambio numero de bundles'
assert any(all(x+':' in line for x in ['ALU','LSU','BRU','FEIST']) for line in s), 'No hay bundle de 4 unidades'
assert all(re.fullmatch('[0-9A-Fa-f]{32}',line) for line in h), 'Bundles invalidos'
assert any(all(int(line[8*i:8*(i+1)],16)>0 for i in range(4)) for line in h), 'No estan codificados los 4 slots'
print('PASS: empaquetamiento y 4 codificaciones simultaneas')
PY
iverilog -g2012 -s tb_cuatro_slots_desde_mem -o sim.vvp "$SRC/tb_cuatro_slots_desde_mem.sv" "$ARQ"/src/*.sv >iverilog.log 2>&1 || { echo 'ERROR IVERILOG'; tail -n 20 iverilog.log; exit 1; }
vvp sim.vvp | tee simulacion.log
if ! grep -q '^PASS: 4 UNIDADES EN PARALELO' simulacion.log; then
 echo 'FAIL: no se comprobo FEIST+ALU+LSU+BRU en hardware'; exit 1
fi
echo "RESULTADO: PASS FEIST automatico desde ASM de prueba"
echo "EVIDENCIA: $DEST"
echo 'PENDIENTE: generacion de FEIST desde fuentes .e del compilador'
