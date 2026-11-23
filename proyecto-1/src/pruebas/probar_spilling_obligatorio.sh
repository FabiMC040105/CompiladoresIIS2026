#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

mkdir -p evidencia-vliw/spilling

pruebas=(
    "pruebas/spill_01_10_argumentos.e"
    "pruebas/spill_02_12_argumentos.e"
    "pruebas/spill_03_14_argumentos.e"
)

for fuente in "${pruebas[@]}"; do
    nombre="$(basename "${fuente%.e}")"
    log="evidencia-vliw/spilling/${nombre}.log"

    echo
    echo "=================================================="
    echo "PRUEBA: $nombre"
    echo "=================================================="

    ./cdff \
        --codegen-ir \
        --spill-ir \
        --emit-ir \
        -s \
        -v \
        "$fuente" 2>&1 | tee "$log"

    if ! grep -Eq \
        '\[SPILL IR\] rondas=[1-9][0-9]* STORE=[1-9][0-9]* LOAD=[1-9][0-9]* slots=[1-9][0-9]*' \
        "$log"; then

        echo "FAIL: no se detecto spilling real en $nombre"
        exit 1
    fi

    asm="${fuente%.e}.asm"
    bin="${fuente%.e}.bin"

    if [[ ! -s "$asm" ]]; then
        echo "FAIL: no se genero $asm"
        exit 1
    fi

    if [[ ! -s "$bin" ]]; then
        echo "FAIL: no se genero $bin"
        exit 1
    fi

    # El backend con Chaitin ya no fija los temporales a x4 ni usa x11
    # como direccion de spill. Verificamos el efecto semantico en la IR y
    # la materializacion real mediante LSU usando el registro de direccion x13.
    if ! grep -Eq 'cdff_spill_area\[[0-9]+\] = %t[0-9]+' "$log"; then
        echo "FAIL: no se encontro STORE de spilling en IR"
        exit 1
    fi

    if ! grep -Eq '%t[0-9]+ = cdff_spill_area\[[0-9]+\]' "$log"; then
        echo "FAIL: no se encontro LOAD de spilling en IR"
        exit 1
    fi

    if ! grep -Eq 'STORE x[0-9]+, 0\(x13\)' "$asm"; then
        echo "FAIL: no se materializo STORE de memoria en ASM"
        exit 1
    fi

    if ! grep -Eq 'LOAD x[0-9]+, 0\(x13\)' "$asm"; then
        echo "FAIL: no se materializo LOAD de memoria en ASM"
        exit 1
    fi

    echo "PASS: $nombre"
done

echo
echo "=================================================="
echo "PASS: LAS 3 PRUEBAS OBLIGATORIAS DE SPILLING"
echo "=================================================="
