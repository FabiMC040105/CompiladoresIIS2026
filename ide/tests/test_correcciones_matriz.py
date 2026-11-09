#!/usr/bin/env python3
"""Regresión LL(1) pura: toda propuesta debe nacer de FIRST(pila)
   y mejorar/aceptar una cadena al analizarla de nuevo con M[A,a]."""
import subprocess
import tempfile
import unittest
from pathlib import Path

IDE = Path(__file__).resolve().parents[1]
EXE = IDE/'ide_syntax'

def check(source):
    with tempfile.NamedTemporaryFile('w',encoding='utf-8',suffix='.e') as f:
        f.write(source);f.flush()
        p=subprocess.run([str(EXE),f.name],capture_output=True,text=True,check=True)
    found=[]
    for line in p.stdout.splitlines():
        a=line.split('\t',5)
        assert len(a)==6, line
        found.append(dict(line=int(a[0]),col=int(a[1]),action=a[2],old=a[3],new=a[4],message=a[5]))
    return found

def apply(source,fix):
    lines=source.split('\n');i=fix['line']-1
    at=fix['col'];old=fix['old'];new=fix['new']
    assert lines[i][at:at+len(old)]==old
    lines[i]=lines[i][:at]+new+lines[i][at+len(old):]
    return '\n'.join(lines)

BASE='defina vacio principal() {\n entero x = 1 :)\n} :)\n'

class LL1Correcciones(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O2',
                        '-o',str(EXE),str(IDE/'ide_syntax.c'),str(IDE/'ll1_predict.c')],check=True)

    def test_valid_code_has_no_corrections(self):
        valid=[BASE, '', '  \n',
          'defina vacio principal() {\n entero x = 1\n + 2 :)\n} :)',
          'defina vacio principal() {\n entero x = (2 +\n 3) :)\n} :)',
          'defina vacio principal() {\n si (verdadero) { entero x = 1 :) }\n sino { entero x = 2 :) } :)\n} :)',
          'defina vacio principal() {\n entero x = 1 :)\n} \n :)',
          'defina vacio principal() {\n ^^ comentario con :) } ^^ entero x=1 :)\n} :)',
          'defina vacio principal() {\n entero x=1 :) ^ comentario\n} :)'
        ]
        for src in valid:
            with self.subTest(src=src): self.assertEqual(check(src),[])

    def test_all_edits_reach_valid_program(self):
        broken=[
            BASE.replace('x = 1 :)','x = 1'),
            BASE.replace('} :)\n','}\n'),
            BASE.replace('} :)\n','} :\n'),
            BASE.replace('} :)\n','} )\n'),
            BASE.replace('} :)\n',':)\n'),
            BASE.replace('} :)\n',')\n'),
            BASE.replace('entero x = 1 :)','entero x = (2 + 3 :)'),
            BASE.replace('entero x = 1 :)','entero x = (2 + 3'),
            BASE.replace('principal() {','principal( {'),
            BASE.rsplit('} :)',1)[0],
        ]
        for source in broken:
            with self.subTest(src=source):
                for _ in range(5):
                    issues=check(source)
                    if not issues: break
                    fixes=[r for r in issues if r['action']!='NO_FIX']
                    self.assertTrue(fixes,issues)
                    for fix in fixes:self.assertIn(fix['action'],('LL1_INSERT','LL1_REPLACE'))
                    source=apply(source,fixes[0])
                self.assertEqual(check(source),[],source)

    def test_multiple_missing_terminators(self):
        source='defina vacio principal() {\n entero x = 1\n entero y = 2\n} :)\n'
        issues=check(source)
        self.assertEqual([i['line'] for i in issues if i['action']=='LL1_INSERT'],[2,3])

    def test_multiline_expression_is_not_cut(self):
        source='defina vacio principal() {\n entero x = 1\n + 2 :)\n} :)'
        self.assertEqual(check(source),[])

    def test_incomplete_expression_reports_first_set_not_closure(self):
        src='defina vacio principal() {\n entero x = 2 +\n} :)'
        issues=check(src)
        self.assertEqual(len(issues),1)
        self.assertEqual(issues[0]['action'],'NO_FIX')
        self.assertIn('identificador',issues[0]['message'])

    def test_incompatible_string_no_fake_terminator(self):
        src='defina vacio principal() {\n entero x = "hola" :)\n} :)'
        issues=check(src)
        self.assertTrue(any(i['action']=='NO_FIX' for i in issues))
        self.assertFalse(any(i['action'].startswith('LL1_') for i in issues))

    def test_malformed_expression_not_fixed_with_fin(self):
        src='defina vacio principal() {\n entero x = + 2 :)\n} :)'
        self.assertTrue(all(i['action']=='NO_FIX' for i in check(src)))

    def test_comment_position(self):
        src='defina vacio principal() {\n entero x = 1 ^ comentario\n} :)'
        fixes=[i for i in check(src) if i['action']=='LL1_INSERT']
        self.assertEqual(fixes[0]['line'],2)
        self.assertEqual(fixes[0]['col'],len(' entero x = 1'))
        self.assertIn(':) ^ comentario',apply(src,fixes[0]))

    def test_no_legacy_decisions_in_implementation(self):
        text=(IDE/'ide_syntax.c').read_text()
        for old in ('is_header(', 'statement(', 'ends_operator(', 'following_is_if_continuation(',
                    'll1_approves(', 'candidate_count', 'previous_ended('):
            self.assertNotIn(old,text)
        self.assertIn('ll1_analyze(',text)
        self.assertIn('expected_at_error(',text)

if __name__=='__main__':unittest.main(verbosity=2)
