#!/usr/bin/env python3
"""Pruebas HTTP de autocompletado y revisión LL(1), más preservación de clones."""
import importlib.util
import json
import subprocess
import tempfile
import threading
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request,urlopen

IDE=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cdff_app',IDE/'app.py')
app=importlib.util.module_from_spec(spec);spec.loader.exec_module(app)
CODE='defina vacio principal() {\n entero x = 1 :)\n} :)\n'

class IDELL1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory()
        cls.src=Path(cls.temp.name)/'proyecto-1'/'src';cls.src.mkdir(parents=True)
        (cls.src/'lexer.l').write_text('"defina" { return DEFINA; }\n')
        (cls.src/'parser.y').write_text('/* gramática fija */')
        app.WORKSPACE=Path(cls.temp.name)/'workspace';app.WORKSPACE.mkdir()
        assert app.build_completion()
        assert app.build_tool(app.SYNTAX_SRC,app.SYNTAX_EXE)
        assert app.build_tool(app.CLONES_SRC,app.CLONES_EXE)
        cls.server=ThreadingHTTPServer(('127.0.0.1',0),app.handler_factory(cls.src))
        cls.port=cls.server.server_address[1]
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True)
        cls.thread.start()
    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown();cls.server.server_close();cls.thread.join(timeout=2)
        cls.temp.cleanup()
    @classmethod
    def post(cls,route,body):
        req=Request(f'http://127.0.0.1:{cls.port}{route}',
            json.dumps(body).encode(),{'Content-Type':'application/json'})
        with urlopen(req,timeout=5) as r:return json.load(r)
    def complete(self,prefix,context):
        return [x['word'] for x in self.post('/api/complete',{'prefix':prefix,'context':context})['items']]
    def check(self,code):
        return self.post('/api/check',{'name':'test.e','content':code})['suggestions']
    def test_shell_remains(self):
        html=(IDE/'index.html').read_text()
        self.assertIn('code.value = "";',html)
        self.assertIn('Sugerencias en vivo',html)
        self.assertIn('context: code.value.slice(0, word.start)',html)
    def test_missing_context_does_not_invoke_old_fallback(self):
        req=Request(f'http://127.0.0.1:{self.port}/api/complete',
                    json.dumps({'prefix':'mie'}).encode(),{'Content-Type':'application/json'})
        with self.assertRaises(HTTPError) as err:urlopen(req,timeout=5)
        self.assertEqual(err.exception.code,400)
    def test_contextual_completion(self):
        body='defina vacio principal() {\n '
        self.assertEqual(self.complete('mie',body),['mientras'])
        self.assertEqual(self.complete('en',body),['entero'])
        self.assertEqual(self.complete('mie',''),[])
        self.assertEqual(self.complete('fal',body+'entero x = '),['falso'])
        self.assertEqual(self.complete('mie',body+'entero x = '),[])
        self.assertEqual(self.complete('def','defina vacio principal() {'),[])
    def test_if_continuation_is_grammatical(self):
        source='defina vacio principal() { si (verdadero) { rompa :) } '
        self.assertEqual(self.complete('sin',source),['sino','sinosi'])
    def test_ll1_syntax_api(self):
        self.assertEqual(self.check(CODE),[])
        issues=self.check(CODE.replace('x = 1 :)','x = 1'))
        self.assertEqual(issues[0]['code'],'LL1_INSERT')
        self.assertEqual(issues[0]['line'],2)
        self.assertIn(':)',issues[0]['replacement'])
    def test_ll1_expected_when_expression_incomplete(self):
        issues=self.check(CODE.replace('x = 1 :)','x = 1 +'))
        self.assertEqual([i['code'] for i in issues],['NO_FIX'])
        self.assertIn('identificador',issues[0]['message'])
    def test_valid_multiline_unchanged(self):
        self.assertEqual(self.check(CODE.replace('x = 1 :)','x = 1\n + 2 :)')),[])
    def test_unicode_byte_to_utf16_column(self):
        line=' ^^ piñata 🎈 ^^ entero x = 1'
        issues=self.check('defina vacio principal() {\n'+line+'\n} :)')
        match=[i for i in issues if i['line']==2 and i['code']=='LL1_INSERT']
        self.assertEqual(len(match),1,issues)
        self.assertEqual(match[0]['column'],len(line.encode('utf-16-le'))//2)
    def test_clones_unchanged(self):
        texts={
        'a.e':'defina vacio principal() { entero x=1 :) entero y=2 :) x=x+y :) } :)',
        'b.e':'defina vacio principal() { entero x=1 :) entero y=2 :) x=x+y :) } :)',
        }
        for name,content in texts.items():
            self.post('/api/save',{'name':name,'content':content})
        result=self.post('/api/clones',{'include_examples':False})
        self.assertGreaterEqual(result['files_scanned'],2)

if __name__=='__main__':unittest.main(verbosity=2)
