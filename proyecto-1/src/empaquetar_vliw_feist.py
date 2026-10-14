#!/usr/bin/env python3
"""Empaquetado FEIST conservador para ASM VLIW ya resuelto.

Mueve una instruccion FEIST del bundle i al siguiente i+1 cuando es seguro.
Los indices, offsets de BRU y numero de bundles permanecen intactos.
No introduce FEIST en programas fuente .e que no lo tienen.
"""
import argparse
import re
from dataclasses import dataclass
from pathlib import Path

LINE = re.compile(r'^(ALU|LSU|BRU|FEIST)\s*:\s*(.+)$', re.I)
LABEL = re.compile(r'^[a-zA-Z_]\w*:$')
REG = re.compile(r'^x(\d+)$', re.I)
ADDR = re.compile(r'^([+-]?(?:0[xX][0-9a-fA-F]+|\d+))\s*\(x(\d+)\)$', re.I)
NUM = re.compile(r'^[+-]?(?:0[xX][0-9a-fA-F]+|\d+)$')


def nval(s):
    return int(s, 0) if not re.fullmatch(r'[+-]?0\d+', s) else int(s, 10)


def reg(s):
    m = REG.fullmatch(s.strip())
    if not m or not 0 <= int(m[1]) <= 15:
        raise ValueError(f'Registro invalido: {s}')
    return int(m[1])


def rset(*rs):
    return set(x for x in rs if x >= 2)


@dataclass
class Op:
    slot: str
    name: str
    text: str
    reads: set
    writes: set
    vault_change: bool = False
    pc_change: bool = False
    offset: int | None = None


def parse_inst(s):
    m = LINE.fullmatch(s.strip())
    if not m:
        raise ValueError(f'Slot invalido: {s}')
    slot, rest = m[1].upper(), m[2].strip()
    op, _, tail = rest.partition(' ')
    name = op.upper()
    args = [p.strip() for p in tail.split(',')] if tail.strip() else []

    def count(n):
        if len(args) != n:
            raise ValueError(f'{slot}: {name} requiere {n} operandos, recibido {len(args)}')

    if name == 'NOP':
        count(0)
        return Op(slot, name, s, set(), set())
    if slot == 'ALU':
        if name in ('ADD','SUB','OR','XOR','AND','SLT'):
            count(3)
            rd, a, b = map(reg, args)
            return Op(slot,name,s,rset(a,b),rset(rd),pc_change=rd==1)
        if name in ('ADDI','SLLI','SRLI'):
            count(3)
            rd, a = reg(args[0]),reg(args[1])
            if not NUM.fullmatch(args[2]):
                raise ValueError(f'Inmediato ALU no resuelto: {s}')
            return Op(slot,name,s,rset(a),rset(rd),pc_change=rd==1)
    if slot == 'LSU' and name in ('LOAD','STORE'):
        count(2)
        rd = reg(args[0])
        addr = ADDR.fullmatch(args[1])
        if not addr:
            raise ValueError(f'Direccion LSU invalida: {s}')
        base = int(addr[2])
        if not 0<=base<=15:
            raise ValueError('Base invalida')
        if name == 'LOAD':
            return Op(slot,name,s,rset(base),rset(rd),pc_change=rd==1)
        return Op(slot,name,s,rset(base,rd),set())
    if slot == 'BRU':
        if name == 'JMP':
            count(1)
            if not NUM.fullmatch(args[0]):
                raise ValueError('Salto simbolico no resuelto')
            return Op(slot,name,s,set(),set(),pc_change=True,offset=nval(args[0]))
        if name in ('BGE','BLE','BEQ','BNE'):
            count(3)
            a,b = reg(args[0]),reg(args[1])
            if not NUM.fullmatch(args[2]):
                raise ValueError('Salto simbolico no resuelto')
            return Op(slot,name,s,rset(a,b),set(),pc_change=True,offset=nval(args[2]))
    if slot == 'FEIST':
        if name == 'AUTH':
            count(2)
            a,b = map(reg,args)
            return Op(slot,name,s,rset(a,b),set(),vault_change=True)
        if name == 'LOGOUT':
            count(0)
            return Op(slot,name,s,set(),set(),vault_change=True)
        if name == 'KSTORE':
            count(3)
            key, ronda = nval(args[0]),nval(args[1])
            if not 0<=key<=3 or not 0<=ronda<=3:
                raise ValueError('Key/ronda fuera de rango')
            return Op(slot,name,s,rset(reg(args[2])),set(),vault_change=True)
        if name == 'FROUND':
            count(7)
            rd1,rd2,rs1,rs2 = map(reg,args[:4])
            key,ronda,mode = map(nval,args[4:])
            if not (0<=key<=3 and 0<=ronda<=3 and mode in (0,1)):
                raise ValueError('Key/ronda/modo fuera de rango')
            if rd1 in (0,1) or rd2 in (0,1) or rd1==rd2:
                raise ValueError('FROUND requiere dos destinos GPR distintos (x2..x15)')
            return Op(slot,name,s,rset(rs1,rs2),rset(rd1,rd2))
    raise ValueError(f'Instruccion no reconocida: {s}')


def read_bundles(path):
    result=[]
    for k,raw in enumerate(Path(path).read_text(encoding='utf8').splitlines(),1):
        s=raw.split('#',1)[0].split('//',1)[0].strip()
        if not s:
            continue
        if LABEL.fullmatch(s):
            raise ValueError(f'Entrada {path}:{k}: resolver etiquetas antes del empaquetado')
        insts={}
        for piece in s.split('|'):
            op=parse_inst(piece.strip())
            if op.slot in insts:
                raise ValueError(f'Dos operaciones para el mismo slot en linea {k}')
            insts[op.slot]=op
        result.append(insts)
    return result


def read_labels(path,n):
    labels=set()
    if not path:
        return labels
    idx=0
    for raw in Path(path).read_text(encoding='utf8').splitlines():
        s=raw.split('#',1)[0].split('//',1)[0].strip()
        if not s:
            continue
        if LABEL.fullmatch(s):
            labels.add(idx)
        elif LINE.match(s):
            idx+=1
        else:
            raise ValueError('Fuente de etiquetas no reconocida: '+s)
    if idx!=n:
        raise ValueError(f'Original tiene {idx} bundles y entrada {n}')
    return labels


def targets(bundles, labels):
    all_targets=set(labels)
    n=len(bundles)
    for i,b in enumerate(bundles):
        if 'BRU' in b and b['BRU'].offset is not None:
            target_byte=16*i+b['BRU'].offset
            if target_byte%16 or not 0<=target_byte<16*n:
                raise ValueError(f'Salto invalido en bundle {i}: destino {target_byte}')
            all_targets.add(target_byte//16)
    return all_targets


def all_reads(bundle):
    return set().union(*(op.reads for op in bundle.values())) if bundle else set()


def all_writes(bundle):
    return set().union(*(op.writes for op in bundle.values())) if bundle else set()


def has_actual(b,slot):
    return slot in b and b[slot].name!='NOP'


def pack(bundles, label_targets):
    out=[dict(b) for b in bundles]
    n=len(out)
    stats={'FEIST_MIX':0,'CUATRO_SLOTS':0,'AUTH':0,'KSTORE':0,'FROUND':0,'LOGOUT':0}
    for i in range(n-1):
        left,right=out[i],out[i+1]
        if set(left)!={'FEIST'} or not has_actual(left,'FEIST'):
            continue
        feist=left['FEIST']
        if has_actual(right,'FEIST') or not any(has_actual(right,s) for s in ('ALU','LSU','BRU')):
            continue
        # Prohibe cambiar el comportamiento de entradas alternativas por salto.
        if i in label_targets or i+1 in label_targets:
            continue
        # FEIST no debe leer el PC: moverlo alteraria su significado.
        if any(re.search(r'\bx1\b', feist.text, flags=re.I) for _ in (0,)):
            continue
        # Al agrupar lecturas y escrituras no debe haber dependencias ni WAW.
        if (feist.writes & (all_writes(right)|all_reads(right)) or
            feist.reads & all_writes(right)):
            continue
        # PC redirigido en WB produce peligros adicionales en el mismo bundle.
        if any(z.pc_change and z.slot!='BRU' for z in right.values()):
            continue
        # La nueva lectura FEIST ocurrira un ciclo mas tarde; no cruzar WB
        # de ningun productor cercano que pudiera cambiar el valor leido.
        if any(feist.reads & all_writes(out[j]) for j in range(max(0,i-3),i)):
            continue
        # FEIST escribe un ciclo mas tarde, los consumidores de los siguientes
        # tres bundles podrian encontrar un dato prematuro sin forwarding.
        if any(feist.writes & all_reads(out[j]) for j in range(i+2,min(n,i+4))):
            continue
        # Conservador para las operaciones que modifican la boveda.
        if feist.vault_change and any(has_actual(out[j],'FEIST') for j in range(i+2,min(n,i+4))):
            continue
        # Conserva el orden relativo de FEIST, no se cruza ninguna otra.
        out[i]={'ALU':parse_inst('ALU: NOP')}
        out[i+1]=dict(right)
        out[i+1]['FEIST']=feist
        stats['FEIST_MIX']+=1
        stats[feist.name]+=1
        if all(has_actual(out[i+1],s) for s in ('ALU','LSU','BRU','FEIST')):
            stats['CUATRO_SLOTS']+=1
    return out,stats


def serialize(b):
    return ' | '.join(b[s].text.strip() for s in ('ALU','LSU','BRU','FEIST') if s in b)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('entrada',help='ASM con slots, sin etiquetas; FEIST debe estar presente')
    p.add_argument('-o','--salida',required=True)
    p.add_argument('--etiquetas',help='ASM original con etiquetas, opcional')
    args=p.parse_args()
    bundles=read_bundles(args.entrada)
    labels=read_labels(args.etiquetas,len(bundles))
    opts=targets(bundles,labels)
    packed, stats=pack(bundles,opts)
    Path(args.salida).write_text('\n'.join(map(serialize,packed))+'\n',encoding='utf8')
    print('Bundles originales:',len(bundles))
    print('Bundles finales:',len(packed))
    print('Empaquetados FEIST:',stats['FEIST_MIX'])
    print('Bundles con 4 unidades:',stats['CUATRO_SLOTS'])
    print('Detalle:',', '.join(f'{x}={stats[x]}' for x in ('AUTH','KSTORE','FROUND','LOGOUT')))
    print('Salida:',args.salida)

if __name__=='__main__':
    main()
