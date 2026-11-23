# 3. Representación intermedia y optimizaciones

## 3.1 Representación intermedia (IR)

La IR implementada en `ir.h` utiliza una secuencia de `IRInstruction` administrada por `IRProgram`. Cada instrucción contiene una operación (`IROp`), hasta cuatro campos de operandos/resultados, la línea de origen y un puntero no propietario al nodo AST. Se admiten asignaciones, aritmética, comparaciones, etiquetas, saltos, funciones, parámetros y operaciones con arreglos y matrices.

Los temporales tienen nombres como `%t0`; las etiquetas, nombres como `@L0`. Esta representación facilita optimizar sin depender de un registro físico concreto.

La IR se genera desde el AST mediante `ir_generate()` y puede imprimirse usando `--emit-ir`. `basic_blocks.c` construye bloques básicos y CFG, que alimentan los análisis posteriores.

## 3.2 Orden de las fases

En `optimizaciones_ir.c`, `opt_ir_aplicar()` primero clona la IR y luego ejecuta las fases elegidas en este orden:

```text
IR de entrada
    ↓
DCE  →  Renombramiento  →  Reordenamiento  →  Spilling
    ↓
IR transformada y estadísticas
```

Cada fase se activa por máscara. La transformación se confirma al terminar satisfactoriamente; ante un error, el IR de entrada no se sustituye por una versión parcialmente transformada. **Loop unrolling** se ejecuta desde `parser.y` antes de esa secuencia, exclusivamente en la ruta IR/CFG activada.

## 3.3 Eliminación de código muerto (DCE)

**Archivos:** `dead_code_elimination.c/.h`, `liveness.c/.h`, `basic_blocks.c/.h`.

**Objetivo:** suprimir resultados que no se usan cuando la operación no tiene un efecto que deba conservarse. Se usa análisis de variables vivas y se consideran flujo y efectos de memoria, llamadas y retorno.

**Ejemplo de principio de transformación:**

```text
x = 5
%t0 = 10 + 20      # su valor nunca se usa
return x
```

Resultado esperado:

```text
x = 5
return x
```

**Condición de seguridad:** no eliminar una escritura a memoria, llamada con efectos u otra operación observable simplemente porque su valor no aparece en una lectura posterior.

**Validación disponible:** pruebas `test_dce*.c` sobre ciclos, funciones, memoria, variables locales y globales. También se prueba la fase junto con renombramiento y reordenamiento.

## 3.4 Renombramiento

**Archivos:** `renombramiento.c/.h`, `dependencias.c/.h`, `dag.c/.h`.

**Objetivo:** crear versiones distintas de temporales o definiciones admitidas, evitando dependencias falsas como WAR y WAW cuando es seguro. Se preservan las dependencias de lectura reales (RAW).

**Ejemplo conceptual:**

```text
%t0 = a + b
x = %t0
%t0 = c + d
return %t0
```

Se puede conservar la primera versión y renombrar la segunda, actualizando exclusivamente los usos correspondientes. El algoritmo real aplica criterios conservadores para funciones, ciclos y variables, y evita renombrar casos ambiguos.

**Precondiciones principales:** identificar la definición que alimenta cada uso, no atravesar barreras inseguras y mantener el valor observable al salir de la región.

**Validación disponible:** `prueba_renombramiento.c` contempla WAW, WAR, combinaciones, CFG, llamadas, ciclos y casos ambiguos.

## 3.5 Reordenamiento seguro de instrucciones

**Archivos:** `reordenamiento.c/.h`, `dependencias.c/.h`, `dag.c/.h`.

**Objetivo:** buscar órdenes de ejecución que permitan aprovechar operaciones independientes sin violar RAW, WAR, WAW ni dependencias de control y memoria. El código construye información de dependencias, valida el nuevo orden y aplica criterios sobre presión local de temporales.

**Ejemplo conceptual:**

```text
%t0 = a + b
%t1 = %t0 * 2
%t2 = c + d
```

La suma independiente que produce `%t2` puede adelantarse, pero `%t1` debe mantenerse después de `%t0`.

**Precondiciones:** sólo reordenar cuando el DAG lo permita, preservar barreras por llamadas y memoria, y mantener la semántica de saltos y funciones.

**Validación disponible:** `prueba_reordenamiento.c` incluye dependencias, regiones, condiciones, ciclos, memoria, funciones y pruebas con gran cantidad de bloques.

## 3.6 Spilling de registros

**Archivos:** `grafo_interferencia.c/.h`, `asignacion_registros.c/.h`, `spill_transform.c/.h`, `liveness.c/.h`.

**Objetivo:** resolver la presión cuando hay más valores simultáneamente vivos que registros disponibles. Se calcula liveness, se construye un grafo de interferencia y se intenta colorearlo con un número de registros disponible. Cuando es necesario, se reescribe la IR insertando pseudoaccesos de carga/almacenamiento en el área interna `cdff_spill_area` y se repite el análisis.

**Ejemplo conceptual:** si diez temporales requieren coexistir y se dispone de nueve colores, al menos uno puede necesitar almacenamiento temporal fuera de registros. El conjunto exacto de temporales afectados depende del grafo.

**Validación disponible:** `prueba_grafo_interferencia.c`, `prueba_spill_transform.c`, `prueba_integracion_opt_ir.c`. Las pruebas ejecutadas incluyen presión real y comprobación de resultado equivalente.

**Limitación actual:** la transformación de spilling IR existe y pasa pruebas locales, pero **`parser.y` rechaza `--codegen-ir` cuando hay `OPT_IR_SPILL`**, porque falta bajar sus pseudoaccesos a memoria en el backend. No debe afirmarse que el flujo completo de spilling IR emite un `.bin` funcional. Existe también una opción `--spill` que habilita un mecanismo del backend tradicional desde AST; no confundir ambas rutas.

## 3.7 Loop unrolling

**Archivos:** `loop_unrolling.c/.h` y `pruebas/test_loop_unrolling.c`.

**Objetivo:** desenrollar cuerpos de ciclos reconocidos con factor configurable, copiando operaciones y renombrando los temporales generados en cada copia.

**Configuración:** la CLI acepta factores de **2 a 16**. La estructura `LoopUnrollConfig` permite limitar el tamaño del cuerpo (valor predeterminado 32 instrucciones) y el crecimiento extra (valor predeterminado 256 instrucciones).

**Precondiciones y heurística:** se reconoce una forma simple de ciclo presente en el IR y se exige un salto de regreso válido en el CFG. Se rechazan cuerpos con saltos internos u operaciones con efectos no contemplados, temporales que escaparían de forma insegura o un crecimiento excesivo. Se pueden omitir ciclos cortos reconocidos cuando el factor no aporta beneficio.

**Ejemplo conceptual:** un ciclo que suma valores puede agrupar varias iteraciones en el mismo cuerpo transformado, manteniendo controles de salida para no alterar el resultado.

**Validación disponible:** `make test-loop-unrolling`, que ejecuta verificaciones de equivalencia.

**Limitación actual de CLI:** `--unroll N` exige `--ir-cfg`. Ese modo imprime el IR/CFG transformado, pero no entra en la ruta de generación de ensamblador del mismo llamado. Por otra parte, `--codegen-ir` reconstruye el IR desde el AST sin pasar por `loop_unroll()`. Falta integrar el IR desenrollado con el backend definitivo.

## 3.8 Relación con VLIW

El procesador cuenta con cuatro slots de 32 bits: ALU, LSU, BRU y FEIST. El backend dispone de módulos de calendarización, resolución de etiquetas y empaquetamiento. La representación binaria de un bundle de 128 bits respeta el orden de slots descrito en el README histórico del repositorio.

Una optimización correcta del IR no garantiza automáticamente una mejora de ciclos. Debe además verificarse que la secuencia producida respete dependencias entre bundles, latencias y comportamiento del hardware, y medir el resultado con un benchmark representativo.
