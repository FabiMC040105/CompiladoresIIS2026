#!/usr/bin/env python3

"""
Servidor local del entorno de desarrollo CDFF.

Este módulo permite la comunicación entre la interfaz web y
las herramientas del compilador. Se encarga de administrar
los archivos del usuario, ejecutar la compilación y enviar
los resultados a la interfaz.

También integra las herramientas auxiliares escritas en C
para el autocompletado, la revisión sintáctica y la
detección de código duplicado.

Se utiliza únicamente la biblioteca estándar de Python.
"""

from __future__ import annotations

import argparse
import tempfile
import json
import re
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse


# CONFIGURACIÓN GENERAL

"""
Se definen las rutas utilizadas por el servidor y el
directorio donde se almacenan los archivos del usuario.
También se establece el tamaño máximo permitido para
evitar procesar archivos demasiado grandes.
"""

IDE_DIR = Path(__file__).resolve().parent
WORKSPACE = IDE_DIR / "workspace"
WORKSPACE.mkdir(parents=True, exist_ok=True)
MAX_BYTES = 1024 * 1024


# Marcadores que identifican la representacion intermedia enviada por el compilador C

IR_BEGIN = "=== CDFF_IR_BEGIN ==="
IR_END = "=== CDFF_IR_END ==="


def extract_ir(output: str) -> tuple[str, str]:
    """
    Separa la representacion intermedia de la salida general del compilador
    Retorna:
        output_clean: mensajes normales del compilador
        ir_text: codigo de tres direcciones
    """

    start = output.find(IR_BEGIN)

    if start == -1:
        return output, ""

    end = output.find(IR_END, start + len(IR_BEGIN))

    if end == -1:
        return output, ""

    ir_text = output[start + len(IR_BEGIN):end].strip()

    output_clean = (
        output[:start] +
        output[end + len(IR_END):]
    ).strip()

    return output_clean, ir_text


# Verifica que el nombre del archivo sea válido y tenga extensión .e

def clean_name(value: object) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"[a-zA-Z0-9_\-]+\.e", value):
        raise ValueError("Use un nombre sencillo terminado en .e (letras, números, _ o -).")
    return value

"""
Lee los datos JSON enviados por la interfaz.
Comprueba el tamaño de la solicitud y verifica que
la información recibida tenga el formato esperado.
"""

def read_json(handler: BaseHTTPRequestHandler) -> dict:
    length = int(handler.headers.get("Content-Length", "0"))
    if not (0 < length <= MAX_BYTES):
        raise ValueError("Tamaño inválido o mayor a 1 MB.")
    data = json.loads(handler.rfile.read(length).decode("utf-8"))
    if not isinstance(data, dict):
        raise ValueError("Se esperaba un objeto JSON.")
    return data

"""
Guarda el código fuente en el espacio de trabajo.
Antes de escribir el archivo, valida su nombre,
contenido y tamaño para evitar entradas inválidas.
"""

def write_workspace(name: object, content: object) -> Path:
    name = clean_name(name)
    if not isinstance(content, str) or len(content.encode("utf-8")) > MAX_BYTES:
        raise ValueError("El contenido debe ser texto de hasta 1 MB.")
    target = WORKSPACE / name
    target.write_text(content, encoding="utf-8")
    return target


# HERRAMIENTAS AUXILIARES EN C

"""
Cada herramienta tiene su archivo fuente y su ejecutable.
El servidor comprueba si están disponibles y los compila
cuando es necesario, sin modificar el compilador principal.
"""

ANALYZER_SRC = IDE_DIR / "ide_analyzer.c"
ANALYZER_EXE = IDE_DIR / "ide_analyzer"
COMPLETION_SRC = IDE_DIR / "ide_completion.c"
COMPLETION_EXE = IDE_DIR / "ide_completion"
SYNTAX_SRC = IDE_DIR / "ide_syntax.c"
SYNTAX_EXE = IDE_DIR / "ide_syntax"
CLONES_SRC = IDE_DIR / "ide_clones.c"
CLONES_EXE = IDE_DIR / "ide_clones"
LL1_SRC = IDE_DIR / "ll1_predict.c"
SYMBOLS_SRC = IDE_DIR / "ide_symbols.c"
SYMBOLS_API = IDE_DIR / "ide_symbols.h"
LL1_HEADER = IDE_DIR / "ll1_table.h"
LL1_API = IDE_DIR / "ll1_predict.h"

"""
Construye el analizador auxiliar cuando su ejecutable
no existe o cuando el archivo fuente fue modificado.
Los errores de compilación se informan sin detener
completamente el servidor del IDE.
"""

def needs_rebuild(exe: Path, *sources: Path) -> bool:
    return not exe.is_file() or any(exe.stat().st_mtime < x.stat().st_mtime for x in sources)


def build_completion() -> bool:
    """Compila el motor de sugerencias escrito en C (independiente de cdff)."""
    if not COMPLETION_SRC.is_file():
        return False
    if not needs_rebuild(COMPLETION_EXE, COMPLETION_SRC, LL1_SRC, LL1_HEADER, LL1_API, SYMBOLS_SRC, SYMBOLS_API):
        return True
    try:
        proc = subprocess.run(
            ["cc", "-std=c11", "-Wall", "-Wextra", "-O2", "-o", str(COMPLETION_EXE), str(COMPLETION_SRC), str(LL1_SRC), str(SYMBOLS_SRC)],
            capture_output=True, text=True, timeout=15, check=False
        )
        if proc.returncode:
            print("AVISO: no se pudo compilar el autocompletado C:\n" + proc.stderr, flush=True)
        return proc.returncode == 0
    except (OSError, subprocess.TimeoutExpired) as exc:
        print(f"AVISO: autocompletado C no disponible: {exc}", flush=True)
        return False

"""
Solicita sugerencias de autocompletado al módulo en C.
El servidor valida el prefijo escrito por el usuario,
ejecuta la herramienta y convierte su salida en datos
que la interfaz puede utilizar para mostrar opciones.
"""

def completions_from_c(prefix: object, context: object) -> list[dict]:
    """Predicciones reales desde LL(1); no existe modo de respaldo léxico."""
    if not isinstance(prefix, str) or not re.fullmatch(r"#?[a-zA-Z][a-zA-Z_0-9]{1,39}", prefix):
        raise ValueError("Prefijo de autocompletado inválido.")
    if not isinstance(context, str) or len(context.encode("utf-8")) > MAX_BYTES:
        raise ValueError("El autocompletado LL(1) requiere contexto del cursor (hasta 1 MB).")
    if not COMPLETION_EXE.is_file():
        raise RuntimeError("Motor de autocompletado LL(1) no disponible; revise gcc/cc.")
    try:
        with tempfile.TemporaryDirectory(prefix="cdff-ll1-") as tmp:
            path = Path(tmp) / "cursor.e"
            path.write_text(context, encoding="utf-8")
            proc = subprocess.run([str(COMPLETION_EXE), prefix, str(path)],
                                  capture_output=True, text=True, timeout=3, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise RuntimeError("Error al consultar el autocompletado LL(1).") from exc
    if proc.returncode:
        raise RuntimeError("No se pudo ejecutar el predictor LL(1).")
    items = []
    for row in proc.stdout.splitlines():
        parts = row.split("\t", 1)
        if len(parts) == 2 and re.fullmatch(r"#?[a-zA-Z][a-zA-Z_0-9]{1,79}", parts[0]):
            items.append({"word": parts[0], "token": parts[1]})
    return items[:12]


def build_tool(source: Path, executable: Path) -> bool:
    """Compila un analizador adicional escrito en C11. No toca cdff."""
    if not source.is_file():
        return False
    dependencies = [source, LL1_SRC, LL1_HEADER, LL1_API] if source == SYNTAX_SRC else [source]
    if not needs_rebuild(executable, *dependencies):
        return True
    try:
        compilation_sources = [source, LL1_SRC] if source == SYNTAX_SRC else [source]
        p = subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-O2",
                            "-o", str(executable), *map(str, compilation_sources)],
                           capture_output=True, text=True, timeout=20, check=False)
        if p.returncode:
            print(f"AVISO: no pude construir {source.name}: {p.stderr}", flush=True)
        return p.returncode == 0
    except (OSError, subprocess.TimeoutExpired) as exc:
        print(f"AVISO: herramienta C no disponible {source.name}: {exc}", flush=True)
        return False


# REVISIÓN SINTÁCTICA Y CORRECCIONES

"""
Ejecuta el analizador sintáctico auxiliar desarrollado en C y recibe las sugerencias de corrección
Cada sugerencia contiene la línea, columna, tipo de corrección, texto esperado y mensaje descriptivo

Las posiciones recibidas en bytes UTF-8 se convierten
a índices UTF-16 para que coincidan con las posiciones
utilizadas por el editor en JavaScript.

Este módulo no decide qué reglas sintácticas son válidas; únicamente comunica y valida los resultados del analizador
"""

def syntax_from_c(source: Path) -> list[dict]:
    """Consulta reglas en C. Cada reemplazo trae texto original exacto y ubicación.

    Python no propone ni transforma código; únicamente valida el protocolo.
    """
    if not SYNTAX_EXE.is_file():
        raise RuntimeError("El analizador sintáctico del IDE en C no está disponible.")
    proc = subprocess.run([str(SYNTAX_EXE), str(source)], capture_output=True,
                          text=True, timeout=5, check=False)
    if proc.returncode:
        raise RuntimeError("El analizador sintáctico en C no pudo revisar el archivo.")
    codes = {"LL1_INSERT", "LL1_REPLACE", "NO_FIX"}
    result = []
    # C cuenta offsets en bytes UTF-8 y el textarea usa índices UTF-16.
    # Convertimos exclusivamente la columna para no desplazar las ediciones
    # cuando hay tildes o emojis antes de la sugerencia.
    source_lines = source.read_text(encoding="utf-8").split("\n")
    for record in proc.stdout.splitlines():
        parts = record.split("\t", 5)
        if len(parts) != 6 or not parts[0].isdigit() or not parts[1].isdigit():
            continue
        line, column, action, expected, replacement, message = parts
        if action not in codes or int(line) < 1 or int(column) < 0:
            continue
        if (len(expected) > 50 or len(replacement) > 50 or
                "\n" in expected + replacement or "\r" in expected + replacement):
            continue
        line_no = int(line)
        if line_no > len(source_lines):
            continue
        raw_line = source_lines[line_no - 1].encode("utf-8")
        byte_col = int(column)
        if byte_col > len(raw_line):
            continue
        try:
            prefix = raw_line[:byte_col].decode("utf-8")
        except UnicodeDecodeError:
            continue
        utf16_col = len(prefix.encode("utf-16-le")) // 2
        result.append({"line": line_no, "column": utf16_col,
                       "code": action, "expected": expected,
                       "replacement": replacement, "message": message})
    return result[:80]

# DETECCIÓN DE CÓDIGO DUPLICADO
"""
Reúne los archivos guardados y, cuando el usuario lo
solicita, también los ejemplos del proyecto.

Envía los archivos al detector implementado en C y
organiza los resultados para mostrar coincidencias
de tipo I y tipo II en la interfaz.
"""

def clones_from_c(src: Path, include_examples: bool) -> dict:
    if not CLONES_EXE.is_file():
        raise RuntimeError("El detector de clones en C no esta disponible.")
    items = [("Guardado/" + p.name, p) for p in sorted(WORKSPACE.glob("*.e")) if p.is_file()]
    if include_examples:
        items += [("Ejemplo/" + p.name, p) for p in sorted(src.glob("*.e")) if p.is_file()]
    if len(items) < 2:
        return {"clones": [], "files_scanned": len(items), "message": "Guarda dos archivos .e para compararlos."}
    if len(items) > 25:
        raise ValueError("Hay mas de 25 archivos. Desactiva los ejemplos o reduce los archivos de prueba.")
    cmd = [str(CLONES_EXE), str(src / "lexer.l")]
    for label, path in items:
        if path.stat().st_size > 1024 * 1024:
            raise ValueError(f"Archivo mayor que 1 MB: {label}")
        cmd += [label, str(path)]
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=15, check=False)
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError("La comparacion tardo demasiado; reduce los archivos.") from exc
    if p.returncode:
        raise RuntimeError("Fallo el detector de clones escrito en C.")
    clones = []
    for row in p.stdout.splitlines():
        parts = row.split("\t")
        if len(parts) == 7 and parts[0] in ("I", "II"):
            try:
                clones.append({"type": parts[0], "file_a": parts[1], "start_a": int(parts[2]),
                               "end_a": int(parts[3]), "file_b": parts[4],
                               "start_b": int(parts[5]), "end_b": int(parts[6])})
            except ValueError:
                continue
    return {"clones": clones, "files_scanned": len(items),
            "message": "Coincidencias entre archivos (3 o mas lineas y 18 o mas tokens)."}


# SERVIDOR HTTP DEL IDE

"""
Define las rutas utilizadas para comunicar la interfaz web con las herramientas del proyecto

Las solicitudes GET permiten consultar archivos, resultados y estado del sistema

Las solicitudes POST permiten guardar, compilar, revisar sintaxis, solicitar autocompletado y
buscar fragmentos de código duplicados.
"""

def handler_factory(src: Path):
    class Handler(BaseHTTPRequestHandler):
        def send_json(self, data: dict, status: int = 200) -> None:
            payload = json.dumps(data, ensure_ascii=False).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)

        # Atiende las solicitudes de consulta realizadas
        # desde la interfaz, incluyendo la carga del editor,
        # la lista de archivos y los resultados generados

        def do_GET(self) -> None:
            url = urlparse(self.path)
            if url.path in ("/", "/index.html"):
                payload = (IDE_DIR / "index.html").read_bytes()
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("X-Content-Type-Options", "nosniff")
                self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
                self.send_header("Pragma", "no-cache")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)
            elif url.path == "/api/status":
                self.send_json({"compiler_ready": (src / "cdff").is_file(), "completion_ready": COMPLETION_EXE.is_file(), "syntax_ready": SYNTAX_EXE.is_file(), "clones_ready": CLONES_EXE.is_file(), "src": str(src), "workspace": str(WORKSPACE)})
            elif url.path == "/api/files":
                examples = [p.name for p in sorted(src.glob("*.e")) if p.is_file()]
                saved = [p.name for p in sorted(WORKSPACE.glob("*.e")) if p.is_file()]
                self.send_json({"examples": examples, "saved": saved})
            
            elif url.path == "/api/artifact":
                params = parse_qs(url.query)
                name = params.get("name", [""])[0]
                download = params.get("download", ["0"])[0] == "1"

                if not re.fullmatch(
                    r"[a-zA-Z0-9_-]+\.(asm|bin)", name
                ):
                    self.send_json(
                        {"error": "Nombre de archivo inválido."}, 400
                    )
                    return

                target = WORKSPACE / name

                if not target.is_file():
                    self.send_json(
                        {"error": "El archivo no existe."}, 404
                    )
                    return

                if target.stat().st_size > MAX_BYTES:
                    self.send_json(
                        {"error": "El archivo supera 1 MB."}, 413
                    )
                    return

                """
                Se conserva el contenido original del archivo para
                permitir su descarga sin modificar ningún byte.
                
                Cuando el usuario solicita visualizar un archivo .bin,
                se genera una representación hexadecimal con direcciones
                y grupos de 16 bytes para facilitar su lectura.
                
                Esta representación es únicamente visual: el archivo
                descargado conserva su contenido original.
                """

                raw = target.read_bytes()

                if download:
                    self.send_response(200)
                    self.send_header(
                        "Content-Type", "application/octet-stream"
                    )
                    self.send_header(
                        "Content-Disposition",
                        f'attachment; filename="{name}"'
                    )
                    self.send_header(
                        "X-Content-Type-Options", "nosniff"
                    )
                    self.send_header(
                        "Content-Length", str(len(raw))
                    )
                    self.end_headers()
                    self.wfile.write(raw)
                    return

                if name.endswith(".asm"):
                    content = raw.decode("utf-8", errors="replace")
                    kind = "asm"
                else:
                    rows = []

                    for offset in range(0, len(raw), 16):
                        chunk = raw[offset:offset + 16]
                        hexadecimal = " ".join(
                            f"{byte:02X}" for byte in chunk
                        )
                        rows.append(
                            f"{offset:08X}  {hexadecimal}"
                        )

                    content = "\n".join(rows)
                    kind = "bin"

                self.send_json({
                    "name": name,
                    "kind": kind,
                    "size": len(raw),
                    "content": content
                })

            elif url.path == "/api/open":
                params = parse_qs(url.query)
                try:
                    name = clean_name(params.get("name", [""])[0])
                    origin = params.get("origin", [""])[0]
                    if origin not in ("example", "saved"):
                        raise ValueError("Origen inválido")
                    file = (src if origin == "example" else WORKSPACE) / name
                    self.send_json({"name": name, "content": file.read_text(encoding="utf-8"), "origin": origin})
                except (ValueError, OSError) as exc:
                    self.send_json({"error": str(exc)}, 400)
            else:
                self.send_json({"error": "Ruta no encontrada"}, 404)

        # Procesa las acciones enviadas desde el editor.
        # Según la ruta solicitada, utiliza la herramienta
        # correspondiente y devuelve una respuesta JSON.

        def do_POST(self) -> None:
            url = urlparse(self.path)
            if url.path not in ("/api/save", "/api/compile", "/api/complete", "/api/check", "/api/clones"):
                self.send_json({"error": "Ruta no encontrada"}, 404)
                return
            try:
                data = read_json(self)
                if url.path == "/api/complete":
                    self.send_json({"items": completions_from_c(
                        data.get("prefix"), data.get("context"))})
                    return
                if url.path == "/api/clones":
                    self.send_json(clones_from_c(src, data.get("include_examples") is True))
                    return
                if url.path == "/api/check":
                    name = clean_name(data.get("name"))
                    content = data.get("content")
                    if not isinstance(content, str) or len(content.encode("utf-8")) > MAX_BYTES:
                        raise ValueError("El codigo supera 1 MB o no es texto.")
                    with tempfile.TemporaryDirectory(prefix="cdff-check-") as tmp:
                        path = Path(tmp) / name
                        path.write_text(content, encoding="utf-8")
                        self.send_json({"suggestions": syntax_from_c(path)})
                    return
                name = clean_name(data.get("name"))
                file = write_workspace(name, data.get("content"))
                if url.path == "/api/save":
                    self.send_json({"ok": True, "name": name, "saved_to": str(file)})
                    return
               
                # EJECUCIÓN DEL COMPILADOR CDFF
                """
                Se verifica que el compilador esté disponible y se
                preparan las rutas de los archivos de salida.
                
                Antes de ejecutar una nueva compilación se eliminan
                los resultados anteriores para evitar mostrar archivos
                que no correspondan al código actual.
                
                La compilación se ejecuta como un proceso independiente.
                Su salida, estado y diagnósticos se envían a la interfaz
                para informar al usuario y mostrar los resultados.
                """

                executable = src / "cdff"
                if not executable.is_file():
                    self.send_json({"ok": False, "message": "No se encontró cdff. Ejecute 'make' en proyecto-1/src desde WSL/Ubuntu y vuelva a compilar.", "lines": [], "output": ""})
                    return
                out = WORKSPACE / (Path(name).stem + ".bin")
                
                asm_out = file.with_suffix(".asm")

                for generated in (out, asm_out):
                    if generated.is_file():
                        generated.unlink()

                try:
                    proc = subprocess.run([str(executable), "-s", "--emit-ir", "-o", str(out), str(file)], cwd=src, text=True, capture_output=True, timeout=30, check=False)
                    output = (proc.stdout + "\n" + proc.stderr).strip()
                    # Separamos el IR de los mensajes del compilador
                    output, ir_text = extract_ir(output)
                    lines = sorted({int(x) for x in re.findall(r"(?:l[ií]nea|line)\s+(\d+)", output, re.IGNORECASE)})
                    artifacts = [p.name for p in (out, file.with_suffix(".asm")) if p.is_file()] if proc.returncode == 0 else []
                    suggestions = (syntax_from_c(file) if proc.returncode != 0 else [])
                    # La revisión C unificada evita consejos duplicados o contradictorios.
                    unique = {}
                    for suggestion in suggestions:
                        unique[(suggestion["line"], suggestion["code"])] = suggestion
                    suggestions = list(unique.values())
                    self.send_json({"ok": proc.returncode == 0, "exit_code": proc.returncode, "output": output,
                                    "lines": lines, "artifacts": artifacts, "suggestions": suggestions, "ir": ir_text})
                except subprocess.TimeoutExpired:
                    self.send_json({"ok": False, "output": "La compilación tardó más de 30 segundos.", "lines": []})
                except OSError as exc:
                    self.send_json({"ok": False, "output": "No se pudo ejecutar cdff: " + str(exc), "lines": []})
            except (ValueError, OSError, RuntimeError, json.JSONDecodeError) as exc:
                self.send_json({"error": str(exc)}, 400)

    return Handler

# INICIALIZACIÓN DEL IDE

"""
Configura la ubicación del compilador y el puerto HTTP.
Comprueba que existan los archivos principales del
lenguaje y prepara las herramientas auxiliares.

Finalmente, inicia el servidor local que permite
utilizar el IDE desde el navegador.
"""

def main():
    ap = argparse.ArgumentParser(description="IDE CDFF Iteración 1 (versión inicial)")
    ap.add_argument("--src", type=Path, default=IDE_DIR.parent / "proyecto-1" / "src", help="ruta a proyecto-1/src")
    ap.add_argument("--port", type=int, default=8877)
    args = ap.parse_args()
    src = args.src.expanduser().resolve()
    if not (src / "lexer.l").is_file() or not (src / "parser.y").is_file():
        ap.error(f"No encuentro el compilador en {src}. Pase --src /ruta/proyecto-1/src")
    build_completion()
    build_tool(SYNTAX_SRC, SYNTAX_EXE)
    build_tool(CLONES_SRC, CLONES_EXE)
    server = ThreadingHTTPServer(("127.0.0.1", args.port), handler_factory(src))
    print(f"IDE CDFF listo: http://localhost:{args.port}", flush=True)
    print(f"Compilador: {src}", flush=True)
    print("Ctrl+C para cerrar.", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
