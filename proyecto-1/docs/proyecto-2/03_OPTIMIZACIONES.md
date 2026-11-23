# 3. Representación intermedia, análisis y optimizaciones

## 3.1 IR de tres direcciones

Los archivos `ir.c/.h` definen `IRProgram` como una secuencia dinámica de instrucciones `IRInstruction`, con contadores de temporales y etiquetas. Cada instrucción conserva operación, argumentos, resultado, campo adicional, línea de origen y un puntero no propietario `nodo_origen` al AST. `ir_ast.c` construye la IR desde el árbol generado por el parser. Se representan operaciones aritméticas y lógicas, asignaciones, saltos, funciones, parámetros, arreglos y matrices. Los temporales suelen escribirse como `%tN` y las etiquetas como `@LN`.

`basic_blocks.c` identifica líderes, bloques básicos y aristas del grafo de flujo de control (CFG). `liveness.c` calcula información de usos y definiciones e información IN/OUT para bloques e instrucciones, utilizada por DCE y spilling. `dependencias.c` y `dag.c` contribuyen al análisis necesario para renombrar y reordenar instrucciones de forma segura.

Se puede inspeccionar la IR con `--emit-ir` y el CFG con `--ir-cfg`. Este último modo no continúa hacia la generación de ensamblador.

## 3.2 Coordinador de optimizaciones: estado actualizado

`optimizaciones_ir.h` define cinco bits independientes:

| Fase | Constante | Objetivo |
|---|---|---|
| Desenrollamiento | `OPT_IR_UNROLL` | Reducir sobrecarga de control duplicando cuerpos de ciclos admitidos. |
| Eliminación de código muerto | `OPT_IR_DCE` | Eliminar cómputos sin uso, preservando efectos observables. |
| Renombramiento | `OPT_IR_RENOMBRAR` | Evitar falsas dependencias WAR/WAW en casos seguros. |
| Reordenamiento | `OPT_IR_REORDENAR` | Aprovechar independencia preservando RAW/WAR/WAW y control. |
| Spilling | `OPT_IR_SPILL` | Resolver presión de registros con almacenamiento y recargas. |

`OPT_IR_TODAS` incluye **las cinco fases**, no cuatro. `opt_ir_aplicar_config()` recibe la máscara, cantidad de colores, registro base, factor de unrolling y el destino de estadísticas. Primero clona la IR. Si una fase falla, descarta la copia y conserva la IR original. Si todas terminan, reemplaza la IR de entrada con el resultado.

**Orden implementado:**

```text
IR inicial
   |
   v
Loop Unrolling  (factor de 2 a 16; si la fase está activa)
   |
   v
DCE  ->  Renombramiento  ->  Reordenamiento  ->  Spilling
   |
   v
IR definitiva + OptIREstadisticas
```

La función de compatibilidad `opt_ir_aplicar()` sigue disponible y llama a la función configurable con factor **2**. El parser invoca `opt_ir_aplicar_config()` en la ruta de inspección y en la ruta del backend IR, pasando el factor indicado por `--unroll N`, o **2** cuando se activa el desenrollamiento por `--opt-ir` sin definir factor.

`OptIREstadisticas` guarda los contadores de las cinco fases y el total de instrucciones antes y después. `opt_ir_imprimir()` reporta ciclos encontrados/desenrollados/omitidos, eliminaciones DCE, definiciones y usos renombrados, regiones reordenadas y cantidad de operaciones de spilling.

## 3.3 Loop Unrolling

**Archivos:** `loop_unrolling.c/.h` y `pruebas/test_loop_unrolling.c`.

**Regla:** cuando se reconoce un ciclo que satisface las precondiciones, se emiten varias copias del cuerpo por iteración transformada, ajustando temporales y el control de salida. El factor configurable puede tomar valores de **2 a 16**. La configuración admite límites de crecimiento, con valores predeterminados de 32 instrucciones para el cuerpo y 256 instrucciones extra.

**Restricciones:** no se debe desenrollar un ciclo cuya estructura de control o efectos no puedan conservarse con seguridad. El optimizador puede omitir ciclos por condiciones internas o por heurística de tamaño. Por tanto, activar la bandera no garantiza que todo ciclo se desenrolle.

**Cambio en este avance:** la función que copia instrucciones (`emitir_copia`) también copia `nodo_origen`. Este vínculo permite al backend recuperar información de ámbitos desde el AST original al traducir la IR transformada.

**Pruebas:** el conjunto `make test-loop-unrolling` verifica casos con diferentes tamaños de iteración, factores, rechazo de saltos internos, heurística de crecimiento y equivalencia de resultados. En la revisión local resultó **PASS**.

**Ejemplo conceptual:**

```text
para i de 0 a N-1: total += a[i]
```

Con factor 4, el cuerpo optimizado puede procesar cuatro posiciones por grupo cuando se cumplen las precondiciones, manteniendo la lógica necesaria para cualquier resto. Esta ilustración es conceptual: el IR emitido concreto depende de la estructura reconocida por el código.

## 3.4 Eliminación de código muerto (DCE)

**Archivos:** `dead_code_elimination.c/.h`, `liveness.c/.h`, `basic_blocks.c/.h`.

Se eliminan definiciones cuyo valor no se utiliza cuando el cálculo puede retirarse sin cambiar efectos observables. Llamadas, escrituras relevantes de memoria, retornos y decisiones de control deben conservarse. El algoritmo trabaja con información de variables vivas y puede ejecutar varias pasadas.

```text
Antes:                            Después:
  a = 5                             a = 5
  %t0 = 10 + 20                     return a
  return a
```

**Validación:** pruebas `test_dce*.c` contemplan memoria, llamadas, ciclos, variables globales y locales. Se ejecutaron dentro de `make test-opt-modulos` con resultado **PASS**.

## 3.5 Renombramiento de instrucciones

**Archivos:** `renombramiento.c/.h`, `dependencias.c/.h`, `dag.c/.h`.

Renombra definiciones y usos admitidos para evitar dependencias artificiales **WAR** y **WAW**, sin romper la dependencia verdadera **RAW**. Conserva criterios restrictivos ante ciclos, uniones de control, llamadas, memoria o nombres ambiguos. El cambio se aplica sobre el IR, no sobre el código fuente del usuario.

```text
Antes:                   Forma conceptual renombrada:
%t0 = a + b               %t0 = a + b
x = %t0                   x = %t0
%t0 = c + d               %t1 = c + d
return %t0                return %t1
```

**Validación:** `prueba_renombramiento.c` verifica antidependencias, colisiones, ciclos, llamadas, preservación del resultado y recuperación ante errores. Resultado local: **PASS**.

## 3.6 Reordenamiento seguro

**Archivos:** `reordenamiento.c/.h`, `dependencias.c/.h`, `dag.c/.h`.

Reordena operaciones dentro de regiones seguras según su DAG, preservando dependencias **RAW/WAR/WAW**, relaciones de control y barreras de memoria/llamadas. Busca mejorar la disponibilidad de operaciones independientes sin alterar el resultado.

```text
%t0 = a + b
%t1 = %t0 * 2
%t2 = c + d
```

La última suma puede anticiparse cuando resulta independiente; la multiplicación no puede ejecutarse antes de que `%t0` esté disponible. La calendarización real VLIW es otra fase y requiere comprobar la semántica de latencias y bundles.

**Validación:** `prueba_reordenamiento.c` cubre dependencias, caminos de control, funciones, efectos y gran cantidad de bloques. Resultado local: **PASS**.

## 3.7 Spilling de registros y reserva de memoria

**Archivos:** `grafo_interferencia.c/.h`, `asignacion_registros.c/.h`, `spill_transform.c/.h` y `codegen_ir.c`.

El compilador calcula variables vivas, construye un grafo de interferencia e intenta colorearlo según la cantidad de registros disponibles. Si quedan temporales sin color, reescribe usos/definiciones introduciendo cargas y almacenes hacia el área reservada `cdff_spill_area`, y repite el análisis hasta eliminar la presión, fallar o alcanzar el límite de rondas.

Para mantener la compatibilidad de la IR, las recargas y escrituras internas usan `IR_ARRAY_LOAD` e `IR_ARRAY_STORE`; `cdff_spill_area` **no es un arreglo definido por el usuario**, sino un identificador reservado para el proceso de optimización.

**Backend IR actual (`codegen_ir.c`):**

- Detecta los accesos al identificador reservado `cdff_spill_area` como `IR_ARRAY_STORE` e `IR_ARRAY_LOAD`.
- Reserva slots de cuatro bytes en una región que no se superpone con los datos globales y los temporales internos, verificando que se respete el límite de memoria.
- Materializa el **cálculo de dirección en `x13`** y genera `LSU STORE xN, 0(x13)` o `LSU LOAD xN, 0(x13)` según el registro asignado. Si el destino no tiene color válido, emplea `x3` como auxiliar y la ruta de escritura al destino correspondiente.
- Los temporales coloreables se asignan a **nueve registros `x4`–`x12`**; el resolvedor puede rechazar presión excesiva si no se activa spilling.
- La baseline de diagnóstico `--codegen-ir-mem` desactiva el mapeo Chaitin para permitir comparación con temporales en memoria. **No equivale** al modo normal sin optimizaciones en programas con alta presión.

**Nueva evidencia preparada en el avance:** `make test-spill-obligatorio` comprueba tres archivos `.e` con presión de 10, 12 y 14 argumentos. El script exige valores no nulos de `STORE`, `LOAD` y slots de spilling en el log, y verifica que el ASM contenga operaciones LSU con `x13` y que se genere binario. `make test-spill-completo` combina estas comprobaciones con pruebas modulares y backend IR. Su ejecución completa requiere un `cdff` compilado y no se certifica únicamente por inspeccionar el script.

**Verificación modular efectuada:** los módulos de grafos, transformación y coordinación de IR pasaron los casos incluidos. Para la validez funcional de los programas completos, la suite de 15 casos compara valores esperados y exige **RAW = 0** usando `simular_asm_ir.py`. Ver [09_SUITE_Y_METRICAS.md](09_SUITE_Y_METRICAS.md).

## 3.8 Integración con el backend VLIW

El procesador objetivo utiliza bundles de 128 bits con cuatro slots de 32 bits: ALU, LSU, BRU y FEIST. El ensamblador se resuelve mediante `resolucion_etiquetas.c`, se empaqueta en `vliw_packer.c` y se codifica con `binario.c`; los slots vacíos utilizan ceros.

En `parser.y`, sin `--codegen-ir` se mantiene el backend tradicional desde AST. Con `--codegen-ir` se reconstruye una IR desde el AST, se aplican las fases solicitadas y `codegen_ir_generar_archivo()` produce ASM. Las fases de análisis IR que se muestran al usuario no se reutilizan por referencia; el backend vuelve a generar y optimizar su propia copia.

**El comportamiento correcto del código generado y su mejora de rendimiento requieren validación adicional** con los benchmarks y la implementación RTL de Arquitectura de Computadores I. Una reducción de instrucciones IR no es una prueba directa de reducción de ciclos, misses o energía.

## 3.9 Pruebas de interacción y medición de las fases

El avance agrega entradas `.e` por optimización en `pruebas/suite_optimizaciones_vliw/` y una herramienta que compara código base/optimizado en **instrucciones IR, tamaño de código, ciclos estimados, tiempo de compilación y transformaciones**. Los casos fuerzan una transformación real de la fase aislada. El resultado no siempre mejora simultáneamente las cinco métricas: unrolling puede aumentar bytes y el compilador puede tardar más. Ver [09_SUITE_Y_METRICAS.md](09_SUITE_Y_METRICAS.md).
