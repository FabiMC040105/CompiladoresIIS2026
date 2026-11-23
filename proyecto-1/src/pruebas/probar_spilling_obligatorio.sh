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

    if ! grep -q 'STORE x4, 0(x11)' "$asm"; then
        echo "FAIL: no se encontro STORE de spilling en ASM"
        exit 1
    fi

    if ! grep -q 'LOAD x4, 0(x11)' "$asm"; then
        echo "FAIL: no se encontro LOAD de spilling en ASM"
        exit 1
    fi

    echo "PASS: $nombre"
done

echo
echo "=================================================="
echo "PASS: LAS 3 PRUEBAS OBLIGATORIAS DE SPILLING"
echo "=================================================="
