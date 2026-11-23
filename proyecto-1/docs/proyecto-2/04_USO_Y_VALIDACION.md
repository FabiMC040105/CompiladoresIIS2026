# 4. Uso del proyecto y validación

## 4.1 Preparación en WSL

En la **raíz del repositorio**:

```bash
sudo apt update
sudo apt install -y build-essential flex bison make python3 graphviz iverilog
cd proyecto-1/src
make rebuild
./cdff --help
```

Opcional para pruebas del frontend: instalar Node.js y verificar `node --version`. Para pruebas sobre el procesador se necesita el repositorio separado de Arquitectura.

## 4.2 Comandos de ejemplo

Desde `proyecto-1/src`:

```bash
# Construcción y ejemplo simple
make
./cdff --codegen-ir -s factorial.e

# Mostrar IR y bloques básicos (análisis)
./cdff --emit-ir factorial.e
./cdff --ir-cfg pruebas/prueba_ir_cfg.e

# Activar fases individuales
./cdff --codegen-ir --unroll 4 -s pruebas/suite_optimizaciones_vliw/loop_unrolling/unroll_01.e
./cdff --codegen-ir --dce -s pruebas/suite_optimizaciones_vliw/dce/dce_01.e
./cdff --codegen-ir --renombrar-ir -s pruebas/suite_optimizaciones_vliw/renombramiento/ren_01.e
./cdff --codegen-ir --reordenar-ir -s pruebas/suite_optimizaciones_vliw/reordenamiento/reo_01.e
./cdff --codegen-ir --spill-ir --spill-mapa -s pruebas/suite_optimizaciones_vliw/spilling/spill_01.e

# Pipeline de cinco fases (factor 2 predeterminado)
./cdff --codegen-ir --opt-ir -s factorial.e

# Baseline de temporales en memoria (diagnóstico)
./cdff --codegen-ir-mem -s factorial.e
```

`--unroll N` acepta de 2 a 16. La ayuda de `parser.y` todavía dice «requiere --ir-cfg», pero la condición antigua fue retirada de la lógica de validación: es una **inconsistencia de mensaje**, no una recomendación de uso.

## 4.3 Pruebas recomendadas, en orden

```bash
# Pruebas por módulos e IDE
make test-loop-unrolling
make test-opt-modulos
make test-ide

# Construyen cdff y requieren Flex/Bison
make test-codegen-ir-backend
make test-importaciones
make test-spill-completo
make test-suite-optimizaciones
make metricas-optimizaciones

# Batería local completa
make test-entrega-local
```

Para el test RTL, desde el mismo directorio, con el código de Arquitectura **ya integrado en la ruta indicada**:

```bash
make test-rtl-actual ARQ_ROOT="$HOME/TEC/Arqui1/Proyecto2/Arquitectura1-IIS2026"
# Alternativa: incluye toda la batería local y después RTL
make test-entrega ARQ_ROOT="$HOME/TEC/Arqui1/Proyecto2/Arquitectura1-IIS2026"
```

El Makefile verifica también `python/assembler.py` del repositorio de Arquitectura. `test-rtl-actual` requiere que `top.sv` tenga los puertos usados por `tb_integracion_rtl_actual.sv`; no basta con que la carpeta exista. Si la integración aún está en una rama `feature`, ejecutar sobre una copia del RTL de esa rama o esperar a que llegue a `development`/`main`.

## 4.4 Prueba combinada y enlaces entre archivos

```bash
bash pruebas/compilar_integracion_3_opt.sh
make test-importaciones
```

La prueba `integracion_3_opt.e` compila una línea base y una versión con `--unroll 2 --dce --renombrar-ir --reordenar-ir`; el nombre del script es histórico y **el comando visible activa cuatro fases**. Compara el ASM mediante el simulador y señala que debe comprobarse el resultado almacenado en memoria. No confundir esta prueba con el test RTL, que también solicita `--spill-ir`.

`test-importaciones` comprueba imports transitivos con `#traigase`, duplicados y error ante un archivo inexistente; el simulador exige `resultado=22` en `RAM[0]` y `RAW=0`.

## 4.5 IDE web

Desde la raíz del repositorio:

```bash
python3 ide/app.py
```

Abrir `http://127.0.0.1:8877`. El IDE integra completado LL(1), completado de símbolos, detección y sugerencias de errores, clones Tipo I/II y solicitud de compilación. El endpoint `/api/compile` ejecuta `cdff` con parámetros propios del IDE; **no se debe asumir** que selecciona todas las optimizaciones o el backend IR de la misma forma que los comandos de terminal.

## 4.6 Salidas generadas

| Ruta | Contenido |
|---|---|
| `src/evidencia-vliw/metricas-optimizaciones/` | Archivos temporales de compilación, ASM, binarios, logs y resúmenes por prueba. |
| `proyecto-1/docs/metricas-optimizaciones/` | `metricas.csv`, `metricas.json`, `reporte.html`, `README.md` y gráficas SVG (después de ejecutar el runner). |
| `src/evidencia-vliw/spilling/` | Logs de las tres pruebas de spilling. |
| `src/evidencia-vliw/rtl-integracion-final/` | Binario, simulador funcional, `iverilog.log`, `rtl.log` y VCD del test RTL. |

Para los resultados y limitaciones de **esta revisión**, ver [11_VERIFICACION_LOCAL.md](11_VERIFICACION_LOCAL.md). No se incluyen resultados numéricos inventados de scripts no ejecutados.
