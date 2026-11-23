# 8. Cambios observados respecto del ZIP `integration-ir-optimizaciones-local`

Se compararon las rutas internas y el hash SHA-256 de archivos entre el ZIP previo `CompiladoresIIS2026-integration-ir-optimizaciones-local (1).zip` y el actual `CompiladoresIIS2026-fix-integracion-final(1).zip`. Se detectaron **32 archivos nuevos**, **11 archivos modificados** y **ninguno eliminado**. Estas cifras corresponden a archivos regulares, no directorios.

## 8.1 Archivos modificados

| Archivo | Área y cambio principal |
|---|---|
| `proyecto-1/src/codegen_ir.c` y `.h` | Backend IR, asignación por coloreo y materialización de operaciones de memoria/spill. |
| `proyecto-1/src/spill_transform.c` | Transformación y gestión de presión de registros. |
| `proyecto-1/src/codegen.c` | Ajustes al backend original. |
| `proyecto-1/src/parser.y` | Cambios en rutas de generación, opciones e integración. |
| `proyecto-1/src/Makefile` | Nuevas metas para pruebas integradas, importaciones, métricas y entrega. |
| `proyecto-1/src/probar_codegen_ir_backend.sh` | Escenarios de backend y su validación. |
| `proyecto-1/src/pruebas/test_codegen_ir_runtime.c` | Pruebas de backend IR. |
| `proyecto-1/src/pruebas/simular_asm_ir.py` | Simulador funcional para comparar resultados y RAW. |
| `proyecto-1/src/factorial.asm` y `ejercicio_modulo.e` | Actualizaciones en casos/artefactos de ejemplo. |
| `proyecto-1/src/.gitignore` | Ajustes de exclusiones para salidas generadas. |

## 8.2 Archivos nuevos destacados

- `proyecto-1/docs/SPILLING_REGISTER.md`: descripción interna de grafo, coloreo, asignación y transformación de spills.
- `pruebas/suite_optimizaciones_vliw/`: **15 archivos `.e`**, organizados en cinco categorías.
- `pruebas/medir_suite_optimizaciones.py`: comparación de baseline y versión optimizada, verificación de resultados y generación de métricas gráficas.
- `pruebas/probar_spilling_obligatorio.sh` y tres entradas `spill_0*_...e`: presión de 10, 12 y 14 argumentos.
- `pruebas/probar_importaciones.sh` y `pruebas/importaciones/`: `#traigase`, importaciones anidadas y reporte de errores.
- `pruebas/integracion_3_opt.e` y `pruebas/compilar_integracion_3_opt.sh`: ejecución de cuatro banderas de optimización (independientemente del nombre histórico).
- `pruebas/probar_integracion_rtl_actual.sh` y `pruebas/tb_integracion_rtl_actual.sv`: ruta de compilador optimizado a CPU, L1, L2 y memoria del RTL actual.

**Importante:** el ZIP trae también binarios y ensambladores de ejemplo en `pruebas/importaciones/`. No son evidencia de que la revisión local actual los haya recompilado.

## 8.3 Cambios que deben reflejarse en la defensa

1. La asignación del backend IR emplea **Chaitin con registros `x4`–`x12`**, y los spill a memoria se traducen con `x13` para la dirección, no con el anterior ejemplo que suponía `x11`.
2. Ya hay **programas de prueba** para los cinco grupos exigidos por el profesor, con un runner que puede generar un reporte gráfico.
3. Se añadió un test de integración de compilador y RTL actual (incluyendo métricas de caché) que debe verificarse contra la rama correcta del repositorio de Arquitectura.
4. El proyecto ofrece objetivos `make test-entrega-local` y `make test-entrega`; el segundo depende además del repositorio de CE4301.
5. Esta documentación puede subirse por adelantado a `main`, pero sus afirmaciones sobre el código corresponden a la rama/copia analizada hasta que se fusionen los cambios.
