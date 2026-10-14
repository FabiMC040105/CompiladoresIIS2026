#!/usr/bin/env bash
# Reutiliza los binarios .mem y el simulador compilado de validar_feist_4r.sh.
# Corrige la seleccion de modo paralelo sin repetir FEIST ni pruebas semanticas.
set -Eeuo pipefail
SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ $# -ne 1 ]]; then
  echo "Uso: bash revalidar_regresion_feist.sh evidencia-vliw/feist4r-AAAA..." >&2
  exit 2
fi
RUN="$1"
[[ "$RUN" == /* ]] || RUN="$SRC/$RUN"
[[ -f "$RUN/regresion.vvp" ]] || { echo "Falta $RUN/regresion.vvp" >&2; exit 2; }
command -v vvp >/dev/null || { echo 'Falta vvp' >&2; exit 2; }
command -v python3 >/dev/null || { echo 'Falta python3' >&2; exit 2; }
REPORTE="$RUN/resumen_regresion_corregida.txt"
: > "$REPORTE"
log(){ printf '%s\n' "$*" | tee -a "$REPORTE"; }
fallos=0
for caso in factorial mientras condicional para; do
  d="$RUN/$caso"
  case "$caso" in
    factorial) esperado=120; dir=65528 ;;
    mientras) esperado=6; dir=65532 ;;
    condicional) esperado=42; dir=65532 ;;
    para) esperado=8; dir=65532 ;;
  esac
  if [[ ! -f "$d/finpc.txt" || ! -f "$d/secuencial.mem" || ! -f "$d/paralela.mem" ]]; then
    log "FAIL $caso: faltan archivos anteriores"; fallos=$((fallos+1)); continue
  fi
  fin="$(cat "$d/finpc.txt")"
  if [[ ! "$fin" =~ ^[0-9]+$ ]]; then
    log "FAIL $caso: finpc invalido"; fallos=$((fallos+1)); continue
  fi
  ok=1
  for tipo in secuencial paralela; do
    if ! python3 - "$d/$tipo.mem" "$d/programa_prueba.mem" <<'PY'
import pathlib, sys, re
rows = pathlib.Path(sys.argv[1]).read_text().splitlines()
assert 0 < len(rows) <= 4096 and all(re.fullmatch(r'[0-9A-Fa-f]{32}', x) for x in rows)
pathlib.Path(sys.argv[2]).write_text('\n'.join(rows + ['0'*32]*(4096-len(rows))) + '\n')
PY
    then
      log "FAIL $caso/$tipo: .mem invalido"; ok=0; break
    fi
    extras=()
    if [[ "$tipo" == paralela ]]; then extras+=(+exigir_paralelo); fi
    if ! (cd "$d" && vvp "$RUN/regresion.vvp" \
          "+fin_pc=$fin" "+resultado_dir=$dir" "+esperado=$esperado" \
          "+snapshot=corregido_$tipo" "${extras[@]}" >"corregido_$tipo.log" 2>&1); then
      log "FAIL $caso/$tipo: simulacion"; tail -n 8 "$d/corregido_$tipo.log" | tee -a "$REPORTE"; ok=0
    fi
  done
  if [[ "$ok" == 1 ]] && \
    cmp -s "$d/corregido_secuencial.memory.hex" "$d/corregido_paralela.memory.hex" && \
    cmp -s "$d/corregido_secuencial.registers.txt" "$d/corregido_paralela.registers.txt" && \
    grep -q 'RESULTADO_TB: PASS' "$d/corregido_secuencial.log" && \
    grep -q 'RESULTADO_TB: PASS' "$d/corregido_paralela.log"; then
      log "PASS $caso: resultado=$esperado, RAM y registros identicos"
      grep '^RESUMEN_UNIDADES:' "$d/corregido_paralela.log" | tee -a "$REPORTE"
  else
      log "FAIL $caso: error E2E o diferencia RAM/registros"
      fallos=$((fallos+1))
  fi
done
log "FALLOS E2E=$fallos"
log "REPORTE=$REPORTE"
if [[ "$fallos" == 0 ]]; then log 'RESULTADO FINAL: PASS 4 PROGRAMAS DE REGRESION'; else exit 1; fi
