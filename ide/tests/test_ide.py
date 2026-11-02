#!/usr/bin/env python3
"""Pruebas automáticas del IDE sin alterar el compilador del Proyecto 1.
Ejecutar desde la raíz del repo: python3 ide/tests/test_ide.py
No requiere pytest ni acceso a internet.
"""
import importlib.util
import json
import shutil
import subprocess
import tempfile
import threading
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.request import Request, urlopen

IDE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('cdff_app', IDE / 'app.py')
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

A = '''defina vacio principal() {
  entero edad = 21 :)
  entero copia = edad + 3 :)
  copia = copia * 2 :)
} :)
'''
B = '''defina vacio principal() {
  entero edad=21 :)
  ^ comentario ignorado
  entero copia=edad+3 :)
  copia = copia * 2 :)
} :)
'''
C = '''defina vacio principal() {
  entero valor = 30 :)
  entero nuevo = valor + 6 :)
  nuevo = nuevo * 5 :)
} :)
'''

class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.src = Path(cls.tmp.name) / 'proyecto-1' / 'src'
        cls.src.mkdir(parents=True)
        (cls.src / 'lexer.l').write_text('"defina"  { return DEFINA; }\n"mientras" { return MIENTRAS; }\n"entero" { return ENTERO; }\n"vacio" { return VACIO; }\n"principal" { return PRINCIPAL; }\n')
        (cls.src / 'parser.y').write_text('/* archivo de prueba */')
        app.WORKSPACE = Path(cls.tmp.name) / 'workspace'
        app.WORKSPACE.mkdir()
        for src,exe in [(app.SYNTAX_SRC,app.SYNTAX_EXE),(app.CLONES_SRC,app.CLONES_EXE),(app.COMPLETION_SRC,app.COMPLETION_EXE)]:
            if src == app.COMPLETION_SRC:
                assert app.build_completion()
            else: assert app.build_tool(src,exe)
        cls.server = ThreadingHTTPServer(('127.0.0.1',0),app.handler_factory(cls.src))
        cls.port=cls.server.server_address[1]
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown();cls.server.server_close();cls.thread.join(timeout=2)
        cls.tmp.cleanup()

    @classmethod
    def post(cls,path,data):
        body=json.dumps(data).encode()
        req=Request(f'http://127.0.0.1:{cls.port}{path}',body,{'Content-Type':'application/json'})
        with urlopen(req,timeout=5) as r:return json.load(r)

    def test_new_editor_starts_blank(self):
        html=(IDE/'index.html').read_text()
        self.assertIn("code.value='';code.defaultValue='';",html)
        self.assertIn('Iteración 1 · V5 · QA',html)

    def test_complete_from_c(self):
        data=self.post('/api/complete',{'prefix':'mien'})
        self.assertIn('mientras',[x['word'] for x in data['items']])

    def test_terminator_and_parenthesis(self):
        bad=A.replace('entero edad = 21 :)','entero edad = 21')
        issues=self.post('/api/check',{'name':'prueba.e','content':bad})['suggestions']
        self.assertTrue(any(x['code']=='APPEND_FIN_LINEA' and x['line']==2 for x in issues))
        bad=A.replace('principal() {','principal( {')
        issues=self.post('/api/check',{'name':'prueba.e','content':bad})['suggestions']
        self.assertTrue(any(x['code']=='INSERT_RPAREN' for x in issues))


    def test_missing_end_after_brace(self):
        text="""defina vacio principal() {
 si (1 == 1) {
  entero x = 2 :)
 }
 entero y = 3 :)
} :)
"""
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertTrue(any(x['code']=='APPEND_FIN_LINEA' and x['line']==4
                            and "llave" in x['message'] for x in issues))

    def test_terminator_on_next_line_does_not_warn(self):
        text='defina vacio principal() {\n entero edad = 2 :)\n}\n :)\n'
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertFalse(issues)

    def test_missing_paren_before_terminator(self):
        text=A.replace('entero edad = 21 :)','entero edad = (2 + 3 :)')
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertTrue(any(x['code']=='INSERT_RPAREN_BEFORE_FIN' and x['line']==2
                            for x in issues))
        self.assertFalse(any(x['code']=='APPEND_BLOCK' for x in issues))

    def test_missing_paren_and_terminator(self):
        text=A.replace('entero edad = 21 :)','entero edad = (2 + 3')
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertTrue(any(x['code']=='APPEND_RPAREN_FIN' and x['line']==2
                            for x in issues))

    def test_trailing_comment_keeps_correction_position(self):
        text='defina vacio principal() {\n entero edad = 21 ^ comentario\n} ^ comentario\n'
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertEqual(sorted(x['line'] for x in issues),[2,3])
        self.assertEqual([x['column'] for x in issues if x['line']==3],[1])

    def test_real_syntax_examples_do_not_generate_false_alerts(self):
        text=A.replace('entero edad = 21 :)','entero edad = (21 + 3) :)')
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertFalse(issues)

    def test_missing_brace(self):
        issues=self.post('/api/check',{'name':'prueba.e','content':A.rsplit('} :)',1)[0]})['suggestions']
        self.assertTrue(any(x['code']=='APPEND_BLOCK' for x in issues))

    def test_orphan_terminator_when_closing_brace_deleted(self):
        original = '''defina entero factorial(entero n) {
    devuelva n :)
} :)

defina vacio principal() {
    entero n = 5 :)
    entero resultado = factorial(n) :)
} :)
'''
        missing = original.replace('    entero resultado = factorial(n) :)\n} :)',
                                   '    entero resultado = factorial(n) :)\n:)')
        issues=self.post('/api/check',{'name':'factorial.e','content':missing})['suggestions']
        self.assertTrue(any(x['code']=='INSERT_RBRACE_BEFORE_FIN' and x['line']==8
                            and x['column']==0 for x in issues),issues)
        self.assertFalse(any(x['code']=='APPEND_BLOCK' for x in issues),issues)

    def test_orphan_terminator_with_indent_and_comment(self):
        text='defina vacio principal() {\n  entero x = 1 :)\n  :) ^ comentario\n'
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertTrue(any(x['code']=='INSERT_RBRACE_BEFORE_FIN' and x['line']==3
                            and x['column']==2 for x in issues),issues)

    def test_standalone_terminator_after_closed_brace_is_valid(self):
        text='defina vacio principal() {\n entero x = 1 :)\n}\n :)\n'
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertFalse(issues,issues)

    def test_standalone_terminator_after_unterminated_statement_is_valid(self):
        text='defina vacio principal() {\n entero x = 1\n :)\n} :)\n'
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertFalse(issues,issues)

    def test_old_auto_fix_leaves_redundant_standalone_terminator(self):
        # Estado de la captura despues de aplicar la antigua correccion.
        text='defina vacio principal() {\n entero n = 5 :)\n :)\n} :)\n'
        issues=self.post('/api/check',{'name':'factorial.e','content':text})['suggestions']
        self.assertTrue(any(x['code']=='REMOVE_ORPHAN_FIN_LINEA' and x['line']==3
                            for x in issues),issues)
        self.assertFalse(any(x['code']=='APPEND_BLOCK' for x in issues),issues)

    def test_orphan_paren_if_brace_and_colon_deleted(self):
        original = """defina entero factorial(entero n) {
    devuelva n :)
} :)

defina vacio principal() {
    entero n = 5 :)
    entero resultado = factorial(n) :)
} :)
"""
        broken=original.replace('    entero resultado = factorial(n) :)\n} :)',
                                '    entero resultado = factorial(n) :)\n  )')
        issues=self.post('/api/check',{'name':'factorial.e','content':broken})['suggestions']
        self.assertEqual(len(issues),1,issues)
        self.assertEqual(issues[0]['code'],'REPLACE_ORPHAN_RPAREN_WITH_BLOCK')
        self.assertEqual(issues[0]['line'],8)
        self.assertEqual(issues[0]['column'],2)

    def test_missing_brace_and_colon_nested_block(self):
        text="""defina vacio principal() {
    si (1 == 1) {
        entero x = 2 :)
    )
} :)
"""
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertEqual(len(issues),1,issues)
        self.assertEqual(issues[0]['code'],'REPLACE_ORPHAN_RPAREN_WITH_BLOCK')
        self.assertEqual(issues[0]['line'],4)

    def test_legitimate_multiline_parenthesis_not_mistaken_for_brace(self):
        # Un ')' en línea independiente puede cerrar un '(' de otra línea.
        text="""defina vacio principal() {
    entero valor = (1 + 2
    ) :)
} :)
"""
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertFalse(any(x['code']=='REPLACE_ORPHAN_RPAREN_WITH_BLOCK' for x in issues),issues)

    def test_orphan_paren_without_unclosed_brace_is_not_autofixed(self):
        text="""defina vacio principal() {
    entero valor = 1 :)
} :)
)
"""
        issues=self.post('/api/check',{'name':'prueba.e','content':text})['suggestions']
        self.assertFalse(any(x['code']=='REPLACE_ORPHAN_RPAREN_WITH_BLOCK' for x in issues),issues)
        self.assertTrue(any(x['code']=='NO_FIX' for x in issues),issues)

    def test_valid_has_no_suggestions(self):
        issues=self.post('/api/check',{'name':'prueba.e','content':A})['suggestions']
        self.assertFalse(issues)

    def test_unicode_column_for_textarea(self):
        # C cuenta bytes UTF-8; el frontend indexa UTF-16.
        line=' entero titulo = "piñata 🎈"'
        source='defina vacio principal() {\n'+line+'\n} :)\n'
        issues=self.post('/api/check',{'name':'unicode.e','content':source})['suggestions']
        match=[x for x in issues if x['line']==2 and x['code']=='APPEND_FIN_LINEA']
        self.assertEqual(len(match),1,issues)
        self.assertEqual(match[0]['column'],len(line.encode('utf-16-le'))//2)

    def test_clones_type_i_and_ii(self):
        for name,content in [('copia_a.e',A),('copia_b.e',B),('copia_c.e',C)]:
            self.post('/api/save',{'name':name,'content':content})
        result=self.post('/api/clones',{'include_examples':False})
        self.assertGreaterEqual(result['files_scanned'],3)
        types={x['type'] for x in result['clones']}
        self.assertIn('I',types);self.assertIn('II',types)

    def test_no_clones_for_unrelated_small_files(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder);(p/'a.e').write_text('entero a = 3 :)\n')
            (p/'b.e').write_text('entero b = 9 :)\n')
            result=subprocess.run([str(app.CLONES_EXE),str(self.src/'lexer.l'),'A',str(p/'a.e'),'B',str(p/'b.e')],capture_output=True,text=True)
            self.assertEqual(result.returncode,0)
            self.assertFalse(result.stdout.strip())

if __name__=='__main__':unittest.main(verbosity=2)
