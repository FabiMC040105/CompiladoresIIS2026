from pathlib import Path
import re, sys
p=Path(sys.argv[1]); exigir=int(sys.argv[2])
seq=[s.strip() for s in (p/'secuencial.mem').read_text().splitlines() if s.strip()]
par=[s.strip() for s in (p/'paralelo.mem').read_text().splitlines() if s.strip()]
assert len(seq)==len(par) and 0<len(seq)<=4096, 'La cantidad de bundles cambio o esta fuera de rango'
assert all(re.fullmatch(r'[0-9a-fA-F]{32}',s) for s in seq+par), 'Hay bundles no hexadecimales de 128 bits'

def slots(h):
    x=int(h,16)
    return [(x>>(32*j)) & 0xFFFFFFFF for j in range(4)] # ALU LSU BRU FEIST

paralelos=[]
cambios=set()
for i,(s,t) in enumerate(zip(seq,par)):
    a,b=slots(s),slots(t)
    if sum(v!=0 for v in b)>1:
        paralelos.append(i+1)
    if s!=t:
        cambios.add(i)
        # Solo se permite adelantar la ALU a un LOAD del bundle anterior.
        if (a[1] & 3)==1 and not a[0] and b[1]==a[1] and b[0] and b[2:]==a[2:]:
            assert i+1<len(seq), 'Falta el siguiente bundle'
            siguiente=slots(seq[i+1]); movido=slots(par[i+1])
            assert siguiente[0]==b[0] and siguiente[1:]==[0,0,0] and movido==[0,0,0,0], 'No se conservo el siguiente bundle como NOP'
        elif a[0] and a[1:]==[0,0,0] and b==[0,0,0,0]:
            assert i>=1 and slots(par[i-1])[0]==a[0] and slots(seq[i-1])[1]!=0, 'Se elimino una instruccion inesperada'
        else:
            raise AssertionError(f'Modificacion no permitida en bundle {i+1}')
assert exigir==0 or len(paralelos)>0, 'No hay bundles paralelos en el factorial'
assert len(cambios)==2*len(paralelos), 'Cantidad inesperada de modificaciones de instrucciones'

pc=None;n=0
for line in (p/'salida_resuelta.asm').read_text().splitlines():
    s=line.split('#',1)[0].split('//',1)[0].strip()
    if not s: continue
    if s=='fin_programa:': pc=n*16
    elif re.fullmatch(r'[A-Za-z_]\w*:',s): continue
    elif re.match(r'^(ALU|LSU|BRU|FEIST)\s*:',s,re.I): n+=1
    else: raise AssertionError('Linea ASM inesperada: '+s)
assert pc is not None and n==len(seq), 'PC final o cantidad de instrucciones incorrecta'
(p/'meta.env').write_text(f'FIN_PC={pc}\nBUNDLES={len(seq)}\nFUSIONES={len(paralelos)}\n')
# Pad a 4096 palabras para evitar advertencias readmemh sin cambiar posiciones.
for nombre in ['secuencial.mem','paralelo.mem']:
    v=(p/nombre).read_text().splitlines()
    (p/nombre).write_text('\n'.join(v+['0'*32]*(4096-len(v)))+'\n')
print(f'PASS: {len(seq)} bundles; PC final={pc}; bundles paralelos={paralelos}')
