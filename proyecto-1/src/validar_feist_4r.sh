#!/usr/bin/env bash
# Un comando: 4+4 FEIST desde .e y regresion de factorial/while/if/for.
# Solo lee/compila codigo del repositorio del usuario; no modifica Arquitectura.
set -Eeuo pipefail
SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARQ="${ARQ_ROOT:-/mnt/c/Users/fmoreno/Desktop/Arquitectura1-IIS2026}"
ASSEM="$ARQ/python/assembler.py"
RUN="$SRC/evidencia-vliw/feist4r-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$RUN"
REPORTE="$RUN/resumen_feist_4r.txt"
: > "$REPORTE"
log(){ echo "$*" | tee -a "$REPORTE"; }
fail(){ log "FAIL: $1 (evidencia: $RUN)"; exit 1; }
for c in make gcc python3 iverilog vvp cmp awk; do command -v "$c" >/dev/null || fail "Falta $c"; done
for f in "$ASSEM" "$SRC/compilador" "$SRC/resolver_asm"; do :; done
for f in "$ASSEM" "$SRC/demo_feist_4r.e" "$SRC/tb_feist_4r.sv" \
 "$SRC/tb_regresion_e2e.sv" "$SRC/verificar_empaquetado_4r.py" \
 "$SRC/preparar_asm.py" "$SRC/empaquetar_vliw.py" \
 "$SRC/empaquetar_vliw_bru.py" "$SRC/empaquetar_vliw_feist.py" \
 "$SRC/factorial.e"; do [[ -f "$f" ]] || fail "No existe $f"; done
[[ -d "$ARQ/src" ]] || fail "No esta Arquitectura en $ARQ"

# Cada caso se genera en su carpeta aislada para no sobreescribir los .asm del usuario.
cd "$SRC"
log '== 1/3 Compilacion del compilador y testbenches =='
make all > "$RUN/build_compilador.log" 2>&1 || { tail -n 30 "$RUN/build_compilador.log"; fail 'make all'; }
bash "$SRC/probar_semantico.sh" > "$RUN/semantico.log" 2>&1 || {
    tail -n 30 "$RUN/semantico.log"; fail '34 pruebas semanticas'; }
grep 'Resultado: ' "$RUN/semantico.log" | tail -n 1 | tee -a "$REPORTE"
iverilog -g2012 -s tb_feist_4r -o "$RUN/feist4r.vvp" \
    "$SRC/tb_feist_4r.sv" "$ARQ"/src/*.sv > "$RUN/build_feist4r.log" 2>&1 || {
    tail -n 25 "$RUN/build_feist4r.log"; fail 'Icarus FEIST'; }
iverilog -g2012 -s tb_regresion_e2e -o "$RUN/regresion.vvp" \
    "$SRC/tb_regresion_e2e.sv" "$ARQ"/src/*.sv > "$RUN/build_regresion.log" 2>&1 || {
    tail -n 25 "$RUN/build_regresion.log"; fail 'Icarus regresion'; }

# Parte de la prueba: producir el binario desde el .e ACTUAL.
generar(){
    local dir="$1"
    ( cd "$dir" || exit 1
      "$SRC/cdff" -s programa.e > compilador.log 2>&1 || exit 1
      mv programa.asm salida.asm && "$SRC/resolver_asm" salida.asm salida_resuelta.asm > etiquetas.log 2>&1 || exit 1
      python3 "$SRC/preparar_asm.py" > preparar.log 2>&1 || exit 1
      python3 "$ASSEM" salida_para_binario.asm -o secuencial.mem > assembler_seq.log 2>&1 || exit 1
      python3 "$SRC/empaquetar_vliw_bru.py" > bru.log 2>&1 || exit 1
      python3 "$SRC/empaquetar_vliw_feist.py" salida_paralela_bru.asm \
             -o salida_vliw_completa.asm --etiquetas salida_resuelta.asm > feist.log 2>&1 || exit 1
      python3 "$SRC/verificar_empaquetado_4r.py" salida_para_binario.asm \
             salida_vliw_completa.asm > verificar.log 2>&1 || exit 1
      python3 "$ASSEM" salida_vliw_completa.asm -o paralela.mem > assembler_par.log 2>&1 || exit 1
      awk '/^fin_programa:/{print n*16; exit} /^(ALU|LSU|BRU|FEIST):/{n++}' \
          salida_resuelta.asm > finpc.txt || exit 1
      test "$(wc -l < secuencial.mem)" -eq "$(wc -l < paralela.mem)" || exit 1
      grep -Eq '^[0-9]+$' finpc.txt || exit 1
    )
}
# Preparar .mem de 4096 entradas para evitar warnings por memoria sin inicializar.
cargar_mem(){
    local archivo="$1"; local destino="$2"
    python3 - "$archivo" "$destino" <<'PY'
import pathlib,sys,re
rows=pathlib.Path(sys.argv[1]).read_text().splitlines()
assert 0 < len(rows) <= 4096 and all(re.fullmatch(r'[0-9A-Fa-f]{32}',x) for x in rows)
pathlib.Path(sys.argv[2]).write_text('\n'.join(rows+['0'*32]*(4096-len(rows)))+'\n')
PY
}

log '== 2/3 FEIST: cuatro rondas cifrado, cuatro descifrado, comparacion VLIW =='
D="$RUN/feist"
mkdir -p "$D"
cp "$SRC/demo_feist_4r.e" "$D/programa.e"
# "--exigir-feist" solo aplica al caso completo, no a los programas generales.
if ! ( cd "$D" && \
        "$SRC/cdff" -s programa.e > compilador.log 2>&1 && \
        mv programa.asm salida.asm && "$SRC/resolver_asm" salida.asm salida_resuelta.asm > etiquetas.log 2>&1 && \
        python3 "$SRC/preparar_asm.py" > preparar.log 2>&1 && \
        python3 "$ASSEM" salida_para_binario.asm -o secuencial.mem > assembler_seq.log 2>&1 && \
        python3 "$SRC/empaquetar_vliw_bru.py" > bru.log 2>&1 && \
        python3 "$SRC/empaquetar_vliw_feist.py" salida_paralela_bru.asm \
           -o salida_vliw_completa.asm --etiquetas salida_resuelta.asm > feist.log 2>&1 && \
        python3 "$SRC/verificar_empaquetado_4r.py" salida_para_binario.asm \
           salida_vliw_completa.asm --exigir-feist > verificar.log 2>&1 && \
        python3 "$ASSEM" salida_vliw_completa.asm -o paralela.mem > assembler_par.log 2>&1 && \
        awk '/^fin_programa:/{print n*16; exit} /^(ALU|LSU|BRU|FEIST):/{n++}' \
            salida_resuelta.asm > finpc.txt ); then
    tail -n 20 "$D"/*.log 2>/dev/null || :
    fail 'generacion/empaquetado FEIST desde .e'
fi
finpc="$(cat "$D/finpc.txt")"
[[ "$finpc" =~ ^[0-9]+$ ]] || fail 'FIN_PC FEIST invalido'
for tipo in secuencial paralela; do
    cargar_mem "$D/$tipo.mem" "$D/programa_feist_validacion.mem" || fail "MEM: $tipo FEIST"
    opts=("+fin_pc=$finpc" "+snapshot=$tipo.snapshot.txt")
    [[ "$tipo" == paralela ]] && opts+=(+exigir_mixto)
    if ! (cd "$D" && vvp "$RUN/feist4r.vvp" "${opts[@]}" >"$tipo.log" 2>&1); then
        tail -n 25 "$D/$tipo.log"; fail "simulacion FEIST $tipo"
    fi
    grep -E '^(FIN_EX=|CIFRADO=|PASS:)' "$D/$tipo.log" | tee -a "$REPORTE"
done
cmp -s "$D/secuencial.snapshot.txt" "$D/paralela.snapshot.txt" || fail 'FEIST: RAM/registros diferentes'
cat "$D/verificar.log" | tee -a "$REPORTE"
log 'PASS FEIST 4+4 desde .e: cifrado, descifrado, FEIST mixto y estado completo iguales'

log '== 3/3 Regresion: factorial, mientras, condicional y para desde el compilador ACTUAL =='
mkdir -p "$RUN/factorial" "$RUN/mientras" "$RUN/condicional" "$RUN/para"
cp "$SRC/factorial.e" "$RUN/factorial/programa.e"
cat > "$RUN/mientras/programa.e" <<'CASE'
defina vacio principal() {
    entero resultado = 0 :)
    entero contador = 3 :)
    mientras (contador > 0) {
        resultado = resultado + contador :)
        contador = contador - 1 :)
    } :)
} :)
CASE
cat > "$RUN/condicional/programa.e" <<'CASE'
defina vacio principal() {
    entero resultado = 10 :)
    si (resultado == 10) {
        resultado = resultado + 32 :)
    } :)
} :)
CASE
cat > "$RUN/para/programa.e" <<'CASE'
defina vacio principal() {
    entero resultado = 0 :)
    para (entero i = 0, i < 4, i = i + 1) {
        resultado = resultado + 2 :)
    } :)
} :)
CASE
FALLOS=0
for caso in factorial mientras condicional para; do
    d="$RUN/$caso"
    case "$caso" in
        factorial) esp=120; addr=65528;;
        mientras) esp=6; addr=65532;;
        condicional) esp=42; addr=65532;;
        para) esp=8; addr=65532;;
    esac
    if ! generar "$d"; then
        log "FAIL $caso: compilador, assembler o empaquetamiento (logs en $d)"
        FALLOS=$((FALLOS+1)); continue
    fi
    fin="$(cat "$d/finpc.txt")"
    for tipo in secuencial paralela; do
        cargar_mem "$d/$tipo.mem" "$d/programa_prueba.mem" || { FALLOS=$((FALLOS+1)); continue; }
        # El testbench exige declarar explicitamente el modo paralelo.
        extras=()
        if [[ "$tipo" == "paralela" ]]; then
            extras=(+exigir_paralelo)
        fi
        if ! ( cd "$d" && vvp "$RUN/regresion.vvp" \
                  "+fin_pc=$fin" "+resultado_dir=$addr" "+esperado=$esp" \
                  "+snapshot=$tipo" "${extras[@]}" >"$tipo.log" 2>&1 ); then
            log "FAIL $caso: simulacion $tipo"; tail -n 7 "$d/$tipo.log"
            FALLOS=$((FALLOS+1))
        fi
    done
    if ! cmp -s "$d/secuencial.memory.hex" "$d/paralela.memory.hex" ||
       ! cmp -s "$d/secuencial.registers.txt" "$d/paralela.registers.txt"; then
        log "FAIL $caso: RAM/registros distintos"; FALLOS=$((FALLOS+1))
    elif [[ -f "$d/secuencial.log" && -f "$d/paralela.log" ]] &&
         grep -q 'RESULTADO_TB: PASS' "$d/secuencial.log" &&
         grep -q 'RESULTADO_TB: PASS' "$d/paralela.log"; then
        log "PASS $caso: $esp, estado secuencial == estado empaquetado"
    else
        log "FAIL $caso: no se encontro PASS en ambos logs"; FALLOS=$((FALLOS+1))
    fi
done
log "FALLOS REGRESION=$FALLOS"
log "EVIDENCIA=$RUN"
((FALLOS==0)) || exit 1
log 'RESULTADO FINAL: PASS FEIST 4+4 Y 4 PROGRAMAS DE REGRESION'
