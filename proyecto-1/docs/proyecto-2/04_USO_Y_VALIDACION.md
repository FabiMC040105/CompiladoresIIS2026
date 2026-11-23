# 4. Uso del proyecto y resultados de validación

## 4.1 Compilar desde WSL

Desde una distribución Ubuntu en WSL:

```bash
sudo apt update
sudo apt install -y build-essential flex bison make python3 graphviz
```

Entrar **desde la raíz del repositorio**:

```bash
cd proyecto-1/src
make rebuild
./cdff --help
```

Para los pasos siguientes, permanecer en `proyecto-1/src`. Evitar guardar resultados generados en `main` sin revisar el `.gitignore`.

## 4.2 Ejemplos con el código actualizado

```bash
# Compilación con el backend tradicional (desde AST)
./cdff factorial.e

# Obtener IR sin optimizaciones
./cdff --emit-ir factorial.e

# Inspeccionar CFG (esta ejecución no produce el ensamblador final)
./cdff --ir-cfg pruebas/prueba_ir_cfg.e

# DCE con impresión del IR
./cdff --emit-ir --dce factorial.e

# Renombramiento y reordenamiento en IR
./cdff --emit-ir --renombrar-ir --reordenar-ir factorial.e

# Desenrollamiento de factor cuatro, sin exigir --ir-cfg
./cdff --emit-ir --unroll 4 pruebas/prueba_unroll_12.e

# Generar ensamblador y binario desde IR con unrolling
./cdff --codegen-ir --unroll 4 -s pruebas/prueba_unroll_12.e

# Solicitar transformación de spilling desde IR
./cdff --codegen-ir --spill-ir -s pruebas/prueba_spill_backend.e

# Solicitar las cinco fases IR (factor 2 por defecto)
./cdff --codegen-ir --opt-ir -s factorial.e

# Solicitar las cinco fases IR con factor explícito
./cdff --codegen-ir --opt-ir --unroll 4 -s factorial.e
```

**Importante:** los comandos son formas admitidas según el parser del código recibido. La ejecución completa de los últimos cuatro ejemplos, con verificación del valor calculado y del resultado RTL, todavía **no se ha certificado** en esta revisión. Por ello deben ejecutarse en la WSL del equipo y registrarse sus salidas reales. La existencia de `pruebas/prueba_spill_backend.e` no equivale a prueba aprobada de esa ruta.

### Banderas relevantes

| Opción | Comportamiento del código actual |
|---|---|
| `--emit-ir` | Muestra la IR entre marcadores, útil para el IDE. |
| `--ir-cfg` | Analiza/imprime el CFG; suspende el recorrido de codegen final. |
| `--dce` | Activa DCE en la IR. |
| `--renombrar-ir` | Activa renombramiento de instrucciones. |
| `--reordenar-ir` | Activa reordenamiento seguro. |
| `--spill-ir` | Activa la fase de spilling IR. |
| `--spill` | Habilita spilling AST y solicita también spilling IR. |
| `--unroll N` | Activa unrolling con `N` entre 2 y 16. Ya **no requiere** `--ir-cfg`. |
| `--opt-ir` | Activa unrolling, DCE, renombramiento, reordenamiento y spilling. Factor de unrolling 2 si no se fija otro. |
| `--codegen-ir` | Selecciona backend desde IR; las optimizaciones IR solicitadas se aplican antes de emitir ASM. |
| `-s` | Conserva ensamblador final. |
| `-p` | Muestra los paquetes VLIW generados. |
| `-o archivo.bin` | Selecciona el nombre de archivo binario. |
| `-x archivo.asm` | Ensambla directamente un archivo ASM. |

**Inconsistencia menor del código:** el texto de `--help` en `parser.y` todavía afirma `--unroll N ... (requiere --ir-cfg)`. La validación que imponía esa restricción fue eliminada. Conviene corregir la ayuda antes de entregar.

## 4.3 IDE web

Desde la raíz del repositorio:

```bash
python3 ide/app.py
```

Abrir `http://127.0.0.1:8877`. El IDE ofrece editor de archivos, completado contextual LL(1) y de identificadores, diagnósticos y sugerencias de corrección, análisis de clones Tipo I/II y compilación mediante `cdff`.

En `ide/app.py`, el endpoint `/api/compile` lanza `cdff -s --emit-ir -o ... archivo.e`, **sin agregar `--opt-ir` o `--codegen-ir`** por sí solo. Por tanto, la compilación del IDE debe distinguirse de las pruebas de optimizaciones en la terminal.

## 4.4 Pruebas ejecutables

En `proyecto-1/src`:

```bash
make test-loop-unrolling
make test-opt-modulos
bash probar_codegen_ir_backend.sh
```

En la raíz del repositorio:

```bash
python3 ide/tests/test_ll1.py
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
python3 ide/tests/test_clones.py
python3 ide/tests/test_symbol_completion.py
```

El Makefile ofrece asimismo `make test-codegen-ir-backend`, que intenta ejecutar el script directamente. En copias ZIP sin permiso ejecutable puede fallar por permisos; usar `bash probar_codegen_ir_backend.sh` o asegurar el bit ejecutable en Git.

## 4.5 Resultados de esta revisión

**Fuente de la revisión:** ZIP recibido el 22 de noviembre de 2026, con los archivos internos fechados el 23 de noviembre a las 01:15 UTC. **Los resultados se obtuvieron localmente en una copia extraída de ese ZIP**, no en `main` ni en la WSL del equipo.

| Prueba | Resultado | Qué comprueba |
|---|---|---|
| `make test-loop-unrolling` | **PASS** | Equivalencia IR, varios valores de N/factor y rechazos seguros. |
| `make test-opt-modulos` | **PASS** | Pruebas del coordinador, grafo/spill, renombramiento, reordenamiento y DCE. |
| `bash probar_codegen_ir_backend.sh` | **PASS** | Diez escenarios del backend IR, comparación mediante simulador ASM y resolución de etiquetas. |
| `python3 ide/tests/test_ll1.py` | **PASS: 13 pruebas** | Análisis LL(1) auxiliar. |
| `python3 ide/tests/test_ide.py` | **PASS: 9 pruebas** | API/operaciones del IDE. |
| `python3 ide/tests/test_correcciones_matriz.py` | **PASS: 9 pruebas** | Diagnósticos y correcciones. |
| `python3 ide/tests/test_clones.py` | **PASS: 6 pruebas** | Clones y falsos positivos. |
| `python3 ide/tests/test_symbol_completion.py` | **PASS: 12 pruebas** | Completado de identificadores. |
| `make rebuild` | **NO COMPLETADO POR ENTORNO** | Faltan los ejecutables `bison` y `flex` en el entorno de esta revisión. No se llegó a compilar el parser completo. |

Las diez pruebas del script backend IR son: `factorial`, `para`, `mientras`, `condicional`, `aritmetica`, `comparaciones`, `factorial_recursivo`, `lista_parametro`, `ast_factorial` y `ast_para`.

**Alcance de los PASS:** las pruebas pasan para los casos incluidos. No prueban, por sí mismas, un archivo `.e` con las **cinco fases IR** hasta ejecución sobre la implementación RTL ni prueban de manera aislada el soporte nuevo de `cdff_spill_area` en el backend. Ese estado se mantiene como pendiente en la sección 7.

## 4.6 Registro requerido de métricas

La especificación exige medir para cada programa antes/después: instrucciones, tamaño del código en bytes, ciclos estimados, tiempo de compilación e instrucciones eliminadas, reordenadas o renombradas. El grupo también debe contrastar los resultados con ciclos y comportamiento de memoria en la arquitectura.

| Programa | Fase | Inst. antes | Inst. después | Tamaño ASM/bin (bytes) | Ciclos estimados | Tiempo de compilación | Transformaciones |
|---|---|---:|---:|---:|---:|---:|---|
| [programa 1] | Sin optimizar | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | No aplica |
| [programa 1] | DCE | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| [programa 1] | Unrolling | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| [programa 2] | Reordenamiento | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| [programa 3] | Spilling | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |

Esta tabla es una **plantilla de recolección**, no presenta resultados inventados. Para interpretar los datos, usar el mismo programa de entrada, el mismo hardware/simulador y las mismas condiciones de medición, cambiando solamente las fases que se comparan.
