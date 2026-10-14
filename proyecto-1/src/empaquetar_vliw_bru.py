#!/usr/bin/env python3
"""Extension conservadora del empaquetador existente: ALU/LSU + BRU.

Conserva numero de bundles, direcciones y todas las lineas base.
Mueve un salto UNA posicion antes y ajusta su offset relativo en +16 bytes.
No transforma FEIST ni afirma paralelismo de las cuatro unidades desde .e.
"""
import re
from pathlib import Path
import empaquetar_vliw as base

ORIGINAL = Path('salida_resuelta.asm')
ENTRADA = Path('salida_paralela.asm')
SALIDA = Path('salida_paralela_bru.asm')
LABEL = re.compile(r'^[A-Za-z_]\w*:$')
SLOT = re.compile(r'^(ALU|LSU|BRU|FEIST)\s*:', re.I)
BRANCH = re.compile(r'^BRU:\s*(JMP|BGE|BLE|BEQ|BNE)\s+(.+)$', re.I)
ALU_OK = re.compile(r'^ALU:\s*ADDI\s+x([4-9]|1[0-2]),\s*x0,\s*[-+]?(?:0[xX][0-9a-fA-F]+|\d+)$', re.I)
STORE_OK = re.compile(r'^LSU:\s*STORE\s+x\d+,\s*[-+]?(?:0[xX][0-9a-fA-F]+|\d+)\s*\(x\d+\)$', re.I)
ALU_WRITE = re.compile(r'^ALU:\s*(?:ADD|SUB|OR|XOR|AND|SLT|ADDI|SRLI|SLLI)\s+x(\d+),', re.I)
LSU_WRITE = re.compile(r'^LSU:\s*LOAD\s+x(\d+),', re.I)
FEIST_WRITE = re.compile(r'^FEIST:\s*FROUND\s+x(\d+),\s*x(\d+),', re.I)


def numero(s):
    s = s.strip()
    try:
        return int(s, 0)
    except ValueError:
        return int(s, 10)


def leer_programa_original():
    labels = set()
    count = 0
    for raw in ORIGINAL.read_text(encoding='utf-8').splitlines():
        s = raw.split('#', 1)[0].split('//', 1)[0].strip()
        if not s:
            continue
        if LABEL.fullmatch(s):
            labels.add(count)
        elif SLOT.match(s):
            count += 1
        else:
            raise ValueError(f'Linea original no reconocida: {s}')
    return labels, count


def info_branch(line, pos, total):
    m = BRANCH.fullmatch(line)
    if not m:
        return None
    op = m.group(1).upper()
    ops = [x.strip() for x in m.group(2).split(',')]
    if (op == 'JMP' and len(ops) != 1) or (op != 'JMP' and len(ops) != 3):
        return None
    try:
        offset = numero(ops[-1])
    except ValueError:
        return None
    target_bytes = pos * 16 + offset
    if target_bytes % 16 != 0 or not (0 <= target_bytes < total * 16):
        return None
    target = target_bytes // 16
    reads = set()
    if op != 'JMP':
        regs = ops[:2]
        if not all(re.fullmatch(r'x(?:[0-9]|1[0-5])', r, re.I) for r in regs):
            return None
        reads = {int(r[1:]) for r in regs if r.lower() not in ('x0', 'x1')}
    return op, ops, offset, target, reads


def escriben(bundle):
    """Devuelve escrituras de una linea o None si no se reconoce (seguridad)."""
    res = set()
    for part in bundle.split(' | '):
        part = part.strip()
        if not part or re.fullmatch(r'(ALU|LSU|BRU|FEIST):\s*NOP', part, re.I):
            continue
        m = ALU_WRITE.match(part)
        if m:
            res.add(int(m.group(1)))
            continue
        m = LSU_WRITE.match(part)
        if m:
            res.add(int(m.group(1)))
            continue
        m = FEIST_WRITE.match(part)
        if m:
            res.update([int(m.group(1)), int(m.group(2))])
            continue
        if STORE_OK.fullmatch(part) or BRANCH.fullmatch(part) or re.match(r'^FEIST:\s*(AUTH|LOGOUT|KSTORE)\b', part, re.I):
            continue
        return None
    return res


def fusionar():
    base.empaquetar()
    labels, n = leer_programa_original()
    lines = [s.strip() for s in ENTRADA.read_text(encoding='utf-8').splitlines() if s.strip()]
    if len(lines) != n:
        raise ValueError('La cantidad de bundles no coincide con la fuente original')
    targets = set(labels)
    for i, s in enumerate(lines):
        for part in s.split(' | '):
            info = info_branch(part.strip(), i, n)
            if info:
                targets.add(info[3])
    counts = {'ALU_BRU': 0, 'LSU_BRU': 0}
    for i in range(n - 1):
        first, second = lines[i], lines[i + 1]
        source = 'ALU' if ALU_OK.fullmatch(first) else 'LSU' if STORE_OK.fullmatch(first) else None
        branch = info_branch(second, i + 1, n)
        if not source or not branch:
            continue
        op, ops, offset, target, reads = branch
        if i in targets or (i + 1) in targets:
            continue
        write_current = escriben(first)
        if write_current is None or reads.intersection(write_current):
            continue
        if reads:
            unsafe = False
            for j in (i - 2, i - 1):
                if j < 0:
                    continue
                writes = escriben(lines[j])
                if writes is None or reads.intersection(writes):
                    unsafe = True
                    break
            if unsafe:
                continue
        new_offset = offset + 16
        bits = 20 if op == 'JMP' else 12
        if not -(1 << (bits - 1)) <= new_offset <= (1 << (bits - 1)) - 1:
            continue
        if i * 16 + new_offset != (i + 1) * 16 + offset:
            raise AssertionError('Error interno: cambio del destino de salto')
        ops[-1] = str(new_offset)
        lines[i] = f'{first} | BRU: {op} ' + ', '.join(ops)
        lines[i + 1] = 'ALU: NOP'
        counts[f'{source}_BRU'] += 1
    SALIDA.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'ALU + BRU nuevas: {counts["ALU_BRU"]}')
    print(f'LSU + BRU nuevas: {counts["LSU_BRU"]}')
    print(f'Bundles conservados: {n}')
    print(f'Archivo BRU: {SALIDA}')
    return counts


if __name__ == '__main__':
    fusionar()
