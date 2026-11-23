#!/usr/bin/env python3
"""Simulador funcional VLIW por ciclos para el ensamblador textual de CDFF.

Modelo de EMISION ESTATICA: 1 bundle de 128 bits / ciclo, 4 slots
(ALU/LSU/BRU/FEIST), 16 bytes por bundle, x0=0, x1=PC en bytes,
RAW con separacion de 3 ciclos (scheduler.h), memoria de 64 KiB.

Las operaciones del MISMO bundle leen el estado previo y se consolidan al
final del ciclo; NO hay forwarding dentro del bundle. Las escrituras se
calculan funcionalmente en emision y el scoreboard comprueba que el codigo
ya haya insertado los NOP necesarios. No inserta stalls dinamicos ni
simula IF/ID/EX/WB a nivel RTL. FEIST de cifrado requiere RTL y se rechaza.

Compatibilidad: python3 pruebas/simular_asm_ir.py programa.asm direccion esperado
Evidencia: --traza-csv ciclos.csv --resumen-json resumen.json
Comparacion: --comparar otro.asm (ambos programas deben dar el esperado)
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from collections import Counter, deque
from dataclasses import dataclass
from pathlib import Path

SLOTS = ('ALU', 'LSU', 'BRU', 'FEIST')
SEPARACION_RAW = 3  # scheduler.h: SCHEDULER_SEPARACION_RAW
BUNDLE_BYTES = 16
MEM_BYTES = 65536
CAMPOS_TRAZA = ['ciclo', 'bundle_pc', 'pc_bytes', 'ALU', 'LSU', 'BRU', 'FEIST',
                'lecturas', 'escrituras', 'memoria', 'salto', 'pc_siguiente', 'riesgos_RAW']
REG_RE = re.compile(r'x(1[0-5]|[0-9])\Z')
MEM_RE = re.compile(r'(-?\d+)\((x(?:1[0-5]|[0-9]))\)\Z')
INT_RE = re.compile(r'[+-]?(?:0[xX][0-9a-fA-F]+|\d+)\Z')


class ErrorSimulacion(ValueError):
    """ASM invalido, fallo de ejecucion o riesgo en el bundle."""


def i32(n):
    n = int(n) & 0xffffffff
    return n - 0x100000000 if n & 0x80000000 else n


def indice_registro(nombre):
    if not REG_RE.fullmatch(nombre):
        raise ErrorSimulacion(f'Registro invalido: {nombre}')
    return int(nombre[1:])


@dataclass(frozen=True)
class Operacion:
    slot: str
    texto: str
    tokens: tuple[str, ...]


@dataclass
class Programa:
    bundles: list[dict[str, Operacion]]
    etiquetas: dict[str, int]


def cargar_programa(ruta):
    etiquetas = {}
    bundles = []
    for num, raw in enumerate(Path(ruta).read_text(encoding='utf-8').splitlines(), 1):
        linea = re.split(r'//|#', raw, maxsplit=1)[0].strip()
        if not linea:
            continue
        if linea.endswith(':'):
            etiqueta = linea[:-1]
            if not re.fullmatch(r'[A-Za-z_]\w*', etiqueta):
                raise ErrorSimulacion(f'Linea {num}: etiqueta invalida {etiqueta!r}')
            if etiqueta in etiquetas:
                raise ErrorSimulacion(f'Linea {num}: etiqueta duplicada {etiqueta}')
            etiquetas[etiqueta] = len(bundles)
            continue
        bundle = {}
        for pieza in linea.split('|'):
            match = re.fullmatch(r'\s*(ALU|LSU|BRU|FEIST):\s*(\S.*?)\s*', pieza)
            if not match:
                raise ErrorSimulacion(f'Linea {num}: instruccion/slot invalido: {pieza!r}')
            slot, texto = match.groups()
            if slot in bundle:
                raise ErrorSimulacion(f'Linea {num}: slot repetido dentro de bundle: {slot}')
            tokens = tuple(texto.replace(',', ' ').split())
            if not tokens:
                raise ErrorSimulacion(f'Linea {num}: operacion vacia')
            bundle[slot] = Operacion(slot, texto, tokens)
        bundles.append(bundle)
    if not bundles:
        raise ErrorSimulacion(f'ASM sin instrucciones: {ruta}')
    return Programa(bundles, etiquetas)


def obtener_registros(op):
    """Devuelve (lecturas, escrituras) segun scheduler.c para la ISA soportada."""
    t = op.tokens
    mn = t[0]
    if mn == 'NOP':
        if len(t) != 1:
            raise ErrorSimulacion(f'NOP mal formado: {op.texto}')
        return set(), set()
    if op.slot == 'ALU':
        if mn in ('ADD', 'SUB', 'AND', 'OR', 'XOR', 'SLT') and len(t) == 4:
            d, a, b = map(indice_registro, t[1:])
            return {x for x in (a, b) if x >= 2}, {d} if d >= 2 else set()
        if mn in ('ADDI', 'SLLI', 'SRLI') and len(t) == 4:
            d, a = map(indice_registro, t[1:3])
            return {a} if a >= 2 else set(), {d} if d >= 2 else set()
    elif op.slot == 'LSU':
        if mn in ('LOAD', 'STORE') and len(t) == 3:
            r = indice_registro(t[1])
            m = MEM_RE.fullmatch(t[2])
            if not m:
                raise ErrorSimulacion(f'Acceso de memoria mal formado: {op.texto}')
            b = indice_registro(m.group(2))
            fuentes = {b} if b >= 2 else set()
            if mn == 'LOAD':
                return fuentes, {r} if r >= 2 else set()
            return fuentes | ({r} if r >= 2 else set()), set()
    elif op.slot == 'BRU':
        if mn == 'JMP' and len(t) == 2:
            return set(), set()
        if mn in ('BEQ', 'BNE', 'BLE', 'BGE') and len(t) == 4:
            a, b = map(indice_registro, t[1:3])
            return {x for x in (a, b) if x >= 2}, set()
    elif op.slot == 'FEIST':
        raise ErrorSimulacion('FEIST requiere simulacion en RTL; no existe modelo funcional FEIST aqui')
    raise ErrorSimulacion(f'Operacion no soportada o slot incorrecto: {op.slot}: {op.texto}')


def destino_salto(texto, pc, programa):
    if INT_RE.fullmatch(texto):
        desplazamiento = int(texto, 0)
        destino_bytes = pc * BUNDLE_BYTES + desplazamiento
        if destino_bytes % BUNDLE_BYTES:
            raise ErrorSimulacion(f'Salto desalineado pc={pc}, offset={desplazamiento}')
        destino = destino_bytes // BUNDLE_BYTES
    elif texto in programa.etiquetas:
        destino = programa.etiquetas[texto]
    else:
        raise ErrorSimulacion(f'Etiqueta de salto inexistente: {texto}')
    if not 0 <= destino < len(programa.bundles):
        raise ErrorSimulacion(f'Salto fuera del programa: bundle {destino}')
    return destino


def simular(ruta, direccion, esperado, *, max_ciclos=1500000,
            traza_csv=None, resumen_json=None, validar_raw=False,
            muestra_ciclos=0, rango_ciclos=None):
    programa = cargar_programa(ruta)
    if direccion is not None and (direccion % 4 or not 0 <= direccion <= MEM_BYTES - 4):
        raise ErrorSimulacion(f'Direccion de resultado invalida: {direccion}')
    regs = [0] * 16
    mem = {}
    disponible = [1] * 16
    pc = 0
    ciclo = 0
    estadisticas = Counter()
    riesgos = []
    etiquetas = programa.etiquetas
    fin = etiquetas.get('fin_programa')
    escritor = None
    out_csv = None
    primera_muestra = []
    ultima_muestra = deque(maxlen=max(0, muestra_ciclos))
    muestra_rango = []
    if traza_csv:
        destino = Path(traza_csv)
        destino.parent.mkdir(parents=True, exist_ok=True)
        fichero = destino.open('w', newline='', encoding='utf-8')
        escritor = csv.writer(fichero)
        escritor.writerow(CAMPOS_TRAZA)
        out_csv = fichero
    try:
        while 0 <= pc < len(programa.bundles) and pc != fin:
            if ciclo >= max_ciclos:
                raise ErrorSimulacion(f'Se supero el limite de {max_ciclos} ciclos, PC={pc}')
            ciclo += 1
            bundle = programa.bundles[pc]
            operaciones = [bundle[slot] for slot in SLOTS if slot in bundle]
            sets = [(op, *obtener_registros(op)) for op in operaciones]
            # Emision simultanea: no hay forwarding entre slots del mismo bundle.
            for n, (op, reads, writes) in enumerate(sets):
                for otro, other_reads, other_writes in sets[n + 1:]:
                    for r in sorted(writes & (other_reads | other_writes)):
                        raise ErrorSimulacion(f'Bundle {pc}: dependencia intra-bundle sobre x{r}: '
                                              f'{op.texto} / {otro.texto}')
                    for r in sorted(reads & other_writes):
                        raise ErrorSimulacion(f'Bundle {pc}: dependencia intra-bundle sobre x{r}: '
                                              f'{op.texto} / {otro.texto}')
            reads = set().union(*(r for _, r, _ in sets)) if sets else set()
            writes = set().union(*(w for _, _, w in sets)) if sets else set()
            riesgos_ciclo = [f'x{r}: listo en ciclo {disponible[r]}' for r in sorted(reads)
                             if ciclo < disponible[r]]
            if riesgos_ciclo:
                riesgos.append({'ciclo': ciclo, 'pc': pc, 'riesgos': riesgos_ciclo})
                if validar_raw:
                    raise ErrorSimulacion(f'RAW antes de WB: ciclo {ciclo}, PC={pc}: '
                                          + ', '.join(riesgos_ciclo))
            # Captura simultanea de fuentes; las escrituras no afectan lecturas hermanas.
            snapshot = regs[:]
            def read(s):
                n = indice_registro(s)
                return 0 if n == 0 else pc * BUNDLE_BYTES if n == 1 else snapshot[n]
            def immed(s):
                if INT_RE.fullmatch(s):
                    return int(s, 0)
                return (destino_salto(s, pc, programa) - pc) * BUNDLE_BYTES
            nuevos_regs = {}
            nuevos_mem = {}
            nuevo_pc = pc + 1
            redirecciona_pc = None
            eventos_mem = []
            salto = ''
            for op, _, _ in sets:
                t = op.tokens
                mn = t[0]
                if mn == 'NOP':
                    continue
                estadisticas['operaciones'] += 1
                estadisticas[f'ops_{op.slot}'] += 1
                if op.slot == 'ALU':
                    d, a, b = t[1:]
                    av = read(a)
                    if mn in ('ADD', 'SUB', 'AND', 'OR', 'XOR', 'SLT'):
                        bv = read(b)
                        resultado = {'ADD': lambda: av + bv, 'SUB': lambda: av - bv,
                                     'AND': lambda: av & bv, 'OR': lambda: av | bv,
                                     'XOR': lambda: av ^ bv, 'SLT': lambda: int(av < bv)}[mn]()
                    else:
                        bv = immed(b)
                        if mn in ('SLLI', 'SRLI') and not 0 <= bv <= 31:
                            raise ErrorSimulacion(f'Shift invalido: {op.texto}')
                        if mn == 'ADDI': resultado = av + bv
                        elif mn == 'SLLI': resultado = av << bv
                        else: resultado = (av & 0xffffffff) >> bv
                    r = indice_registro(d)
                    if r == 1:
                        redirecciona_pc = i32(resultado)
                    elif r != 0:
                        nuevos_regs[r] = i32(resultado)
                elif op.slot == 'LSU':
                    r = indice_registro(t[1])
                    match = MEM_RE.fullmatch(t[2])
                    off, base = int(match.group(1)), match.group(2)
                    dir_mem = i32(read(base) + off)
                    if dir_mem % 4 or not 0 <= dir_mem <= MEM_BYTES - 4:
                        raise ErrorSimulacion(f'Acceso fuera de memoria o desalineado: '
                                              f'ciclo {ciclo}, {op.texto}, direccion {dir_mem}')
                    if mn == 'LOAD':
                        resultado = mem.get(dir_mem, 0)
                        if r == 1:
                            redirecciona_pc = i32(resultado)
                        elif r != 0:
                            nuevos_regs[r] = i32(resultado)
                        estadisticas['loads'] += 1
                        eventos_mem.append(f'LOAD [{dir_mem}]={resultado}')
                    else:
                        nuevos_mem[dir_mem] = read(t[1])
                        estadisticas['stores'] += 1
                        eventos_mem.append(f'STORE [{dir_mem}]={nuevos_mem[dir_mem]}')
                elif op.slot == 'BRU':
                    if mn == 'JMP':
                        nuevo_pc = destino_salto(t[1], pc, programa)
                        salto = f'JMP->{nuevo_pc}'
                        estadisticas['saltos_tomados'] += 1
                    else:
                        a, b, etiqueta = t[1:]
                        av, bv = read(a), read(b)
                        tomar = {'BEQ': av == bv, 'BNE': av != bv,
                                 'BLE': av <= bv, 'BGE': av >= bv}[mn]
                        estadisticas['condicionales'] += 1
                        if tomar:
                            nuevo_pc = destino_salto(etiqueta, pc, programa)
                            estadisticas['saltos_tomados'] += 1
                        salto = f'{mn} {"T" if tomar else "NT"}->{nuevo_pc}'
            if redirecciona_pc is not None:
                if salto:
                    raise ErrorSimulacion(f'Bundle {pc}: PC modificado tanto por x1 como por BRU')
                if redirecciona_pc % BUNDLE_BYTES:
                    raise ErrorSimulacion(f'PC x1 desalineado: {redirecciona_pc}')
                nuevo_pc = redirecciona_pc // BUNDLE_BYTES
                salto = f'x1->{nuevo_pc}'
                estadisticas['saltos_tomados'] += 1
            # Escritura funcional al finalizar el ciclo; scoreboard modela WB +3.
            for r, val in nuevos_regs.items():
                regs[r] = val
                disponible[r] = ciclo + SEPARACION_RAW
            for d, val in nuevos_mem.items():
                mem[d] = i32(val)
            estadisticas['bundles_emitidos'] += 1
            if not any(op.tokens[0] != 'NOP' for op in operaciones):
                estadisticas['bundles_nop'] += 1
            if len([op for op in operaciones if op.tokens[0] != 'NOP']) > 1:
                estadisticas['bundles_paralelos'] += 1
            if escritor or muestra_ciclos or rango_ciclos:
                fila = dict(zip(CAMPOS_TRAZA, [
                    ciclo, pc, pc * BUNDLE_BYTES,
                    *(bundle[slot].texto if slot in bundle else '-' for slot in SLOTS),
                    ' '.join(f'x{r}={snapshot[r]}' for r in sorted(reads)),
                    ' '.join(f'x{r}={v}' for r, v in sorted(nuevos_regs.items())),
                    '; '.join(eventos_mem), salto, nuevo_pc, '; '.join(riesgos_ciclo),
                ]))
                if escritor:
                    escritor.writerow([fila[k] for k in CAMPOS_TRAZA])
                if muestra_ciclos:
                    if len(primera_muestra) < muestra_ciclos:
                        primera_muestra.append(fila)
                    ultima_muestra.append(fila)
                if rango_ciclos and rango_ciclos[0] <= ciclo <= rango_ciclos[1]:
                    muestra_rango.append(fila)
            pc = nuevo_pc
        if fin is None and not 0 <= pc < len(programa.bundles):
            # Terminar al caer fuera del programa es valido para pruebas pequenas.
            pass
        obtenido = mem.get(direccion, 0) if direccion is not None else None
        if esperado is not None and obtenido != esperado:
            raise ErrorSimulacion(f'{ruta}: RAM[{direccion}]={obtenido}, '
                                  f'esperado {esperado}; ciclo={ciclo}, PC={pc}')
        estadisticas['ciclos'] = ciclo
        estadisticas['bundles_estaticos'] = len(programa.bundles)
        estadisticas['riesgos_raw'] = len(riesgos)
        salida = {'archivo': str(ruta), 'modelo': 'VLIW-emision-estatica-no-RTL',
                  'separacion_raw': SEPARACION_RAW, 'bundle_bytes': BUNDLE_BYTES,
                  'resultado': {'direccion': direccion, 'obtenido': obtenido,
                                'esperado': esperado, 'correcto': esperado is None or obtenido == esperado},
                  'estadisticas': dict(estadisticas), 'riesgos': riesgos[:100],
                  'pc_final': pc, 'registros_finales': {f'x{i}': regs[i] for i in range(2, 16)},
                  'memoria_escrita': {str(k): v for k, v in sorted(mem.items())}}
        if muestra_ciclos or rango_ciclos:
            salida['_vista_ciclos'] = {'primeros': primera_muestra, 'ultimos': list(ultima_muestra),
                                      'rango': muestra_rango}
        if resumen_json:
            archivo_json = Path(resumen_json)
            archivo_json.parent.mkdir(parents=True, exist_ok=True)
            archivo_json.write_text(json.dumps(salida, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        return salida
    finally:
        if out_csv:
            out_csv.close()


def ejecutar(ruta, direccion, esperado):
    """API historica para scripts existentes; equivale al modo funcional."""
    salida = simular(ruta, direccion, esperado)
    print(f'PASS: {ruta}: RAM[{direccion}] = {salida["resultado"]["obtenido"]} '
          f'(esperado {esperado}), instrucciones ejecutadas='
          f'{salida["estadisticas"]["operaciones"]}, ciclos='
          f'{salida["estadisticas"]["ciclos"]}')
    return salida


# ---------------------------------------------------------------
# Capa de presentacion: no cambia las reglas ni el motor VLIW.
# ---------------------------------------------------------------

def _numero(valor):
    return f'{valor:,}'.replace(',', ' ')


def _recortar(valor, limite):
    valor = str(valor or '-').replace('\n', ' ')
    return valor if len(valor) <= limite else valor[:limite - 1] + '…'


def _encabezado(titulo):
    print('\n' + '=' * 78)
    print('  ' + titulo)
    print('=' * 78)


def _estado(salida):
    r = salida['resultado']
    if r['esperado'] is None:
        return 'SIN COMPROBAR (se ejecuto, pero no se especifico resultado esperado)'
    return (f'CORRECTO: RAM[{r["direccion"]}] = {r["obtenido"]} '
            f'(esperado {r["esperado"]})')


def _estadisticas(salida):
    e = salida['estadisticas']
    return {
        'Ciclos emitidos (dinamicos)': e.get('ciclos', 0),
        'Bundles en el programa': e.get('bundles_estaticos', 0),
        'Bundles paralelos ejecutados': e.get('bundles_paralelos', 0),
        'Bundles NOP ejecutados': e.get('bundles_nop', 0),
        'Operaciones ALU': e.get('ops_ALU', 0),
        'Operaciones LSU': e.get('ops_LSU', 0),
        'Operaciones BRU': e.get('ops_BRU', 0),
        'LOAD ejecutados': e.get('loads', 0),
        'STORE ejecutados': e.get('stores', 0),
        'Saltos tomados': e.get('saltos_tomados', 0),
        'Riesgos RAW detectados': e.get('riesgos_raw', 0),
    }


def _reporte_una(salida, *, etiqueta='SIMULACION'):
    _encabezado(etiqueta + '  |  CDFF - VLIW / codegen_ir')
    print('Archivo:    ' + salida['archivo'])
    print('Resultado:  ' + _estado(salida))
    print('Modelo:     1 bundle emitido = 1 ciclo del modelo funcional (no RTL)')
    print('-' * 78)
    for clave, valor in _estadisticas(salida).items():
        print(f'  {clave:<36} {_numero(valor):>12}')
    rams = list(salida['memoria_escrita'].items())[-8:]
    if salida['resultado']['esperado'] is None:
        print('\n  Ultimas posiciones de RAM escritas (NO son resultados verificados):')
        print('  ' + (', '.join(f'[{d}]={v}' for d, v in rams) if rams else '(ninguna)'))
    if salida['estadisticas'].get('riesgos_raw', 0):
        print('\n  ATENCION: se detectaron dependencias RAW segun separacion de 3 ciclos.')
        print('  Revisar la calendarizacion y contrastar con RTL de Arquitectura.')


def _porcentaje(antes, despues):
    return (f'{(despues - antes):+d} ({100 * (despues - antes) / antes:+.2f}%)'
            if antes else f'{(despues - antes):+d} (base 0)')


def _reporte_comparacion(base, opt):
    _encabezado('COMPARACION  |  Antes vs. despues')
    print('BASE:       ' + base['archivo'])
    print('COMPARADO:  ' + opt['archivo'])
    print('BASE:       ' + _estado(base))
    print('COMPARADO:  ' + _estado(opt))
    print('-' * 78)
    print(f'{"METRICA":<35} {"BASE":>11} {"COMPARADO":>11}  {"CAMBIO":>13}')
    print('-' * 78)
    b, o = _estadisticas(base), _estadisticas(opt)
    for k in b:
        print(f'{k:<35} {_numero(b[k]):>11} {_numero(o[k]):>11}  {_porcentaje(b[k], o[k]):>13}')
    print('-' * 78)
    if base['resultado']['esperado'] is not None:
        print('RESULTADOS: ambos resultados comprobados contra el valor esperado.')
    else:
        print('RESULTADOS: NO verificados. Los ciclos pueden compararse, no la equivalencia.')
    print('IMPORTANTE: los ciclos son de emision funcional, NO ciclos totales RTL.')


def _filas_elegidas(salida, rango_ciclos):
    muestra = salida.get('_vista_ciclos', {})
    if rango_ciclos:
        return muestra.get('rango', [])
    primeras = muestra.get('primeros', [])
    ultimas = muestra.get('ultimos', [])
    ya = {f['ciclo'] for f in primeras}
    if ultimas and ultimas[0]['ciclo'] > len(primeras) + 1:
        return primeras + [None] + [f for f in ultimas if f['ciclo'] not in ya]
    return primeras + [f for f in ultimas if f['ciclo'] not in ya]


def _mostrar_ciclos(salida, rango_ciclos=None):
    filas = _filas_elegidas(salida, rango_ciclos)
    if not filas:
        print('\nSin ciclos en el rango solicitado.')
        return
    titulo = (f'CICLOS {rango_ciclos[0]} A {rango_ciclos[1]}' if rango_ciclos
              else 'MUESTRA: primeros y ultimos ciclos')
    print('\n  ' + titulo + ' (para conservar todos los ciclos usa --guardar)')
    print('  ' + '-' * 110)
    print(f'  {"CICLO":>6} {"PC":>5}  {"ALU":<24} {"LSU":<25} {"BRU":<22}  EVENTO')
    print('  ' + '-' * 110)
    for fila in filas:
        if fila is None:
            print('    ... ciclos intermedios omitidos ...')
            continue
        eventos = '; '.join(x for x in (fila['memoria'], fila['salto'], fila['riesgos_RAW']) if x)
        print(f'  {str(fila["ciclo"]):>6} {str(fila["bundle_pc"]):>5}  '
              f'{_recortar(fila["ALU"], 24):<24} {_recortar(fila["LSU"], 25):<25} '
              f'{_recortar(fila["BRU"], 22):<22}  {_recortar(eventos, 35)}')


def _reporte_texto(base, opt=None):
    lineas = ['REPORTE DE SIMULACION VLIW - CDFF / codegen_ir',
              '=' * 60, 'Modelo: emision estatica funcional (NO son ciclos RTL)',
              'Base: ' + base['archivo'], 'Estado: ' + _estado(base)]
    if opt:
        lineas.extend(['Comparado: ' + opt['archivo'], 'Estado: ' + _estado(opt)])
    lineas.append('')
    if opt:
        lineas.append(f'{"METRICA":<36} {"BASE":>12} {"COMPARADO":>12}  CAMBIO')
        for nombre, valor in _estadisticas(base).items():
            nuevo = _estadisticas(opt)[nombre]
            lineas.append(f'{nombre:<36} {valor:>12} {nuevo:>12}  {_porcentaje(valor, nuevo)}')
    else:
        for nombre, valor in _estadisticas(base).items():
            lineas.append(f'{nombre:<36} {valor:>12}')
    lineas.extend(['', 'Interpretacion:',
                   '- Los resultados se verifican solo si se indico una direccion RAM y un valor esperado.',
                   '- El cambio en ciclos no implica necesariamente una mejora en el pipeline RTL.',
                   '- Para la defensa, corroborar el resultado en la arquitectura SystemVerilog.'])
    return '\n'.join(lineas) + '\n'


def _rango(texto):
    m = re.fullmatch(r'(\d+)\s*:\s*(\d+)', texto.strip())
    if not m:
        raise argparse.ArgumentTypeError('Use INICIO:FIN, por ejemplo 100:112')
    inicio, fin = map(int, m.groups())
    if inicio < 1 or fin < inicio or fin - inicio > 49:
        raise argparse.ArgumentTypeError('Rango invalido; maximo 50 ciclos por consulta')
    return inicio, fin


def _guardar_rutas(a):
    csv_base = a.traza_csv
    json_base = a.resumen_json
    if a.guardar:
        carpeta = Path(a.carpeta_salida)
        carpeta.mkdir(parents=True, exist_ok=True)
        nombre = Path(a.asm).stem
        if not csv_base:
            csv_base = str(carpeta / f'{nombre}_ciclos.csv')
        if not json_base:
            json_base = str(carpeta / f'{nombre}_resumen.json')
    return csv_base, json_base


def _ruta_comparada(ruta):
    if not ruta:
        return None
    p = Path(ruta)
    return str(p.with_name(p.stem + '_comparado' + p.suffix))


def _procesar(a):
    if a.max_ciclos <= 0:
        raise ErrorSimulacion('--max-ciclos debe ser positivo')
    if a.resultado is not None and (a.direccion is not None or a.esperado is not None):
        raise ErrorSimulacion('Use --resultado DIRECCION VALOR o los dos argumentos posicionales, no ambos')
    direccion, esperado = a.resultado if a.resultado is not None else (a.direccion, a.esperado)
    if (direccion is None) != (esperado is None):
        raise ErrorSimulacion('Indique direccion Y valor esperado, o ninguno')
    csv_base, json_base = _guardar_rutas(a)
    mostrar_n = 0 if a.solo_resumen or a.ciclos else a.vista
    base = simular(a.asm, direccion, esperado, max_ciclos=a.max_ciclos,
                   traza_csv=csv_base, resumen_json=json_base, validar_raw=a.validar_raw,
                   muestra_ciclos=mostrar_n, rango_ciclos=a.ciclos)
    _reporte_una(base, etiqueta='PROGRAMA BASE' if a.comparar else 'SIMULACION')
    if not a.solo_resumen and (mostrar_n or a.ciclos):
        _mostrar_ciclos(base, a.ciclos)
    opt = None
    if a.comparar:
        opt = simular(a.comparar, direccion, esperado, max_ciclos=a.max_ciclos,
                      traza_csv=_ruta_comparada(csv_base), resumen_json=_ruta_comparada(json_base),
                      validar_raw=a.validar_raw, muestra_ciclos=mostrar_n, rango_ciclos=a.ciclos)
        _reporte_comparacion(base, opt)
        if not a.solo_resumen and (mostrar_n or a.ciclos):
            print('\n  VISTA DEL PROGRAMA COMPARADO:')
            _mostrar_ciclos(opt, a.ciclos)
    reporte_txt = None
    if a.guardar:
        p = Path(a.carpeta_salida) / (Path(a.asm).stem + '_reporte.txt')
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(_reporte_texto(base, opt), encoding='utf-8')
        reporte_txt = str(p)
    if csv_base or json_base or reporte_txt:
        print('\n  Evidencias guardadas:')
        for ruta in [csv_base, _ruta_comparada(csv_base) if a.comparar else None,
                     json_base, _ruta_comparada(json_base) if a.comparar else None,
                     reporte_txt]:
            if ruta:
                print('    ' + ruta)
    return 0


def _encontrar_asm():
    raiz = Path.cwd()
    archivos = set()
    for carpeta in [raiz / 'evidencia-vliw', raiz / 'pruebas', raiz / 'docs']:
        if carpeta.is_dir():
            archivos.update(p for p in carpeta.rglob('*.asm') if p.is_file())
    archivos.update(p for p in raiz.glob('*.asm') if p.is_file())
    ordenados = sorted(archivos, key=lambda p: p.stat().st_mtime, reverse=True)
    personales = [p for p in ordenados if '.make-tests' not in p.parts]
    return personales if personales else ordenados


def _seleccionar_asm(titulo):
    print('\n' + titulo)
    archivos = _encontrar_asm()[:15]
    if archivos:
        print('  Archivos ASM recientes:')
        for n, p in enumerate(archivos, 1):
            try:
                etiqueta = str(p.relative_to(Path.cwd()))
            except ValueError:
                etiqueta = str(p)
            print(f'    {n:>2}. {etiqueta}')
    while True:
        respuesta = input('\n  Numero de la lista o ruta al .asm (0 = cancelar): ').strip().strip('"\'')
        if respuesta == '0':
            return None
        if respuesta.isdigit() and 1 <= int(respuesta) <= len(archivos):
            return str(archivos[int(respuesta) - 1])
        ruta = Path(respuesta).expanduser()
        if ruta.is_file():
            return str(ruta)
        print('  No se encontro el archivo. Escribi un numero o una ruta valida.')


def _consultar_resultado():
    print('\n  Opcional: verificar el valor final en memoria RAM.')
    print('  Ejemplo factorial(5): direccion 65528, valor 120 (segun el caso validado).')
    while True:
        direccion = input('  Direccion RAM (Enter = no verificar): ').strip()
        if not direccion:
            return None
        esperado = input('  Valor esperado: ').strip()
        try:
            return int(direccion, 0), int(esperado, 0)
        except ValueError:
            print('  Ambos valores deben ser numeros enteros. Intentá de nuevo.')


def _ver_intervalo_guardado(csv_ruta):
    if not csv_ruta:
        return
    while True:
        texto = input('\n  ¿Queres ver otros ciclos? Escribi INICIO:FIN (Enter = terminar): ').strip()
        if not texto:
            return
        try:
            inicio, fin = _rango(texto)
        except argparse.ArgumentTypeError as e:
            print('  ' + str(e))
            continue
        filas = []
        with open(csv_ruta, newline='', encoding='utf-8') as f:
            for fila in csv.DictReader(f):
                try:
                    ciclo = int(fila['ciclo'])
                except (ValueError, KeyError):
                    continue
                if inicio <= ciclo <= fin:
                    filas.append(fila)
                if ciclo > fin:
                    break
        _mostrar_ciclos({'_vista_ciclos': {'rango': filas}}, (inicio, fin))


def _interactivo():
    _encabezado('SIMULADOR VLIW  |  MODO GUIADO EN TERMINAL')
    print('  Compilá primero tu archivo .e con: make asm FILE=... ARGS="--codegen-ir"')
    print('  Este programa ejecuta los .asm, NO compila archivos .e.')
    print('  1. Simular un programa')
    print('  2. Comparar dos programas (sin/con optimizacion)')
    print('  0. Salir')
    while True:
        opcion = input('\n  Elegi una opcion [1/2/0]: ').strip()
        if opcion in ('0', '1', '2'):
            break
        print('  Opcion invalida.')
    if opcion == '0':
        return 0
    base = _seleccionar_asm('Seleccioná el ASM base:')
    if base is None:
        return 0
    segundo = _seleccionar_asm('Seleccioná el ASM optimizado:') if opcion == '2' else None
    if opcion == '2' and segundo is None:
        return 0
    verificacion = _consultar_resultado()
    a = argparse.Namespace(asm=base, comparar=segundo,
                           direccion=None, esperado=None, resultado=verificacion,
                           max_ciclos=1500000, traza_csv=None, resumen_json=None,
                           validar_raw=False, guardar=True,
                           carpeta_salida='evidencia-vliw/simulaciones',
                           vista=5, ciclos=None, solo_resumen=False)
    codigo = _procesar(a)
    if codigo == 0:
        csv_base, _ = _guardar_rutas(a)
        _ver_intervalo_guardado(csv_base)
        if segundo:
            _ver_intervalo_guardado(_ruta_comparada(csv_base))
    return codigo


def main(argv=None):
    parser = argparse.ArgumentParser(
        description='Simulador VLIW para ASM de codegen_ir. Sin argumentos inicia un menu guiado.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('asm', nargs='?', help='Archivo .asm a simular')
    parser.add_argument('direccion', type=int, nargs='?', help='Direccion de RAM (compatibilidad)')
    parser.add_argument('esperado', type=int, nargs='?', help='Valor esperado (compatibilidad)')
    parser.add_argument('--resultado', type=int, nargs=2, metavar=('DIRECCION', 'VALOR'),
                        help='Verificar RAM[DIR]=VALOR. Ej.: --resultado 65528 120')
    parser.add_argument('--comparar', help='Segundo .asm: antes vs despues de optimizar')
    parser.add_argument('--guardar', action='store_true',
                        help='Guardar automaticamente CSV y JSON de cada simulacion')
    parser.add_argument('--carpeta-salida', default='evidencia-vliw/simulaciones',
                        help='Carpeta de evidencias al usar --guardar')
    parser.add_argument('--traza-csv', help='Ruta personalizada del CSV ciclo por ciclo')
    parser.add_argument('--resumen-json', help='Ruta personalizada del JSON de resultados')
    parser.add_argument('--vista', type=int, default=5,
                        help='Mostrar primeros y ultimos N ciclos en la terminal')
    parser.add_argument('--ciclos', type=_rango, metavar='INICIO:FIN',
                        help='Mostrar un intervalo concreto de hasta 50 ciclos (1:10)')
    parser.add_argument('--solo-resumen', action='store_true',
                        help='No mostrar tabla de ciclos; solo estadisticas')
    parser.add_argument('--validar-raw', action='store_true',
                        help='Detener la simulacion si hay un riesgo RAW detectado')
    parser.add_argument('--max-ciclos', type=int, default=1500000,
                        help='Limite de ciclos para evitar loops infinitos')
    a = parser.parse_args(argv)
    if a.vista < 0 or a.vista > 50:
        parser.error('--vista debe estar entre 0 y 50')
    if a.asm is None:
        if a.direccion is not None or a.esperado is not None or a.comparar or a.resultado:
            parser.error('Hace falta el archivo ASM de entrada')
        if not sys.stdin.isatty():
            parser.print_help()
            return 2
        try:
            return _interactivo()
        except (EOFError, KeyboardInterrupt):
            print('\nSimulacion cancelada.')
            return 130
        except (ErrorSimulacion, OSError) as exc:
            print(f'ERROR: {exc}', file=sys.stderr)
            return 1
    try:
        return _procesar(a)
    except (ErrorSimulacion, OSError) as exc:
        print(f'ERROR de simulacion: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
