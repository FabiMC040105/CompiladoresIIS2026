#!/usr/bin/env python3

"""
Pruebas de Persona 4 para el detector de clones del IDE.

Se valida:
- Clones Tipo I.
- Clones Tipo II.
- Renombramiento consistente.
- Prevención de falsos positivos.
- Umbral mínimo de líneas/tokens.
- Comparación entre archivos.
"""

import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
IDE = ROOT / "ide"

CLONES_SRC = IDE / "ide_clones.c"
LEXER = ROOT / "proyecto-1" / "src" / "lexer.l"


class Persona4Clones(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(
            prefix="cdff-persona4-clones-"
        )

        cls.exe = Path(cls.tmp.name) / "ide_clones"

        proc = subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-O2",
                "-o",
                str(cls.exe),
                str(CLONES_SRC),
            ],
            capture_output=True,
            text=True,
            check=False,
        )

        if proc.returncode != 0:
            raise RuntimeError(
                "No se pudo compilar ide_clones.c:\n"
                + proc.stderr
            )

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def detectar(self, archivos):
        """
        archivos:
        {
            "A": "codigo...",
            "B": "codigo..."
        }
        """

        with tempfile.TemporaryDirectory(
            prefix="cdff-clone-case-"
        ) as tmp:
            tmp = Path(tmp)

            cmd = [
                str(self.exe),
                str(LEXER),
            ]

            for etiqueta, contenido in archivos.items():
                ruta = tmp / f"{etiqueta}.e"
                ruta.write_text(
                    contenido,
                    encoding="utf-8"
                )

                cmd.extend([
                    etiqueta,
                    str(ruta),
                ])

            proc = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                proc.returncode,
                0,
                proc.stderr
            )

            resultados = []

            for linea in proc.stdout.splitlines():
                partes = linea.split("\t")

                if len(partes) != 7:
                    continue

                resultados.append({
                    "tipo": partes[0],
                    "archivo_a": partes[1],
                    "inicio_a": int(partes[2]),
                    "fin_a": int(partes[3]),
                    "archivo_b": partes[4],
                    "inicio_b": int(partes[5]),
                    "fin_b": int(partes[6]),
                })

            return resultados

    def test_tipo_I_ignora_espacios_y_comentarios(self):
        original = """\
defina vacio principal() {
 entero x = 1 :)
 entero y = 2 :)
 x = x + y :)
} :)
"""

        copia = """\
defina   vacio principal() { ^ comentario
 entero x=1 :)
 entero y = 2 :) ^ comentario adicional
 x=x+y :)
} :)
"""

        clones = self.detectar({
            "A": original,
            "B": copia,
        })

        self.assertTrue(
            any(c["tipo"] == "I" for c in clones),
            clones,
        )

    def test_tipo_II_renombra_identificadores_y_literales(self):
        original = """\
defina vacio principal() {
 entero x = 1 :)
 entero y = 2 :)
 x = x + y :)
} :)
"""

        renombrado = """\
defina vacio principal() {
 entero valor = 10 :)
 entero total = 20 :)
 valor = valor + total :)
} :)
"""

        clones = self.detectar({
            "A": original,
            "B": renombrado,
        })

        self.assertTrue(
            any(c["tipo"] == "II" for c in clones),
            clones,
        )

    def test_tipo_II_exige_renombramiento_consistente(self):
        original = """\
defina vacio principal() {
 entero x = 1 :)
 entero y = 2 :)
 x = x + y :)
} :)
"""

        inconsistente = """\
defina vacio principal() {
 entero a = 1 :)
 entero b = 2 :)
 a = b + b :)
} :)
"""

        clones = self.detectar({
            "A": original,
            "B": inconsistente,
        })

        self.assertEqual(
            clones,
            [],
            "Un renombramiento inconsistente no debe "
            "clasificarse como clon Tipo II.",
        )

    def test_operador_diferente_no_es_clon(self):
        suma = """\
defina vacio principal() {
 entero x = 1 :)
 entero y = 2 :)
 x = x + y :)
} :)
"""

        resta = """\
defina vacio principal() {
 entero a = 1 :)
 entero b = 2 :)
 a = a - b :)
} :)
"""

        clones = self.detectar({
            "SUMA": suma,
            "RESTA": resta,
        })

        self.assertEqual(
            clones,
            [],
            "Cambiar la operación modifica la estructura "
            "del código y no debe producir un clon.",
        )

    def test_fragmento_demasiado_pequeno_no_se_reporta(self):
        corto_a = """\
entero x = 1 :)
x = x + 1 :)
"""

        corto_b = """\
entero x = 1 :)
x = x + 1 :)
"""

        clones = self.detectar({
            "A": corto_a,
            "B": corto_b,
        })

        self.assertEqual(
            clones,
            [],
            "El detector exige al menos 3 líneas y 18 tokens.",
        )

    def test_reporta_rango_de_lineas(self):
        codigo = """\
defina vacio principal() {
 entero x = 1 :)
 entero y = 2 :)
 x = x + y :)
} :)
"""

        clones = self.detectar({
            "A": codigo,
            "B": codigo,
        })

        tipo_i = [
            c for c in clones
            if c["tipo"] == "I"
        ]

        self.assertTrue(tipo_i, clones)

        primero = tipo_i[0]

        self.assertEqual(
            primero["inicio_a"],
            1
        )

        self.assertEqual(
            primero["fin_a"],
            5
        )

        self.assertEqual(
            primero["inicio_b"],
            1
        )

        self.assertEqual(
            primero["fin_b"],
            5
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
