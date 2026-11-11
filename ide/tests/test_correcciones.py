#!/usr/bin/env python3

"""
Pruebas específicas para las correcciones
automáticas guiadas por LL(1).

Se valida:
- Falta de :)
- Falta de )
- Falta de }
- Múltiples errores corregibles
- Expresiones ambiguas -> NO_FIX
- Código válido no se modifica
- No se inventan identificadores
- No se duplican símbolos
"""

import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
IDE = ROOT / "ide"

SYNTAX_SRC = IDE / "ide_syntax.c"
LL1_SRC = IDE / "ll1_predict.c"


class CorreccionesAutomaticas(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(
            prefix="cdff-correcciones-"
        )

        cls.exe = Path(cls.tmp.name) / "ide_syntax"

        proc = subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-O2",
                "-o",
                str(cls.exe),
                str(SYNTAX_SRC),
                str(LL1_SRC),
            ],
            capture_output=True,
            text=True,
            check=False,
        )

        if proc.returncode != 0:
            raise RuntimeError(
                "No se pudo compilar ide_syntax.c:\n"
                + proc.stderr
            )

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def check(self, source):
        with tempfile.NamedTemporaryFile(
            mode="w",
            suffix=".e",
            encoding="utf-8",
            delete=False,
        ) as archivo:
            archivo.write(source)
            ruta = Path(archivo.name)

        try:
            proc = subprocess.run(
                [str(self.exe), str(ruta)],
                capture_output=True,
                text=True,
                check=False,
            )
        finally:
            ruta.unlink(missing_ok=True)

        self.assertEqual(
            proc.returncode,
            0,
            proc.stderr
        )

        issues = []

        for line in proc.stdout.splitlines():
            parts = line.split("\t")

            if len(parts) < 6:
                continue

            issues.append({
                "line": int(parts[0]),
                "column": int(parts[1]),
                "code": parts[2],
                "expected": parts[3],
                "replacement": parts[4],
                "message": parts[5],
            })

        return issues

    def test_falta_terminador(self):
        source = """\
defina vacio principal() {
 entero x = 1
} :)
"""

        issues = self.check(source)

        self.assertTrue(
            any(
                i["code"] == "LL1_INSERT"
                and ":)" in i["replacement"]
                for i in issues
            ),
            issues,
        )

    def test_falta_parentesis(self):
        source = """\
defina vacio principal() {
 entero x = (2 + 3 :)
} :)
"""

        issues = self.check(source)

        self.assertTrue(
            any(
                i["code"] in (
                    "LL1_INSERT",
                    "LL1_REPLACE",
                )
                and ")" in i["replacement"]
                for i in issues
            ),
            issues,
        )

    def test_falta_llave(self):
        source = """\
defina vacio principal() {
 entero x = 1 :)
:)
"""

        issues = self.check(source)

        self.assertTrue(
            any(
                i["code"] in (
                    "LL1_INSERT",
                    "LL1_REPLACE",
                )
                and "}" in i["replacement"]
                for i in issues
            ),
            issues,
        )

    def test_varios_errores_corregibles(self):
        source = """\
defina vacio principal() {
 entero x = 1
 entero y = 2
} :)
"""

        issues = self.check(source)

        fixes = [
            i for i in issues
            if i["code"] in (
                "LL1_INSERT",
                "LL1_REPLACE",
            )
        ]

        self.assertGreaterEqual(
            len(fixes),
            2,
            issues,
        )

    def test_expresion_incompleta_no_inventa_operando(self):
        source = """\
defina vacio principal() {
 entero x = 1 + :)
} :)
"""

        issues = self.check(source)

        self.assertTrue(issues)

        self.assertTrue(
            all(
                i["code"] == "NO_FIX"
                for i in issues
            ),
            issues,
        )

    def test_no_inventa_identificador(self):
        source = """\
defina vacio principal() {
 entero = 1 :)
} :)
"""

        issues = self.check(source)

        for issue in issues:
            if issue["code"] != "NO_FIX":
                self.assertNotRegex(
                    issue["replacement"],
                    r"[a-zA-Z_][a-zA-Z_0-9]*",
                    "El corrector no debe inventar "
                    "identificadores."
                )

    def test_codigo_valido_no_se_modifica(self):
        source = """\
defina vacio principal() {
 entero x = 1 :)
 entero y = 2 :)
 x = x + y :)
} :)
"""

        self.assertEqual(
            self.check(source),
            []
        )

    def test_no_duplica_terminador(self):
        source = """\
defina vacio principal() {
 entero x = 1 :)
} :)
"""

        issues = self.check(source)

        self.assertFalse(
            any(
                i["code"] != "NO_FIX"
                and ":)" in i["replacement"]
                for i in issues
            ),
            issues,
        )

    def test_multilinea_valido_no_se_corta(self):
        source = """\
defina vacio principal() {
 entero x = 1
 + 2 :)
} :)
"""

        self.assertEqual(
            self.check(source),
            []
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
