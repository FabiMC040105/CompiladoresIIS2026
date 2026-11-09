#!/usr/bin/env python3
"""Pruebas nuevas del componente LL(1): gramática, M[A,a] y predicciones.
Ejecutar: python3 ide/tests/test_ll1.py
"""
import hashlib
import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path

IDE = Path(__file__).resolve().parents[1]
ROOT = IDE.parent
spec = importlib.util.spec_from_file_location('cdff_ll1_app', IDE / 'app.py')
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
TRACE = IDE / 'ide_ll1_trace'


class LL1Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        assert app.build_completion()
        assert app.build_tool(app.SYNTAX_SRC, app.SYNTAX_EXE)
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2',
                        '-o', str(TRACE), str(IDE / 'ide_ll1_trace.c'),
                        str(IDE / 'll1_predict.c')], check=True)

    def predict(self, context, prefix):
        return [x['word'] for x in app.completions_from_c(
            prefix, context)]

    def check(self, code):
        with tempfile.NamedTemporaryFile('w', suffix='.e', encoding='utf-8') as f:
            f.write(code)
            f.flush()
            return app.syntax_from_c(Path(f.name))

    def trace(self, code):
        with tempfile.NamedTemporaryFile('w', suffix='.e', encoding='utf-8') as f:
            f.write(code)
            f.flush()
            return subprocess.run([str(TRACE), f.name], capture_output=True,
                                  text=True, timeout=5, check=False)

    def test_static_table_regeneration_no_conflicts(self):
        table = IDE / 'll1_table.h'
        baseline = hashlib.sha256(table.read_bytes()).hexdigest()
        p = subprocess.run(['python3', str(IDE / 'tools/generate_ll1.py')],
                           capture_output=True, text=True, check=True)
        self.assertIn('Sin conflictos:', p.stdout)
        self.assertEqual(hashlib.sha256(table.read_bytes()).hexdigest(), baseline)
        self.assertIn('LL1_TABLE[N_COUNT][T_COUNT]', table.read_text())

    def test_real_example_corpus_accepted(self):
        files = list((ROOT / 'proyecto-1').rglob('*.e'))
        self.assertGreaterEqual(len(files), 10)
        for path in files:
            with self.subTest(path=path.name):
                p = subprocess.run([str(TRACE), str(path)], capture_output=True,
                                   text=True, timeout=5)
                self.assertEqual(p.returncode, 0, p.stdout[-1200:])
                self.assertIn('CADENA_COMPLETA\tACEPTAR', p.stdout)

    def test_empty_program_accepted(self):
        p = self.trace('')
        self.assertEqual(p.returncode, 0, p.stdout)

    def test_contextual_keywords_in_function_body(self):
        ctx = 'defina vacio principal() {\n  '
        self.assertEqual(self.predict(ctx, 'mie'), ['mientras'])
        self.assertEqual(self.predict(ctx, 'en'), ['entero'])
        self.assertEqual(self.predict(ctx, 'def'), [])

    def test_context_after_assignment_is_expression(self):
        ctx = 'defina vacio principal() {\n entero x = '
        self.assertEqual(self.predict(ctx, 'fal'), ['falso'])
        self.assertEqual(self.predict(ctx, 'mie'), [])

    def test_if_continuations(self):
        ctx = 'defina vacio principal() { si (verdadero) { rompa :) } '
        self.assertEqual(self.predict(ctx, 'sin'), ['sino', 'sinosi'])
        self.assertEqual(self.predict(ctx, 'ent'), [])

    def test_global_context_only_top_level_candidates(self):
        self.assertEqual(self.predict('', '#tr'), ['#traigase'])
        self.assertEqual(self.predict('', 'mie'), [])
        self.assertEqual(self.predict('defina ', 'va'), ['vacio'])

    def test_complete_grammar_with_list_matrix_for_and_else(self):
        code = '''defina vacio principal() {
 entero x = 2 :)
 entero lista datos[3] = [1,2,3] :)
 entero matriz m[2][2] = [[1,2],[3,4]] :)
 para (entero i = 0, i < 3, i = i + 1) {
  si (i == 2) { rompa :) }
  sinosi (i == 1) { x = 4 :) }
  sino { x = 5 :) } :)
 } :)
} :)
'''
        p = self.trace(code)
        self.assertEqual(p.returncode, 0, p.stdout[-900:])

    def test_trace_proves_real_table_lookup(self):
        p = self.trace('defina vacio principal() {} :)')
        self.assertIn('M[PROGRAM,DEFINA] = P0', p.stdout)
        self.assertIn('M[BLOCK,LC]', p.stdout)
        self.assertIn('COINCIDIR, avanzar', p.stdout)
        self.assertEqual(p.returncode, 0)

    def test_invalid_syntax_not_accepted(self):
        p = self.trace('defina vacio principal(){ entero x = + :) } :)')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('ERROR: M[A,a] vacía', p.stdout)
        self.assertIn('TOKENS_ESPERADOS', p.stdout)

    def test_no_false_terminator_before_else(self):
        code='''defina vacio principal() {
 si (verdadero) {
  entero x = 1 :)
 }
 sino { entero x = 2 :) } :)
} :)
'''
        self.assertEqual(self.check(code), [])
        self.assertEqual(self.trace(code).returncode, 0)

    def test_terminator_only_after_ll1_approved_statement(self):
        good='defina vacio principal() {\n entero x = 12\n} :)\n'
        self.assertTrue(any(x['code']=='LL1_INSERT' and ':)' in x['replacement'] for x in self.check(good)))
        incomplete='defina vacio principal() {\n entero x =\n} :)\n'
        result=self.check(incomplete)
        self.assertFalse(any(x['code']=='LL1_INSERT' and ':)' in x['replacement'] for x in result))
        self.assertTrue(any(x['code']=='NO_FIX' and 'LL(1): se esperaba' in x['message'] for x in result))
        invalid='defina vacio principal() {\n entero\n} :)\n'
        self.assertFalse(any(x['code']=='LL1_INSERT' and ':)' in x['replacement'] for x in self.check(invalid)))

    def test_comments_are_ignored_not_used_as_predictions(self):
        a='defina vacio principal() { ^^ texto falsos :) ^^\n '
        self.assertEqual(self.predict(a, 'mie'), ['mientras'])


if __name__=='__main__':
    unittest.main(verbosity=2)
