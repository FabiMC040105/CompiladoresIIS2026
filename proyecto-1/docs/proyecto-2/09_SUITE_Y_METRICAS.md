# 9. Suite de quince programas y análisis de métricas

## 9.1 Cobertura del enunciado

La sección 6 de la especificación solicita tres programas por cada optimización o dimensión de seguridad: 15 en total. El repositorio actual incluye los siguientes archivos en `proyecto-1/src/pruebas/suite_optimizaciones_vliw/`:

| Categoría | Programas | Característica y resultados esperados |
|---|---|---|
| Loop Unrolling | `unroll_01.e`, `unroll_02.e`, `unroll_03.e` | N constante (66), N variable pequeño (10), N grande (499500). |
| Eliminación de código muerto | `dce_01.e`, `dce_02.e`, `dce_03.e` | Código muerto intercalado con llamadas que modifican `efecto`: 5, 11, 3. |
| Renombramiento | `ren_01.e`, `ren_02.e`, `ren_03.e` | WAW, WAR y ciclos: 11, 32, 10. |
| Reordenamiento | `reo_01.e`, `reo_02.e`, `reo_03.e` | Condicionales, funciones, efectos globales y ciclo: 20, 25, 16. |
| Spilling | `spill_01.e`, `spill_02.e`, `spill_03.e` | Presión superior a nueve registros (10, 12 y 14 argumentos): 55, 90, 133. |

Los resultados indicados son **oráculos codificados en `medir_suite_optimizaciones.py`**, no mediciones reproducidas en esta revisión. Todos los programas verifican la variable global `resultado` en dirección de memoria 0.

## 9.2 Metodología de ejecución y comparación

En `proyecto-1/src/`:

```bash
make rebuild
make test-suite-optimizaciones
# El objetivo siguiente ejecuta la suite y genera el reporte
make metricas-optimizaciones
```

Por caso, el runner:

1. Copia el mismo `.e` a una entrada **base** y otra **optimizada**.
2. Compila ambas con el backend IR; en Spilling, la base usa `--codegen-ir-mem` porque los programas exceden los nueve registros `x4`–`x12`.
3. Ejecuta ambos ASM mediante `pruebas/simular_asm_ir.py`.
4. Exige que cada resultado coincida con el oráculo y que los riesgos **RAW sean cero**.
5. Comprueba que el log de la versión optimizada registre un número positivo de transformaciones de la fase examinada.
6. Registra métricas y genera un CSV, JSON, reporte HTML y gráficas SVG sin dependencias Python externas.

## 9.3 Métricas reportadas

| Columna | Interpretación |
|---|---|
| `ir_base`, `ir_opt` | Conteo de instrucciones de IR antes/después. |
| `bytes_base`, `bytes_opt` | Tamaño de instrucciones del binario generado según log. |
| `ciclos_base`, `ciclos_opt` | Ciclos **estimados por el simulador funcional de ASM**. No son ciclos del RTL. |
| `compilacion_base_ms`, `compilacion_opt_ms` | Tiempo de compilación en milisegundos; depende del equipo y carga del sistema. |
| `transformaciones` | Conteo de transformaciones propias de la categoría. Su significado varía: ciclos desenrollados, eliminaciones, definiciones/usos, instrucciones reordenadas o LOAD+STORE de spill. |
| `raw_base`, `raw_opt` | Número de riesgos RAW detectados por el simulador del ASM. |

Se generan `instrucciones.svg`, `tamano_codigo.svg`, `ciclos.svg`, `tiempo_compilacion.svg` y `transformaciones.svg` más `reporte.html`. Ubicación: `proyecto-1/docs/metricas-optimizaciones/`.

**Precaución en el análisis:** los conteos de transformaciones **no son equivalentes entre categorías**. En `Loop Unrolling`, un aumento de IR y de bytes no implica necesariamente un error; puede ser el precio de reducir el control del ciclo. En Spilling, la baseline distinta introduce una diferencia metodológica que se debe mencionar en cualquier gráfica.

## 9.4 Tabla para pegar resultados reales de WSL

| Programa | Estado | IR base→opt | Bytes base→opt | Ciclos simulador base→opt | Compilación ms base→opt | Transformaciones |
|---|---|---|---|---|---|---|
| `unroll_01` | Pendiente de ejecutar | — | — | — | — | — |
| `unroll_02` | Pendiente de ejecutar | — | — | — | — | — |
| `unroll_03` | Pendiente de ejecutar | — | — | — | — | — |
| `dce_01`–`dce_03` | Pendiente de ejecutar | — | — | — | — | — |
| `ren_01`–`ren_03` | Pendiente de ejecutar | — | — | — | — | — |
| `reo_01`–`reo_03` | Pendiente de ejecutar | — | — | — | — | — |
| `spill_01`–`spill_03` | Pendiente de ejecutar | — | — | — | — | — |

No completar con cifras hasta verificar `docs/metricas-optimizaciones/metricas.csv` en la WSL, idealmente sobre la versión integrada de `main`.

## 9.5 Relación con métricas del procesador

La especificación conjunta también considera IPC, ciclos de ejecución y comportamiento de L1/L2. El runner actual estima ciclos sobre ASM y calcula métricas de compilación. Para afirmar beneficios **sobre el hardware**, los mismos benchmarks deben ejecutarse en la implementación RTL de CE4301 con su jerarquía de caché y contadores. Esta etapa es distinta de la suite funcional de 15 casos.
