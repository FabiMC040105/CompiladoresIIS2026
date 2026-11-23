# 4. Instrucciones de uso y validación

## 4.1 Requisitos en WSL

Desde una terminal Ubuntu/WSL:

```bash
sudo apt update
sudo apt install -y build-essential flex bison make python3 graphviz
```

Situarse en la raíz del repositorio (`CompiladoresIIS2026`) y construir:

```bash
cd proyecto-1/src
make
./cdff --help
```

## 4.2 Operaciones principales de CDFF

```bash
# Compilación tradicional desde AST
./cdff factorial.e

# Inspeccionar IR
./cdff --emit-ir factorial.e

# Analizar bloques básicos y CFG
./cdff --ir-cfg factorial.e

# DCE y visualización de IR
./cdff --dce --emit-ir factorial.e

# Renombrar y reordenar (ruta de análisis)
./cdff --renombrar-ir --reordenar-ir --emit-ir factorial.e

# Probar loop unrolling; aún es ruta de análisis, no binario optimizado
./cdff --ir-cfg --unroll 4 pruebas/prueba_unroll_12.e

# Backend desde IR (sin spilling IR)
./cdff --codegen-ir --dce factorial.e

# Conserva ASM final y muestra bundles
./cdff -s -p factorial.e
```

**No usar como demostración de integración final:** `./cdff --codegen-ir --opt-ir ...` porque `--opt-ir` incluye spilling IR y ese backend lo rechaza actualmente. `--spill-ir` y `--spill` tampoco deben presentarse como spilling IR plenamente traducido hasta código máquina.

### Banderas relevantes

| Bandera | Función observada |
|---|---|
| `--emit-ir` | Muestra la representación intermedia con marcadores para el IDE. |
| `--ir-cfg` | Muestra bloques básicos/CFG, sin entrar a emisión final de ASM en esta ruta. |
| `--dce` | Solicita eliminación de código muerto en IR. |
| `--renombrar-ir` | Solicita renombramiento sobre IR. |
| `--reordenar-ir` | Solicita reordenamiento sobre IR. |
| `--spill-ir` | Solicita transformación de spilling IR. |
| `--spill` | Activa spilling en backend AST y también selecciona fase IR. |
| `--opt-ir` | Solicita DCE, renombramiento, reordenamiento y spilling IR. |
| `--unroll N` | Factor 2–16; requiere `--ir-cfg` en esta versión. |
| `--codegen-ir` | Selecciona backend alternativo basado en IR. |
| `-s`, `-p`, `-o` | Conserva ASM, imprime paquetes, configura archivo de salida. |

## 4.3 IDE web

Desde la **raíz** del repositorio:

```bash
python3 ide/app.py
```

Abrir en el navegador de Windows o WSL: `http://127.0.0.1:8877`.

El IDE permite editar y guardar programas, compilar mediante `cdff`, consultar IR y resultados, proponer completados según contexto LL(1), reportar problemas sintácticos y detectar clones entre archivos. El parser LL(1) auxiliar del IDE es distinto del parser de producción Flex/Bison.

La ruta `/api/compile` ejecuta actualmente `cdff -s --emit-ir -o ...` **sin activar automáticamente las optimizaciones**. Por ello, una compilación desde la interfaz no equivale a probar las cuatro optimizaciones.

El IDE tiene su propia documentación técnica en `ide/README.md`, `ide/DEFENSA_LL1.md` y `ide/CORRECCIONES_Y_CLONES.md`.

## 4.4 Comandos para pruebas

Desde `proyecto-1/src`:

```bash
make test-loop-unrolling
make test-opt-modulos
bash probar_codegen_ir_backend.sh
```

Desde la raíz del repositorio:

```bash
python3 ide/tests/test_ll1.py
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
python3 ide/tests/test_clones.py
python3 ide/tests/test_symbol_completion.py
```

Para compilar todo el frontend se requieren `flex` y `bison`. Para pruebas RTL se necesita, además, el proyecto de Arquitectura y las dependencias de simulación.

**Detalle de permisos:** en la extracción ZIP utilizada para esta revisión, `make test-codegen-ir-backend` falló al intentar ejecutar `probar_codegen_ir_backend.sh` sin permiso de ejecución. El mismo script **sí pasó** usando `bash probar_codegen_ir_backend.sh`. Verificar en WSL si el script conserva el bit ejecutable antes de usar el objetivo `make`.

## 4.5 Evidencia de validación del avance recibido

Se ejecutaron las siguientes pruebas sobre la copia extraída del ZIP del **22 de noviembre de 2026**, en un entorno GNU/Linux, sin modificar el código fuente. No se afirma que esos resultados correspondan a commits posteriores.

| Prueba ejecutada | Resultado de esta revisión | Alcance |
|---|---|---|
| `make test-loop-unrolling` | PASS | Transformación y equivalencia de resultados. |
| `make test-opt-modulos` | PASS | Integración IR, spilling, grafo de interferencia, renombramiento, reordenamiento y regresiones DCE. |
| `bash probar_codegen_ir_backend.sh` | PASS | Diez pruebas del backend IR y resolución de etiquetas, con simulación ASM del propio script. |
| `python3 ide/tests/test_ll1.py` | PASS (13 pruebas) | Predicción sintáctica y gramática auxiliar LL(1). |
| `python3 ide/tests/test_ide.py` | PASS (9 pruebas) | API y comportamiento de edición/diagnóstico. |
| `python3 ide/tests/test_correcciones_matriz.py` | PASS (9 pruebas) | Correcciones y casos conservadores. |
| `python3 ide/tests/test_clones.py` | PASS (6 pruebas) | Clones Tipo I/II y falsos positivos. |
| `python3 ide/tests/test_symbol_completion.py` | PASS (12 pruebas) | Completado de identificadores. |
| `make test-codegen-ir-backend` | No ejecutó el script | Permiso de ejecución faltante en la copia ZIP; el script pasó invocando `bash`. |

Los programas del backend IR que reportaron PASS fueron: factorial, `para`, `mientras`, condicional, aritmética, comparaciones, factorial recursivo, lista como parámetro y dos casos derivados de AST (`ast_factorial`, `ast_para`). La simulación del script **no sustituye** la ejecución RTL en el procesador VLIW con cachés.

## 4.6 Mediciones comparativas pendientes

El enunciado solicita resultados gráficos por prueba. La siguiente tabla es **una plantilla, no datos observados**:

| Benchmark | Optimización | Instrucciones antes/después | Código (bytes) | Ciclos estimados | Tiempo de compilación | Eliminadas/reordenadas/renombradas |
|---|---|---|---|---|---|---|
| Por definir | Sin optimizar | Pendiente | Pendiente | Pendiente | Pendiente | No aplica |
| Por definir | DCE | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| Por definir | Loop unrolling | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| Por definir | Reordenamiento | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| Por definir | Spilling | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |

**Pendientes para la evidencia de defensa:** comprobar al menos tres programas por cada categoría exigida por la sección 6 de la especificación, registrar métricas comparables y validar la ejecución sobre la arquitectura real del grupo.
