#!/usr/bin/env python3
"""Simulador secuencial *de pruebas* del ASM textual del backend IR.
No sustituye RTL: comprueba operaciones, saltos, pila y llamadas en el ISA textual.
"""
import argparse
import re
from pathlib import Path

def i32(n):
    n = int(n) & 0xffffffff
    return n - 0x100000000 if n & 0x80000000 else n

def ejecutar(ruta, direccion, esperado):
    etiquetas = {}
    codigo = []
    for raw in Path(ruta).read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        if line.endswith(':'):
            etiquetas[line[:-1]] = len(codigo)
        else:
            m = re.fullmatch(r'(ALU|LSU|BRU|FEIST):\s*(.+)', line)
            if not m:
                raise ValueError(f'Linea ASM inesperada: {line}')
            codigo.append(m.group(2).replace(',', ' ').split())
    r = [0] * 16
    mem = {}
    pc = 0
    pasos = 0
    while 0 <= pc < len(codigo) and pasos < 1500000:
        ins = codigo[pc]
        op = ins[0]
        if pc == etiquetas.get('fin_programa', -1):
            break
        pasos += 1
        def reg(texto):
            assert re.fullmatch(r'x(?:1[0-5]|[0-9])', texto), texto
            return int(texto[1:])
        def read(x):
            nr = reg(x)
            return 0 if nr == 0 else pc * 16 if nr == 1 else r[nr]
        def valor(x):
            return int(x, 0) if re.fullmatch(r'-?\d+',x) else (etiquetas[x] - pc) * 16
        def write(x, v):
            nr=reg(x)
            if nr == 0:
                return
            r[nr]=i32(v)
        siguiente = pc + 1
        if op == 'NOP':
            pass
        elif op in ('ADD','SUB','AND','OR','XOR','SLT'):
            d, a, b = ins[1:]
            v1, v2 = read(a), read(b)
            if op == 'ADD': out = v1+v2
            elif op == 'SUB': out = v1-v2
            elif op == 'AND': out = v1&v2
            elif op == 'OR': out = v1|v2
            elif op == 'XOR': out = v1^v2
            else: out = int(v1<v2)
            write(d,out)
            if d == 'x1':
                assert r[1] % 16 == 0, (pc,ins,r[1])
                siguiente = (r[1] & 0xffffffff)//16
        elif op in ('ADDI','SLLI','SRLI'):
            d,a,b=ins[1:]
            v=read(a)
            if op=='ADDI': out = v + valor(b)
            elif op == 'SLLI': out = (v << int(b))
            else: out = (v & 0xffffffff) >> int(b)
            write(d,out)
            if d == 'x1': siguiente = (r[1] & 0xffffffff)//16
        elif op in ('LOAD','STORE'):
            a, memarg = ins[1:]
            m = re.fullmatch(r'(-?\d+)\((x\d+)\)', memarg)
            assert m, (pc,ins)
            dir_mem = i32(read(m[2]) + int(m[1]))
            assert dir_mem % 4 == 0 and 0 <= dir_mem <= 65532, (pc, ins, dir_mem)
            if op == 'LOAD':
                write(a,mem.get(dir_mem,0))
                if a == 'x1': siguiente = (r[1] & 0xffffffff)//16
            else:
                mem[dir_mem]=read(a)
        elif op in ('BEQ','BNE','BLE','BGE'):
            a,b,target = ins[1:]
            aa,bb=read(a),read(b)
            tomar={'BEQ':aa==bb,'BNE':aa!=bb,'BLE':aa<=bb,'BGE':aa>=bb}[op]
            if tomar:
                siguiente=etiquetas[target]
        elif op == 'JMP':
            siguiente=etiquetas[ins[1]]
        else:
            raise ValueError(f'Operacion desconocida pc={pc}: {ins}')
        pc = siguiente
    if pasos>=1500000:
        raise AssertionError(f'Ejecucion excede limite de pasos; posible ciclo infinito {ruta}, pc={pc}')
    obtenido=mem.get(direccion, 0)
    assert obtenido == esperado, (ruta,'resultado=',obtenido,'esperado=',esperado,'pasos=',pasos,'pc=',pc)
    print(f'PASS: {ruta}: RAM[{direccion}] = {obtenido} (esperado {esperado}), instrucciones ejecutadas={pasos}')

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('asm'); p.add_argument('direccion',type=int); p.add_argument('esperado',type=int)
    a=p.parse_args()
    ejecutar(a.asm,a.direccion,a.esperado)
