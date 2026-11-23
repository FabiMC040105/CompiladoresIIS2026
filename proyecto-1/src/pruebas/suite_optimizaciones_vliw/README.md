# Suite de pruebas de optimizaciones VLIW

Esta carpeta contiene 15 programas para evaluar las cinco
optimizaciones implementadas en el compilador CDFF.

## Organización

Cada optimización tiene tres casos de prueba:

- loop_unrolling: desenrollamiento de ciclos.
- dce: eliminación de código muerto.
- renombramiento: renombramiento de registros y dependencias.
- reordenamiento: reordenamiento de instrucciones.
- spilling: manejo de presión sobre los registros.

## Ejecución

Los comandos se ejecutan desde proyecto-1/src.

### Loop Unrolling
./cdff --codegen-ir --unroll 4 --ir-cfg pruebas/suite_optimizaciones_vliw/loop_unrolling/unroll_01.e

### Dead Code Elimination
./cdff --codegen-ir --dce pruebas/suite_optimizaciones_vliw/dce/dce_01.e

### Renombramiento
./cdff --codegen-ir --renombrar-ir pruebas/suite_optimizaciones_vliw/renombramiento/ren_01.e

### Reordenamiento
./cdff --codegen-ir --reordenar-ir pruebas/suite_optimizaciones_vliw/reordenamiento/reo_01.e

### Spilling
./cdff --codegen-ir --spill pruebas/suite_optimizaciones_vliw/spilling/spill_01.e

## Validación

La suite contiene tres pruebas por optimización, para un total de 15.

Las pruebas permiten evaluar el código generado y comparar las
versiones originales con las optimizadas.

La compilación y la validación funcional son etapas diferentes.
La ejecución completa en RTL y la comparación de resultados
se documentarán cuando finalice la integración con VLIW-Feist.

Los programas originales se conservan en sus ubicaciones anteriores.
