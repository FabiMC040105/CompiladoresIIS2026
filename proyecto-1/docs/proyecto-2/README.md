# Proyecto Grupal II — Documentación técnica de Compiladores e Intérpretes

**Instituto Tecnológico de Costa Rica · CE1108 · II semestre de 2026**

**Versión documentada:** archivo `CompiladoresIIS2026-fix-integracion-final(1).zip`, último avance compartido el 22 de noviembre de 2026 (hora de Costa Rica). Esta documentación corresponde al **contenido de esa rama/copia**, no implica que se haya fusionado a `main` ni que se hayan corregido los errores futuros.

## Contenido

| Archivo | Sección | Contenido |
|---|---|---|
| [01_INTRODUCCION_AMBIENTE.md](01_INTRODUCCION_AMBIENTE.md) | Introducción y ambiente | Objetivos, tecnologías, estructura y dependencias. |
| [02_DISENO_DIAGRAMAS.md](02_DISENO_DIAGRAMAS.md) | Diseño | Arquitectura, flujo, diagramas UML, módulos e IDE. |
| [03_OPTIMIZACIONES.md](03_OPTIMIZACIONES.md) | Optimizaciones | IR, CFG, liveness, DCE, renombramiento, reordenamiento, unrolling y spilling. |
| [04_USO_Y_VALIDACION.md](04_USO_Y_VALIDACION.md) | Uso y validación | Instalación WSL, comandos, pruebas locales y límites. |
| [05_ATRIBUTOS_CI_PM.md](05_ATRIBUTOS_CI_PM.md) | Atributos | Preguntas y respuestas para CI y PM exigidas por el profesor. |
| [06_BITACORA_CIERRE.md](06_BITACORA_CIERRE.md) | Cierre | Bitácora por integrante, conclusiones, recomendaciones y referencias. |
| [07_PENDIENTES.md](07_PENDIENTES.md) | Pendientes | Lista priorizada de verificación final y diferencias entre ramas. |
| [08_CAMBIOS_AVANCE.md](08_CAMBIOS_AVANCE.md) | Trazabilidad | Diferencias concretas respecto del ZIP `integration-ir-optimizaciones-local`. |
| [09_SUITE_Y_METRICAS.md](09_SUITE_Y_METRICAS.md) | Benchmarks | 15 programas obligatorios, medición, gráficos y condiciones para interpretar cifras. |
| [10_INTEGRACION_Y_IMPORTACIONES.md](10_INTEGRACION_Y_IMPORTACIONES.md) | Integración | Pruebas de importaciones, optimizaciones juntas y ejecución con RTL de CE4301. |
| [11_VERIFICACION_LOCAL.md](11_VERIFICACION_LOCAL.md) | Evidencia | Comprobaciones ejecutadas sobre el ZIP, reproducibilidad y limitaciones del entorno. |

**Diagramas UML editables:** [diagramas/](diagramas/).

## Estado resumido

El código actual integra **Loop Unrolling → DCE → Renombramiento → Reordenamiento → Spilling**, con generación de ensamblador desde IR mediante `--codegen-ir`, asignación de temporales por grafo de interferencia/coloreo y empaquetamiento en bundles VLIW.

El nuevo avance añade una **suite de 15 programas evaluables**, scripts de métricas gráficas, tres pruebas de spilling orientadas al recorrido IR→ASM/binario, casos de importación mediante `#traigase` y un testbench de integración con el RTL actual de Arquitectura. Estos artefactos existen en el ZIP; **no todos pudieron ejecutarse durante esta revisión** por falta de Flex/Bison para construir `cdff` y de una copia del RTL de destino validada en este entorno.

Las pruebas modulares que no requieren construir `cdff` y 58 pruebas Python del IDE, más 6 del frontend JS, pasaron. Los resultados concretos y el alcance están en [11_VERIFICACION_LOCAL.md](11_VERIFICACION_LOCAL.md).

## Cómo integrar esta documentación al repositorio

Se puede agregar `proyecto-1/docs/proyecto-2/` a `main` **antes** del código de las ramas de desarrollo, siempre que se indique que describe el avance `fix-integracion-final`. Al fusionar el código después, repetir las pruebas en `main` y ajustar los documentos al commit final. La ruta elegida no sobrescribe `README.md`, archivos fuente ni documentación histórica de otras carpetas.

La especificación del profesor (sección 8.2.1) solicita también una bitácora con horas reales por estudiante. Está dejada como plantilla, no como información inventada.
