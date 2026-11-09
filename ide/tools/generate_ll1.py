#!/usr/bin/env python3
"""Generación OFFLINE de tabla LL(1) fija del IDE (no se usa en ejecución).

Equivalencia sintáctica con proyecto-1/src/parser.y, corte 2026-11-03.
Cada regla está escrita sin acciones semánticas. Comprueba conflictos FIRST/FOLLOW.
Para regenerar: python3 ide/tools/generate_ll1.py
"""
from pathlib import Path
import json
import csv

IDE = Path(__file__).resolve().parents[1]
TOKENS = '''EOF TRAIGASE QUOTE EXT_E DEFINA ENTERO VOF VACIO PRINCIPAL LISTA MATRIZ
SI SINO SINOSI MIENTRAS PARA DEVUELVA ROMPA ID NUM TRUE FALSE
LP RP LB RB LC RC COMMA FIN ASSIGN OR AND COMP PLUS MINUS MUL DIV MOD POW'''.split()
assert len(TOKENS) < 64
GRAMMAR = '''
PROGRAM -> IMPORTS GLOBALS UNITS
IMPORTS -> IMPORT IMPORTS | ε
IMPORT -> TRAIGASE QUOTE ID EXT_E QUOTE FIN
GLOBALS -> DECL GLOBALS | ε
UNITS -> DEFINA DEF_TAIL | ε
DEF_TAIL -> ENTERO SIGNATURE UNITS | VOF SIGNATURE UNITS | VACIO VACIO_TAIL
SIGNATURE -> ID LP PARAMS RP BLOCK FIN
VACIO_TAIL -> ID LP PARAMS RP BLOCK FIN UNITS | PRINCIPAL LP RP BLOCK FIN
PARAMS -> PARAM PARAMS_MORE | ε
PARAMS_MORE -> COMMA PARAM PARAMS_MORE | ε
PARAM -> ENTERO PARAM_ENTERO | VOF ID
PARAM_ENTERO -> ID | LISTA ID LB DIM RB | MATRIZ ID LB DIM RB LB DIM RB
DIM -> NUM | ID
BLOCK -> LC STMTS RC
STMTS -> STMT STMTS | ε
STMT -> DECL | ID ID_STMT | IF | WHILE | FOR | RETURN | BREAK
DECL -> ENTERO DECL_ENTERO | VOF ID INIT FIN
DECL_ENTERO -> ID INIT FIN | LISTA ID LB DIM RB INIT_LIST FIN | MATRIZ ID LB DIM RB LB DIM RB INIT_MATRIX FIN
INIT -> ASSIGN EXPR | ε
INIT_LIST -> ASSIGN LB ELEMENTS RB | ε
INIT_MATRIX -> ASSIGN LB ROWS RB | ε
ROWS -> ROW ROWS_MORE
ROWS_MORE -> COMMA ROW ROWS_MORE | ε
ROW -> LB ELEMENTS RB
ELEMENTS -> EXPR ELEMENTS_MORE | ε
ELEMENTS_MORE -> COMMA EXPR ELEMENTS_MORE | ε
ID_STMT -> LP ARGS RP FIN | ACCESS_TAIL ASSIGN EXPR FIN
ACCESS_TAIL -> LB ARIT RB ACCESS_SECOND | ε
ACCESS_SECOND -> LB ARIT RB | ε
IF -> SI LP EXPR RP BLOCK IF_MORE
IF_MORE -> FIN | SINO BLOCK FIN | SINOSI LP EXPR RP BLOCK IF_MORE
WHILE -> MIENTRAS LP EXPR RP BLOCK FIN
FOR -> PARA LP FOR_INIT COMMA EXPR COMMA ASSIGNMENT RP BLOCK FIN
FOR_INIT -> ENTERO ID ASSIGN EXPR | ASSIGNMENT
ASSIGNMENT -> ID ACCESS_TAIL ASSIGN EXPR
RETURN -> DEVUELVA EXPR FIN
BREAK -> ROMPA FIN
ARGS -> EXPR ARGS_MORE | ε
ARGS_MORE -> COMMA EXPR ARGS_MORE | ε
EXPR -> AND_EXPR OR_TAIL
OR_TAIL -> OR AND_EXPR OR_TAIL | ε
AND_EXPR -> COMP_EXPR AND_TAIL
AND_TAIL -> AND COMP_EXPR AND_TAIL | ε
COMP_EXPR -> ARIT COMP_TAIL
COMP_TAIL -> COMP ARIT | ε
ARIT -> TERM ARIT_TAIL
ARIT_TAIL -> PLUS TERM ARIT_TAIL | MINUS TERM ARIT_TAIL | ε
TERM -> POWER TERM_TAIL
TERM_TAIL -> MUL POWER TERM_TAIL | DIV POWER TERM_TAIL | MOD POWER TERM_TAIL | ε
POWER -> UNARY POWER_TAIL
POWER_TAIL -> POW POWER | ε
UNARY -> MINUS UNARY | PRIMARY
PRIMARY -> NUM | TRUE | FALSE | ID PRIMARY_SUFFIX | LP EXPR RP
PRIMARY_SUFFIX -> LP ARGS RP | ACCESS_TAIL
'''.strip()

NTS, productions = [], []
for raw in GRAMMAR.splitlines():
    lhs, alternatives = [x.strip() for x in raw.split('->', 1)]
    assert lhs not in NTS
    NTS.append(lhs)
    for alt in alternatives.split('|'):
        rhs = [] if alt.strip() == 'ε' else alt.strip().split()
        productions.append((lhs, rhs))
for lhs, rhs in productions:
    for symbol in rhs:
        assert symbol in TOKENS or symbol in NTS, (lhs, rhs, symbol)

first = {n: set() for n in NTS}
follow = {n: set() for n in NTS}
nullable = {n: False for n in NTS}
follow['PROGRAM'].add('EOF')

def first_of(seq):
    terminals = set()
    for symbol in seq:
        if symbol in TOKENS:
            terminals.add(symbol)
            return terminals, False
        terminals.update(first[symbol])
        if not nullable[symbol]:
            return terminals, False
    return terminals, True

changed = True
while changed:
    changed = False
    for lhs, rhs in productions:
        fs, eps = first_of(rhs)
        if not fs <= first[lhs]:
            first[lhs].update(fs)
            changed = True
        if eps and not nullable[lhs]:
            nullable[lhs] = True
            changed = True
changed = True
while changed:
    changed = False
    for lhs, rhs in productions:
        for index, symbol in enumerate(rhs):
            if symbol not in NTS:
                continue
            fs, eps = first_of(rhs[index + 1:])
            adding = fs | (follow[lhs] if eps else set())
            if not adding <= follow[symbol]:
                follow[symbol].update(adding)
                changed = True

M = [[-1 for _ in TOKENS] for _ in NTS]
conflicts = []
for i, (lhs, rhs) in enumerate(productions):
    fs, eps = first_of(rhs)
    for terminal in fs | (follow[lhs] if eps else set()):
        r, c = NTS.index(lhs), TOKENS.index(terminal)
        if M[r][c] != -1:
            conflicts.append((lhs, terminal, productions[M[r][c]], (lhs, rhs)))
        M[r][c] = i
if conflicts:
    raise SystemExit('Gramática NO LL(1):\n' + '\n'.join(str(x) for x in conflicts))

max_rhs = max(map(lambda x: len(x[1]), productions))
assert max_rhs <= 14

def symbolic(s):
    return f'T_{s}' if s in TOKENS else f'N_{s}'

def macro(s):
    return f'T_{s}' if s in TOKENS else f'(T_COUNT+N_{s})'

def bitset(collection):
    value = sum(1 << TOKENS.index(x) for x in collection)
    return f'UINT64_C({value})'

lines = ['/* ARCHIVO GENERADO: NO EDITAR MANUALMENTE.',
         ' * python3 ide/tools/generate_ll1.py; gramática fija de parser.y.',
         ' * M[A,a] contiene número de producción, -1 equivale a error.',
         ' * Ver ide/DEFENSA_LL1.md para FIRST/FOLLOW y correspondencias. */',
         '#ifndef CDFF_LL1_TABLE_H', '#define CDFF_LL1_TABLE_H',
         '#include <stdint.h>',
         f'enum {{ {", ".join("T_" + t for t in TOKENS)}, T_COUNT }};',
         f'enum {{ {", ".join("N_" + n for n in NTS)}, N_COUNT }};',
         'typedef struct { uint8_t length; int16_t rhs[14]; } LL1Production;',
         '/* Identificador de terminal/NT: 0..T_COUNT-1 terminal; T_COUNT+N_* no terminal. */',
         f'static const LL1Production LL1_PRODUCTIONS[{len(productions)}] = {{']
for lhs,rhs in productions:
    values = ', '.join(macro(s) for s in rhs)
    lines.append(f'    {{{len(rhs)}, {{{values}}}}}, /* {lhs} -> {" ".join(rhs) or "ε"} */')
lines.append('};')
lines.append('/* TABLA PREDICTIVA LL(1) M[A,a], CONSTRUIDA CON FIRST Y FOLLOW. */')
lines.append('static const int16_t LL1_TABLE[N_COUNT][T_COUNT] = {')
for nt,row in zip(NTS,M):
    lines.append('    {' + ', '.join(str(x) for x in row) + '}, /* ' + nt + ' */')
lines.append('};')
lines.append('/* FIRST de cada NT (sin epsilon); LL1_NULLABLE almacena epsilon. */')
lines.append('static const uint64_t LL1_FIRST[N_COUNT] = {' + ', '.join(bitset(first[n]) for n in NTS) + '};')
lines.append('static const unsigned char LL1_NULLABLE[N_COUNT] = {' + ', '.join('1' if nullable[n] else '0' for n in NTS) + '};')
lines.append('static const uint64_t LL1_FOLLOW[N_COUNT] = {' + ', '.join(bitset(follow[n]) for n in NTS) + '};')
lines.append('static const char *const LL1_TERMINAL_NAMES[T_COUNT] = {' + ', '.join(json.dumps(t) for t in TOKENS) + '};')
lines.append('static const char *const LL1_NONTERMINAL_NAMES[N_COUNT] = {' + ', '.join(json.dumps(n) for n in NTS) + '};')
lines.append('#endif /* CDFF_LL1_TABLE_H */')
(IDE / 'll1_table.h').write_text('\n'.join(lines) + '\n')
# Vista amigable para la defensa. Esta CSV NO es consultada al ejecutar el IDE.
with (IDE / 'LL1_TABLA.csv').open('w', newline='', encoding='utf-8') as output:
    writer=csv.writer(output)
    writer.writerow(['No terminal / Token', *TOKENS])
    for nt,row in zip(NTS,M):
        writer.writerow([nt, *[('P'+str(v) if v>=0 else '') for v in row]])

# Referencia legible de construcción: documento de defensa + inspección humana.
report = ['# Tabla LL(1) estática: generación y verificación', '',
          'Se calcula offline con el algoritmo de PRIMERO / SIGUIENTE de clase.',
          'No se calcula FIRST ni FOLLOW al abrir el IDE. Tabla sin conflictos.', '',
          f'- Terminales: {len(TOKENS)}', f'- No terminales: {len(NTS)}',
          f'- Producciones: {len(productions)}', '',
          '## PRIMERO y SIGUIENTE (ε se muestra cuando es anulable)', '',
          '| A | PRIMERO(A) | SIGUIENTE(A) |', '|---|---|---|']
for n in NTS:
    a = sorted(first[n] | ({'ε'} if nullable[n] else set()))
    b = sorted(follow[n]); report.append(f'| `{n}` | {", ".join(a)} | {", ".join(b)} |')
report.extend(['', '## Producciones numeradas', ''])
for i,(lhs,rhs) in enumerate(productions):
    report.append(f'- P{i}: `{lhs} -> {" ".join(rhs) or "ε"}`')
(IDE / 'LL1_CONJUNTOS_Y_PRODUCCIONES.md').write_text('\n'.join(report) + '\n')
print(f'Sin conflictos: {len(TOKENS)} terminales, {len(NTS)} NT, {len(productions)} producciones; tabla estática generada.')
