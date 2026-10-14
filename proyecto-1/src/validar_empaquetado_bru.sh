#!/usr/bin/env bash
# Regresion BRU SIN cambiar RTL, compilador, empaquetador original ni evidencias previas.
set -Eeuo pipefail
SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARQ="${ARQ_ROOT:-/mnt/c/Users/fmoreno/Desktop/Arquitectura1-IIS2026}"
BASE="${1:-$SRC/evidencia-vliw/validacion-20261012-230242-1874}"
[[ -d "$BASE" ]] || { echo "ERROR: no existe carpeta de evidencia: $BASE"; exit 2; }
BASE="$(cd "$BASE" && pwd)"
for f in "$SRC/empaquetar_vliw.py" "$SRC/empaquetar_vliw_bru.py" "$SRC/tb_diagnostico_compilador.sv" "$ARQ/python/assembler.py"; do
  [[ -f "$f" ]] || { echo "ERROR: falta $f"; exit 2; }
done
DEST="$SRC/evidencia-vliw/bru-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$DEST"
REPORT="$DEST/resumen_bru.txt"
: > "$REPORT"
echo "Validacion BRU contra evidencia previamente aprobada: $BASE" | tee -a "$REPORT"
if ! iverilog -g2012 -s tb_diagnostico_compilador -o "$DEST/sim.vvp" \
    "$SRC/tb_diagnostico_compilador.sv" "$ARQ"/src/*.sv > "$DEST/iverilog.log" 2>&1; then
  tail -n 30 "$DEST/iverilog.log"; exit 2
fi
bad=0
total_bru=0
for caso in factorial mientras condicional para; do
  OUT="$DEST/$caso"; mkdir -p "$OUT"
  IN="$BASE/$caso"
  for f in salida_resuelta.asm salida_para_binario.asm secuencial.mem meta.env; do
    if [[ ! -f "$IN/$f" ]]; then
      echo "FAIL $caso: no existe $f en evidencia base" | tee -a "$REPORT"
      bad=$((bad+1)); continue 2
    fi
    cp "$IN/$f" "$OUT/$f"
  done
  if ! (cd "$OUT" && PYTHONPATH="$SRC" python3 "$SRC/empaquetar_vliw_bru.py" > empaquetado.log 2>&1 && \
       python3 "$ARQ/python/assembler.py" salida_paralela_bru.asm -o paralelo.mem > ensamble.log 2>&1); then
    echo "FAIL $caso: empaquetado/ensamblado" | tee -a "$REPORT"
    tail -n 20 "$OUT/empaquetado.log" "$OUT/ensamble.log" 2>/dev/null || true
    bad=$((bad+1)); continue
  fi
  # Comprobar que la transformacion SOLO adelanta un BRU a un bundle contiguo
  # y ajusta su offset en 16; conservar todos los bits restantes.
  if ! (cd "$OUT" && python3 - <<'PY'
from pathlib import Path
import re
old=Path('salida_paralela.asm').read_text().splitlines()
new=Path('salida_paralela_bru.asm').read_text().splitlines()
assert len(old)==len(new)
i=0
while i<len(old):
    if old[i]==new[i]: i+=1; continue
    assert i+1<len(old)
    assert new[i+1]=='ALU: NOP'
    assert old[i+1].startswith('BRU: ')
    assert new[i].startswith(old[i]+' | BRU: ')
    a=old[i+1][5:].split()
    b=new[i].split(' | BRU: ',1)[1].split()
    assert a[0]==b[0]
    aa=' '.join(a[1:]).split(',')
    bb=' '.join(b[1:]).split(',')
    assert len(aa)==len(bb) and [x.strip() for x in aa[:-1]]==[x.strip() for x in bb[:-1]]
    assert int(bb[-1].strip())==int(aa[-1].strip())+16
    i+=2
# Pad al mismo numero de lineas y validar los 128 bits.
seq=Path('secuencial.mem').read_text().splitlines()
par=Path('paralelo.mem').read_text().splitlines()
assert len(par)==len(old)
assert len(seq)>=len(par)
assert all(re.fullmatch('[0-9a-fA-F]{32}',x) for x in par)
Path('paralelo.mem').write_text('\n'.join(par+['0'*32]*(len(seq)-len(par)))+'\n')
print('PASS validacion estatica: ubicacion, NOP, offset, longitud y hex')
PY
  ); then
    echo "FAIL $caso: invariantes de empaquetado" | tee -a "$REPORT"; bad=$((bad+1)); continue
  fi
  source "$OUT/meta.env"
  case "$caso" in
    factorial) RESULT=120; DIR=65528 ;;
    mientras) RESULT=6; DIR=65532 ;;
    condicional) RESULT=42; DIR=65532 ;;
    para) RESULT=8; DIR=65532 ;;
  esac
  stat_seq=0; stat_par=0; stat_diff=0
  (cd "$OUT" && cp secuencial.mem programa_prueba.mem && \
     vvp "$DEST/sim.vvp" "+fin_pc=$FIN_PC" "+resultado_dir=$DIR" "+esperado=$RESULT" "+snapshot=seq" >seq.log 2>&1) || stat_seq=1
  (cd "$OUT" && cp paralelo.mem programa_prueba.mem && \
     vvp "$DEST/sim.vvp" "+fin_pc=$FIN_PC" "+resultado_dir=$DIR" "+esperado=$RESULT" "+snapshot=par" +exigir_paralelo >par.log 2>&1) || stat_par=1
  if [[ ! -f "$OUT/seq.memory.hex" || ! -f "$OUT/par.memory.hex" || \
        ! -f "$OUT/seq.registers.txt" || ! -f "$OUT/par.registers.txt" ]] || \
     ! cmp -s "$OUT/seq.memory.hex" "$OUT/par.memory.hex" || \
     ! cmp -s "$OUT/seq.registers.txt" "$OUT/par.registers.txt"; then
     stat_diff=1
  fi
  new_bru=$(grep -cE '^.* \| BRU:' "$OUT/salida_paralela_bru.asm" || true)
  # Cada linea multi-slot con BRU proviene de esta transformacion.
  total_bru=$((total_bru+new_bru))
  if ((stat_seq==0 && stat_par==0 && stat_diff==0)); then
    echo "PASS $caso: resultado=$RESULT, RAM+registros iguales, bundles+BRU=$new_bru" | tee -a "$REPORT"
  else
    echo "FAIL $caso: secuencial=$stat_seq paralelo=$stat_par diferencias=$stat_diff" | tee -a "$REPORT"
    grep -E 'FIN_CONFIRMADO_EX|FAIL:|RESULTADO_TB|RESUMEN:' "$OUT/seq.log" "$OUT/par.log" | tail -n 20 | tee -a "$REPORT" || true
    bad=$((bad+1))
  fi
  grep -E 'RESUMEN_UNIDADES:' "$OUT/par.log" | tee -a "$REPORT" || true
done
echo "TOTAL BUNDLES NUEVOS CON BRU=$total_bru" | tee -a "$REPORT"
echo "FALLOS E2E=$bad" | tee -a "$REPORT"
echo "REPORTE: $REPORT"
if ((total_bru==0)); then echo "AVISO: no se encontraron combinaciones BRU seguras en estos cuatro programas." | tee -a "$REPORT"; fi
exit "$bad"
