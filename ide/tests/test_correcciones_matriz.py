#!/usr/bin/env python3
"""Regresión de correcciones de CDFF: valida ediciones exactas y reanálisis.
No requiere pytest ni el ejecutable cdff; usa el analizador C del IDE.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

IDE = Path(__file__).resolve().parents[1]
EXEC = IDE / 'ide_syntax'
# Se recompila el analizador antes de probar: no depende de un binario anterior.
SRC = IDE / 'ide_syntax.c'
if not EXEC.exists() or EXEC.stat().st_mtime < SRC.stat().st_mtime:
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O2',
                    '-o',str(EXEC),str(SRC)],check=True)
BASE = 'defina vacio principal() {\n    entero edad = 21 :)\n} :)\n'


def analyze(code):
    with tempfile.NamedTemporaryFile('w', encoding='utf-8', suffix='.e') as f:
        f.write(code); f.flush()
        p = subprocess.run([str(EXEC), f.name], capture_output=True, text=True, timeout=5)
    if p.returncode:
        raise RuntimeError(p.stderr or 'El analizador C no pudo ejecutar')
    out = []
    for row in p.stdout.splitlines():
        x = row.split('\t', 5)
        if len(x) != 6:
            raise AssertionError(f'Respuesta inválida de C: {row!r}')
        out.append(dict(line=int(x[0]), col=int(x[1]), action=x[2], expected=x[3], replacement=x[4], message=x[5]))
    return out


def apply(code, suggestion):
    lines = code.split('\n');i=suggestion['line']-1
    assert 0 <= i < len(lines)
    if suggestion['action']=='APPEND_BLOCK':
        return code.rstrip()+'\n} :)\n'
    j=suggestion['col'];old=suggestion['expected'];new=suggestion['replacement']
    assert lines[i][j:j+len(old)] == old
    lines[i]=lines[i][:j]+new+lines[i][j+len(old):]
    return '\n'.join(lines)


class Matriz(unittest.TestCase):
    def test_correct_stays_correct(self):
        examples = [
            BASE, '', '\n\n',
            'defina vacio principal() {\n entero edad = 2 :)\n}\n :)\n',
            'defina vacio principal() {\n entero edad = 2\n :)\n} :)\n',
            'defina vacio principal() {\n entero edad = (2 + 3) :)\n} :)\n',
            'defina vacio principal() {\n entero edad = (2 +\n   3) :)\n} :)\n',
            'defina vacio principal() {\n entero edad = 2 :)\n ^ comentario con :) y } ) :\n} :)\n',
            'defina vacio principal() {\n entero edad = 2 :)\n ^^ texto con } ) :) ^^\n} :)\n',
            'defina vacio principal() {\n entero edad = 2 :)\n si (edad == 2) {\n  entero x = 4 :)\n } :)\n} :)\n',
            'defina vacio principal() {\n entero edad = "texto con :) } (" :)\n} :)\n',
        ]
        for text in examples:
            with self.subTest(text=text):
                self.assertEqual(analyze(text), [])

    def test_edits_are_minimal(self):
        cases = [
            (BASE.replace('edad = 21 :)','edad = 21'), 2, 'APPEND_FIN_LINEA', BASE),
            (BASE.replace('edad = 21 :)','edad = 21 :'), 2, 'INSERT_RPAREN_AFTER_COLON', BASE),
            (BASE.replace('edad = 21 :)','edad = 21 )'), 2, 'INSERT_COLON_BEFORE_RPAREN', BASE),
            (BASE.replace('} :)\n','}\n'), 3, 'APPEND_FIN_LINEA', BASE),
            (BASE.replace('} :)\n','} :\n'), 3, 'INSERT_RPAREN_AFTER_COLON', BASE),
            (BASE.replace('} :)\n','} )\n'), 3, 'INSERT_COLON_BEFORE_RPAREN', BASE),
            (BASE.replace('} :)\n', ':)\n'), 3, 'INSERT_RBRACE_BEFORE_FIN', BASE),
            (BASE.replace('} :)\n', ':\n'), 3, 'REPLACE_ORPHAN_COLON_WITH_BLOCK', BASE),
            (BASE.replace('} :)\n', ')\n'), 3, 'REPLACE_ORPHAN_RPAREN_WITH_BLOCK', BASE),
            (BASE.replace('edad = 21 :)','edad = (2+3 :)'), 2, 'INSERT_RPAREN_BEFORE_FIN', BASE.replace('edad = 21','edad = (2+3)')),
            (BASE.replace('edad = 21 :)','edad = (2+3'), 2, 'APPEND_RPAREN_FIN', BASE.replace('edad = 21','edad = (2+3)')),
            (BASE.replace('principal() {','principal( {'), 1, 'INSERT_RPAREN', BASE.replace('principal() {','principal( ) {')),
        ]
        for bad, line, fix, expected in cases:
            with self.subTest(bad=bad):
                issues=analyze(bad)
                matches=[s for s in issues if s['line']==line and s['action']==fix]
                self.assertEqual(len(matches),1,issues)
                got=apply(bad,matches[0]);self.assertEqual(got,expected)
                self.assertFalse([x for x in analyze(got) if x['action']!='NO_FIX'],analyze(got))


    def test_many_contexts_and_indents(self):
        # 4 sangrías x 2 tipos de bloque x 7 mutaciones.
        for indent in ('', '  ', '    ', '        '):
            for nested in (False,True):
                opening=('defina vacio principal() {\n'
                         +(' si (1 == 1) {\n' if nested else ''))
                closing=(' } :)\n' if nested else '')
                pristine=(opening+f'{indent}entero n = 10 :)\n'
                          +closing+'} :)\n')
                edits=[('}', 'APPEND_FIN_LINEA'),
                       ('} :', 'INSERT_RPAREN_AFTER_COLON'),
                       ('} )', 'INSERT_COLON_BEFORE_RPAREN'),
                       (':)', 'INSERT_RBRACE_BEFORE_FIN'),
                       (':', 'REPLACE_ORPHAN_COLON_WITH_BLOCK'),
                       (')', 'REPLACE_ORPHAN_RPAREN_WITH_BLOCK')]
                for broken,action in edits:
                    bad=pristine.replace('} :)\n',broken+'\n',1 if nested else 1)
                    # La mutación se realiza sobre el primer cierre encontrado.
                    with self.subTest(indent=repr(indent),nested=nested,broken=broken):
                        candidates=[x for x in analyze(bad) if x['action']==action]
                        self.assertTrue(candidates,analyze(bad))
                        fixed=apply(bad,candidates[0])
                        self.assertEqual(fixed,pristine)
                        self.assertEqual(analyze(fixed),[])

    def test_single_char_fix_with_comment(self):
        originals=['defina vacio principal() {\n entero x = 1 :)\n} :) ^ cierre\n',
                   'defina vacio principal() {\n entero x = 1 :)\n} :) ^^ comentario ^^\n']
        for original in originals:
            for tail in ('} :','} )','}',')',':',':)'):
                broken=original.replace('} :)',tail)
                with self.subTest(tail=tail,original=original):
                    fixes=[x for x in analyze(broken) if x['action']!='NO_FIX']
                    self.assertEqual(len(fixes),1,fixes)
                    fixed=apply(broken,fixes[0])
                    self.assertEqual(fixed,original)
                    self.assertEqual(analyze(fixed),[])

    def test_no_replacement_of_ambiguous(self):
        for bad in [
            'defina vacio principal() {\n entero x = (\n} :)\n',
            'defina vacio principal() {\n entero x = 2 +\n} :)\n',
            'defina vacio principal() {\n entero x = 2 :)\n)\n} :)\n',
            'defina vacio principal() {\n entero x = 1 :)\n} :)\n)\n',
        ]:
            with self.subTest(bad=bad):
                self.assertFalse([s for s in analyze(bad) if s['action'] in ('REPLACE_ORPHAN_RPAREN_WITH_BLOCK','REPLACE_ORPHAN_COLON_WITH_BLOCK')],analyze(bad))

    def test_comment_insertion_position(self):
        bad='defina vacio principal() {\n entero x = 1 ^ comentario\n} :)\n'
        match=[x for x in analyze(bad) if x['line']==2 and x['action']=='APPEND_FIN_LINEA']
        self.assertEqual(len(match),1)
        expected='defina vacio principal() {\n entero x = 1 :) ^ comentario\n} :)\n'
        self.assertEqual(apply(bad,match[0]),expected)

    def test_unchanged_after_safe_fix(self):
        original='defina vacio principal() {\n  entero x = 1 :)\n} :)\n'
        for mutation in [original.replace('} :)',':)'), original.replace('} :)',')'), original.replace('} :)','} :')]:
            first=analyze(mutation)
            candidates=[s for s in first if s['action']!='NO_FIX']
            self.assertEqual(len(candidates),1,first)
            final=apply(mutation,candidates[0])
            self.assertEqual(final,original)
            self.assertEqual(analyze(final),[])

if __name__=='__main__':
    unittest.main(verbosity=2)
