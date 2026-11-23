#!/usr/bin/env python3
"""Ejecuta las 15 pruebas obligatorias y genera métricas/gráficas sin dependencias externas."""
from __future__ import annotations

import csv
import html
import json
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parent.parent
SUITE = ROOT / "pruebas" / "suite_optimizaciones_vliw"
WORK = ROOT / "evidencia-vliw" / "metricas-optimizaciones"
OUT = ROOT.parent / "docs" / "metricas-optimizaciones"
SIM = ROOT / "pruebas" / "simular_asm_ir.py"
CDFF = ROOT / "cdff"

@dataclass(frozen=True)
class Caso:
    nombre: str
    categoria: str
    fuente: Path
    esperado: int
    flags: tuple[str, ...]
    base_memoria: bool = False

CASOS = [
    Caso("unroll_01", "Loop unrolling", SUITE/"loop_unrolling"/"unroll_01.e", 66, ("--unroll", "4")),
    Caso("unroll_02", "Loop unrolling", SUITE/"loop_unrolling"/"unroll_02.e", 10, ("--unroll", "4")),
    Caso("unroll_03", "Loop unrolling", SUITE/"loop_unrolling"/"unroll_03.e", 499500, ("--unroll", "4")),
    Caso("dce_01", "DCE", SUITE/"dce"/"dce_01.e", 5, ("--dce",)),
    Caso("dce_02", "DCE", SUITE/"dce"/"dce_02.e", 11, ("--dce",)),
    Caso("dce_03", "DCE", SUITE/"dce"/"dce_03.e", 3, ("--dce",)),
    Caso("ren_01", "Renombramiento", SUITE/"renombramiento"/"ren_01.e", 11, ("--renombrar-ir",)),
    Caso("ren_02", "Renombramiento", SUITE/"renombramiento"/"ren_02.e", 32, ("--renombrar-ir",)),
    Caso("ren_03", "Renombramiento", SUITE/"renombramiento"/"ren_03.e", 10, ("--renombrar-ir",)),
    Caso("reo_01", "Reordenamiento", SUITE/"reordenamiento"/"reo_01.e", 20, ("--reordenar-ir",)),
    Caso("reo_02", "Reordenamiento", SUITE/"reordenamiento"/"reo_02.e", 25, ("--reordenar-ir",)),
    Caso("reo_03", "Reordenamiento", SUITE/"reordenamiento"/"reo_03.e", 16, ("--reordenar-ir",)),
    Caso("spill_01", "Spilling", SUITE/"spilling"/"spill_01.e", 55, ("--spill-ir",), True),
    Caso("spill_02", "Spilling", SUITE/"spilling"/"spill_02.e", 90, ("--spill-ir",), True),
    Caso("spill_03", "Spilling", SUITE/"spilling"/"spill_03.e", 133, ("--spill-ir",), True),
]

RE_IR = re.compile(r"Instrucciones IR procesadas:\s*(\d+)")
RE_BYTES = re.compile(r"Bytes de instrucciones:\s*(\d+)")
RE_OPT = re.compile(r"\[OPT IR\] instrucciones:\s*(\d+)\s*->\s*(\d+)")
RE_UNROLL = re.compile(r"\[UNROLL IR\] encontrados=(\d+) desenrollados=(\d+) omitidos=(\d+)")
RE_DCE = re.compile(r"\[OPT IR\] DCE: eliminadas=(\d+) pasadas=(\d+)")
RE_REN = re.compile(r"\[OPT IR\] Renombramiento: definiciones=(\d+) usos=(\d+)")
RE_REO = re.compile(r"\[OPT IR\] Reordenamiento: regiones=(\d+) instrucciones=(\d+)")
RE_SPILL = re.compile(r"\[SPILL IR\] rondas=(\d+) STORE=(\d+) LOAD=(\d+) slots=(\d+)")


def run(cmd: list[str], log: Path) -> float:
    inicio = time.perf_counter()
    p = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    ms = (time.perf_counter() - inicio) * 1000.0
    log.write_text(p.stdout, encoding="utf-8")
    if p.returncode:
        print(p.stdout[-4000:], file=sys.stderr)
        raise RuntimeError(f"fallo comando ({p.returncode}): {' '.join(cmd)}")
    return ms


def compilar(src: Path, binario: Path, flags: tuple[str, ...], log: Path, memoria: bool=False) -> float:
    modo = "--codegen-ir-mem" if memoria else "--codegen-ir"
    return run([str(CDFF), modo, "-s", "-v", *flags, "-o", str(binario), str(src)], log)


def simular(asm: Path, esperado: int, resumen: Path, log: Path) -> dict:
    run([sys.executable, str(SIM), str(asm), "--resultado", "0", str(esperado),
         "--solo-resumen", "--validar-raw", "--resumen-json", str(resumen)], log)
    return json.loads(resumen.read_text(encoding="utf-8"))


def entero(rx: re.Pattern[str], texto: str, grupo: int=1, default: int=0) -> int:
    m = rx.search(texto)
    return int(m.group(grupo)) if m else default


def efecto(categoria: str, log: str) -> tuple[int, dict[str, int]]:
    extra: dict[str, int] = {}
    if categoria == "Loop unrolling":
        m = RE_UNROLL.search(log)
        if not m: return 0, extra
        extra = {"loops_encontrados": int(m.group(1)), "loops_desenrollados": int(m.group(2)), "loops_omitidos": int(m.group(3))}
        return extra["loops_desenrollados"], extra
    if categoria == "DCE":
        m = RE_DCE.search(log)
        if not m: return 0, extra
        extra = {"eliminadas": int(m.group(1)), "pasadas_dce": int(m.group(2))}
        return extra["eliminadas"], extra
    if categoria == "Renombramiento":
        m = RE_REN.search(log)
        if not m: return 0, extra
        extra = {"definiciones_renombradas": int(m.group(1)), "usos_renombrados": int(m.group(2))}
        return extra["definiciones_renombradas"] + extra["usos_renombrados"], extra
    if categoria == "Reordenamiento":
        m = RE_REO.search(log)
        if not m: return 0, extra
        extra = {"regiones_reordenadas": int(m.group(1)), "instrucciones_reordenadas": int(m.group(2))}
        return extra["instrucciones_reordenadas"], extra
    if categoria == "Spilling":
        m = RE_SPILL.search(log)
        if not m: return 0, extra
        extra = {"rondas_spill": int(m.group(1)), "stores_spill": int(m.group(2)), "loads_spill": int(m.group(3)), "slots_spill": int(m.group(4))}
        return extra["stores_spill"] + extra["loads_spill"], extra
    return 0, extra


def barra_svg(rows: list[dict], key_base: str, key_opt: str, titulo: str, archivo: Path, unidad: str="") -> None:
    width, height = 1500, 650
    ml, mr, mt, mb = 80, 30, 70, 170
    cw, ch = width-ml-mr, height-mt-mb
    vals = [float(r.get(key_base) or 0) for r in rows] + [float(r.get(key_opt) or 0) for r in rows]
    vmax = max(vals + [1.0])
    group = cw / len(rows)
    bw = min(24.0, group * 0.28)
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
             '<rect width="100%" height="100%" fill="white"/>',
             f'<text x="{width/2}" y="34" text-anchor="middle" font-family="Arial" font-size="24">{html.escape(titulo)}</text>',
             f'<line x1="{ml}" y1="{mt+ch}" x2="{ml+cw}" y2="{mt+ch}" stroke="black"/>',
             f'<line x1="{ml}" y1="{mt}" x2="{ml}" y2="{mt+ch}" stroke="black"/>']
    for tick in range(6):
        value = vmax * tick / 5
        y = mt + ch - ch * tick / 5
        parts.append(f'<line x1="{ml-5}" y1="{y:.1f}" x2="{ml+cw}" y2="{y:.1f}" stroke="#ddd"/>')
        parts.append(f'<text x="{ml-10}" y="{y+5:.1f}" text-anchor="end" font-family="Arial" font-size="12">{value:.0f}</text>')
    for i, r in enumerate(rows):
        cx = ml + group*(i+0.5)
        for dx, key, fill in [(-bw*0.65, key_base, "#777"), (bw*0.65, key_opt, "#222")]:
            v = r.get(key)
            if v is None:
                continue
            h = ch * float(v) / vmax
            x = cx + dx - bw/2
            y = mt + ch - h
            parts.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{bw:.1f}" height="{h:.1f}" fill="{fill}"/>')
        label = html.escape(r["prueba"])
        parts.append(f'<text x="{cx:.1f}" y="{mt+ch+18}" transform="rotate(55 {cx:.1f} {mt+ch+18})" font-family="Arial" font-size="11">{label}</text>')
    parts += [f'<rect x="{width-250}" y="20" width="14" height="14" fill="#777"/><text x="{width-230}" y="32" font-family="Arial" font-size="12">Base</text>',
              f'<rect x="{width-170}" y="20" width="14" height="14" fill="#222"/><text x="{width-150}" y="32" font-family="Arial" font-size="12">Optimizado</text>',
              f'<text x="20" y="{mt+ch/2}" transform="rotate(-90 20 {mt+ch/2})" font-family="Arial" font-size="13">{html.escape(unidad)}</text>',
              '</svg>']
    archivo.write_text("\n".join(parts), encoding="utf-8")


def reporte_html(rows: list[dict]) -> None:
    cols = ["prueba", "categoria", "resultado", "ir_base", "ir_opt", "bytes_base", "bytes_opt", "ciclos_base", "ciclos_opt", "compilacion_base_ms", "compilacion_opt_ms", "transformaciones", "raw_base", "raw_opt"]
    trs=[]
    for r in rows:
        tds=[]
        for c in cols:
            v=r.get(c)
            if isinstance(v,float): v=f"{v:.2f}"
            if v is None: v="N/A"
            tds.append(f"<td>{html.escape(str(v))}</td>")
        trs.append("<tr>"+"".join(tds)+"</tr>")
    imgs=["instrucciones.svg","tamano_codigo.svg","ciclos.svg","tiempo_compilacion.svg","transformaciones.svg"]
    doc=f'''<!doctype html><html lang="es"><head><meta charset="utf-8"><title>Métricas de optimizaciones</title>
<style>body{{font-family:Arial,sans-serif;max-width:1500px;margin:30px auto;padding:0 20px}}table{{border-collapse:collapse;width:100%;font-size:12px}}th,td{{border:1px solid #bbb;padding:6px}}th{{background:#eee;position:sticky;top:0}}img{{width:100%;height:auto;margin:20px 0}}.ok{{font-weight:bold}}</style></head><body>
<h1>Suite de 15 pruebas de optimizaciones VLIW</h1>
<p class="ok">PASS: 15/15 programas preservan el resultado esperado y no presentan riesgos RAW en el simulador funcional.</p>
<p>La columna Base usa el mismo backend con Chaitin, excepto en Spilling: allí se usa la baseline diagnóstica <code>--codegen-ir-mem</code> para representar el esquema previo de temporales en memoria, ya que el programa intencionalmente excede los nueve registros físicos.</p>
<table><thead><tr>{''.join(f'<th>{html.escape(c)}</th>' for c in cols)}</tr></thead><tbody>{''.join(trs)}</tbody></table>
{''.join(f'<h2>{html.escape(i.rsplit(".",1)[0].replace("_"," ").title())}</h2><img src="{i}" alt="{i}">' for i in imgs)}
</body></html>'''
    (OUT/"reporte.html").write_text(doc, encoding="utf-8")


def main() -> int:
    if not CDFF.exists():
        print("ERROR: falta ./cdff. Ejecute make primero.", file=sys.stderr)
        return 2
    WORK.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    rows=[]
    for idx, caso in enumerate(CASOS, 1):
        print(f"[{idx:02d}/15] {caso.nombre} ({caso.categoria})")
        d=WORK/caso.nombre
        if d.exists(): shutil.rmtree(d)
        d.mkdir(parents=True)
        base_e=d/"base.e"; opt_e=d/"optimizado.e"
        shutil.copy2(caso.fuente, base_e); shutil.copy2(caso.fuente, opt_e)

        base_log=d/"base.log"; opt_log=d/"optimizado.log"
        base_bin=d/"base.bin"; opt_bin=d/"optimizado.bin"
        base_ms=compilar(base_e, base_bin, (), base_log, memoria=caso.base_memoria)
        opt_ms=compilar(opt_e, opt_bin, caso.flags, opt_log, memoria=False)
        btxt=base_log.read_text(encoding="utf-8", errors="replace")
        otxt=opt_log.read_text(encoding="utf-8", errors="replace")

        base_json=d/"base_sim.json"; opt_json=d/"opt_sim.json"
        bsim=simular(base_e.with_suffix(".asm"), caso.esperado, base_json, d/"base_sim.log")
        osim=simular(opt_e.with_suffix(".asm"), caso.esperado, opt_json, d/"opt_sim.log")
        if bsim["estadisticas"].get("riesgos_raw",0) or osim["estadisticas"].get("riesgos_raw",0):
            raise RuntimeError(f"{caso.nombre}: se detectaron riesgos RAW")

        mopt=RE_OPT.search(otxt)
        ir_before=int(mopt.group(1)) if mopt else entero(RE_IR,btxt)
        ir_after=int(mopt.group(2)) if mopt else entero(RE_IR,otxt)
        ntrans, extra=efecto(caso.categoria, otxt)
        if ntrans <= 0:
            raise RuntimeError(f"{caso.nombre}: la optimizacion no reporto transformaciones")

        row={
            "prueba":caso.nombre, "categoria":caso.categoria, "resultado":caso.esperado,
            "ir_base":ir_before, "ir_opt":ir_after,
            "bytes_base":entero(RE_BYTES,btxt), "bytes_opt":entero(RE_BYTES,otxt),
            "ciclos_base":bsim["estadisticas"].get("ciclos",0), "ciclos_opt":osim["estadisticas"].get("ciclos",0),
            "bundles_base":bsim["estadisticas"].get("bundles_estaticos",0), "bundles_opt":osim["estadisticas"].get("bundles_estaticos",0),
            "compilacion_base_ms":base_ms, "compilacion_opt_ms":opt_ms,
            "transformaciones":ntrans, "raw_base":bsim["estadisticas"].get("riesgos_raw",0), "raw_opt":osim["estadisticas"].get("riesgos_raw",0),
            **extra,
        }
        rows.append(row)
        print(f"      PASS resultado={caso.esperado} IR={ir_before}->{ir_after} ciclos={row['ciclos_base']}->{row['ciclos_opt']} transformaciones={ntrans}")

    campos=sorted({k for r in rows for k in r.keys()})
    with (OUT/"metricas.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=campos); w.writeheader(); w.writerows(rows)
    (OUT/"metricas.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")

    barra_svg(rows,"ir_base","ir_opt","Conteo de instrucciones IR",OUT/"instrucciones.svg","instrucciones")
    barra_svg(rows,"bytes_base","bytes_opt","Tamaño de código",OUT/"tamano_codigo.svg","bytes")
    barra_svg(rows,"ciclos_base","ciclos_opt","Ciclos estimados por simulador",OUT/"ciclos.svg","ciclos")
    barra_svg(rows,"compilacion_base_ms","compilacion_opt_ms","Tiempo de compilación",OUT/"tiempo_compilacion.svg","ms")
    # Para transformaciones la base conceptual es cero.
    trans_rows=[{**r,"trans_base":0,"trans_opt":r["transformaciones"]} for r in rows]
    barra_svg(trans_rows,"trans_base","trans_opt","Transformaciones aplicadas",OUT/"transformaciones.svg","cantidad")
    lineas = [
        "# Métricas de las 15 pruebas obligatorias", "",
        "Todas las pruebas verifican el mismo resultado antes y después de la optimización y exigen RAW=0 en el simulador funcional.", "",
        "| Prueba | Optimización | IR antes | IR después | Bytes antes | Bytes después | Ciclos antes | Ciclos después | Transformaciones |",
        "|---|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for r in rows:
        lineas.append(
            f"| {r['prueba']} | {r['categoria']} | {r['ir_base']} | {r['ir_opt']} | "
            f"{r['bytes_base']} | {r['bytes_opt']} | {r['ciclos_base']} | {r['ciclos_opt']} | {r['transformaciones']} |"
        )
    lineas += [
        "", "## Gráficas", "",
        "![Conteo de instrucciones](instrucciones.svg)", "",
        "![Tamaño de código](tamano_codigo.svg)", "",
        "![Ciclos estimados](ciclos.svg)", "",
        "![Tiempo de compilación](tiempo_compilacion.svg)", "",
        "![Transformaciones](transformaciones.svg)", "",
        "Para las pruebas de spilling, la línea base usa `--codegen-ir-mem`, que conserva los temporales en memoria. Esto permite medir el costo del esquema previo aun cuando el programa excede intencionalmente los nueve registros físicos x4-x12.", ""
    ]
    (OUT/"README.md").write_text("\n".join(lineas), encoding="utf-8")
    reporte_html(rows)
    print("\n==================================================")
    print("PASS: 15/15 PRUEBAS DE OPTIMIZACIONES")
    print("PASS: equivalencia funcional y RAW=0 en base/optimizado")
    print(f"Metricas: {OUT/'metricas.csv'}")
    print(f"Reporte grafico: {OUT/'reporte.html'}")
    print("==================================================")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
