#!/usr/bin/env python3
"""Compara operaciones y destinos reales de saltos antes/despues del empaquetado.
No sustituye las simulaciones: detecta instrucciones perdidas, alteradas o redirigidas.
"""
import argparse
from collections import Counter
from pathlib import Path
import sys

# Importar el analizador de instrucciones ya empleado por el empaquetador FEIST.
import empaquetar_vliw_feist as isa


def normalizar(path):
    bundles = isa.read_bundles(path)
    resultado = Counter()
    for i, bundle in enumerate(bundles):
        for slot, inst in bundle.items():
            if inst.name == 'NOP':
                continue
            if slot == 'BRU':
                destino = i * 16 + inst.offset
                if destino % 16 or not 0 <= destino < len(bundles) * 16:
                    raise ValueError(f'Salto no alineado/fuera de programa: {path}, bundle {i}')
                # El salto conserva los registros de comparacion y el destino
                # absoluto, aunque el empaquetador modifique offset/PC.
                args = inst.text.split(':', 1)[1].strip().split(None, 1)[1]
                args = [z.strip() for z in args.split(',')]
                firma = (slot, inst.name, tuple(args[:-1]), destino)
            else:
                firma = (slot, inst.name, inst.text.strip().upper())
            resultado[firma] += 1
    return bundles, resultado


def main():
    p = argparse.ArgumentParser()
    p.add_argument('original')
    p.add_argument('empaquetado')
    p.add_argument('--exigir-feist', action='store_true')
    args = p.parse_args()
    a, ca = normalizar(args.original)
    b, cb = normalizar(args.empaquetado)
    if len(a) != len(b):
        raise ValueError(f'Cantidad de bundles cambio: {len(a)} -> {len(b)}')
    if ca != cb:
        borradas = ca - cb
        anadidas = cb - ca
        raise ValueError(f'Operaciones/destinos alterados:\nPerdidas: {borradas}\nNuevas: {anadidas}')
    mix = sum(any(x in bb and bb[x].name != 'NOP' for x in ('ALU','LSU','BRU'))
              and 'FEIST' in bb and bb['FEIST'].name != 'NOP' for bb in b)
    four = sum(all(x in bb and bb[x].name != 'NOP'
                   for x in ('ALU','LSU','BRU','FEIST')) for bb in b)
    cf = Counter(inst.name for bb in b for s,inst in bb.items()
                 if s == 'FEIST' and inst.name != 'NOP')
    if args.exigir_feist:
        if cf != Counter({'AUTH':1, 'KSTORE':4, 'FROUND':8, 'LOGOUT':1}):
            raise ValueError(f'FEIST incompleto: {dict(cf)}')
        if mix < 1:
            raise ValueError('El empaquetador no fusiono ningun FEIST con otra unidad')
    print(f'PASS ESTRUCTURAL: bundles={len(b)} operaciones+destinos intactos FEIST_MIX={mix} CUATRO={four} FEIST={dict(cf)}')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as ex:
        print('FAIL ESTRUCTURAL:', ex, file=sys.stderr)
        sys.exit(1)
