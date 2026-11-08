#!/usr/bin/env python3
"""Pruebas de autocompletado de identificadores gobernado por LL(1)."""
import importlib.util
import unittest
from pathlib import Path
IDE=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ll1_symbols_app',IDE/'app.py')
app=importlib.util.module_from_spec(spec);spec.loader.exec_module(app)
class SymbolCompletion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        assert app.build_completion()
    def choices(self,context,prefix):
        return {r['word']:r['token'] for r in app.completions_from_c(prefix,context)}
    def test_local_variable(self):
        ctx='defina vacio principal() { entero resultado = 3 :)\n '
        self.assertEqual(self.choices(ctx,'res').get('resultado'),'entero')
    def test_global_variable(self):
        ctx='entero globales = 2 :)\ndefina vacio principal() {\n '
        self.assertIn('globales',self.choices(ctx,'glo'))
    def test_params(self):
        ctx='defina entero sumar(entero primero, vof segundo) {\n '
        self.assertEqual(self.choices(ctx,'pri').get('primero'),'parámetro')
        self.assertEqual(self.choices(ctx,'seg').get('segundo'),'parámetro')
    def test_functions(self):
        ctx='defina entero sumar(entero a) { devuelva a :) } :)\ndefina vacio principal() {\n '
        self.assertEqual(self.choices(ctx,'sum').get('sumar'),'función')
    def test_array_and_matrix(self):
        ctx='defina vacio principal() { entero lista datos[3] :) entero matriz mapas[2][2] :)\n '
        self.assertEqual(self.choices(ctx,'dat').get('datos'),'lista')
        self.assertEqual(self.choices(ctx,'map').get('mapas'),'matriz')
    def test_out_of_scope(self):
        ctx='defina vacio uno() { entero secreto = 3 :) } :)\ndefina vacio principal() {\n '
        self.assertNotIn('secreto',self.choices(ctx,'sec'))
    def test_sibling_block_out_of_scope(self):
        ctx=('defina vacio principal() { si(verdadero) { entero interno = 1 :) } :)\n '
             'entero inicio = 4 :)\n ')
        self.assertNotIn('interno',self.choices(ctx,'int'))
        self.assertIn('inicio',self.choices(ctx,'ini'))
    def test_for_variable_in_loop_and_not_after(self):
        a=('defina vacio principal() { para (entero indice = 0, indice < 4, indice = indice + 1) {\n ')
        self.assertIn('indice',self.choices(a,'ind'))
        b=a+'entero total = 0 :) } :)\n '
        self.assertNotIn('indice',self.choices(b,'ind'))
    def test_no_symbols_without_ll1_id_prediction(self):
        ctx='defina vacio principal() { entero resultado = 1 :)\n '
        self.assertEqual(self.choices('entero resultado = 1 :)\n ','res'),{})
        self.assertEqual(self.choices(ctx,'def'),{})
    def test_no_suggestions_on_invalid_prefix_context(self):
        self.assertEqual(self.choices('defina vacio principal() { entero x = 2 + :)\n ','res'),{})
    def test_comment_identifier_not_a_declaration(self):
        ctx='defina vacio principal() { ^ entero secreto = 1 :)\n entero resultado = 1 :)\n '
        self.assertNotIn('secreto',self.choices(ctx,'sec'))
    def test_no_future_variable(self):
        ctx='defina vacio principal() {\n '
        self.assertNotIn('resultado',self.choices(ctx,'res'))
if __name__=='__main__':unittest.main()
