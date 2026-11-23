# Documentación del Proyecto Grupal II — CE1108

**Instituto Tecnológico de Costa Rica · Compiladores e Intérpretes · II semestre de 2026**

Este directorio documenta el estado del compilador **CDFF** y su IDE en el último avance disponible. El Proyecto Grupal II se desarrolla conjuntamente con Arquitectura de Computadores I, cuyo procesador VLIW ejecuta los programas generados por el compilador.

## Documentos

| Documento | Contenido |
|---|---|
| [01 — Introducción y ambiente](01_INTRODUCCION_AMBIENTE.md) | Contexto, objetivos, herramientas y estructura del repositorio. |
| [02 — Diseño y diagramas](02_DISENO_DIAGRAMAS.md) | Arquitectura del software, relaciones UML y flujo de funcionalidades. |
| [03 — Optimizaciones](03_OPTIMIZACIONES.md) | IR, CFG, DCE, renombramiento, reordenamiento, loop unrolling y spilling. |
| [04 — Uso y validación](04_USO_Y_VALIDACION.md) | Comandos de WSL, flags, IDE y resultados reproducibles de pruebas. |
| [05 — Atributos CI y PM](05_ATRIBUTOS_CI_PM.md) | Preguntas y respuestas requeridas por el enunciado. |
| [06 — Bitácora y cierre](06_BITACORA_CIERRE.md) | Bitácora por integrante, conclusiones provisionales, recomendaciones y referencias. |
| [07 — Pendientes](07_PENDIENTES.md) | Aspectos aún no terminados ni validados de extremo a extremo. |

Se incluyen diagramas fuente en [`diagramas/`](diagramas/), editables con PlantUML.

## Alcance y criterio de documentación

Se documenta **lo que aparece en el código** y lo que se comprobó mediante pruebas locales, sin dar por completadas funciones pendientes. La presencia de una función o un test unitario no equivale a integración con el procesador VLIW ni a cumplimiento de todos los escenarios de la defensa.

En particular, el pipeline de optimizaciones IR está implementado y tiene pruebas locales satisfactorias, pero el backend IR aún rechaza **spilling IR**; la ruta de `--unroll` exige `--ir-cfg` y no está conectada a la generación final de código máquina. Estos límites están detallados en los documentos 03 y 07.

> **Para completar antes de entregar:** identificar autores del grupo, agregar horas reales, ejecutar pruebas en la versión final del repositorio, obtener mediciones comparativas y completar la integración pendiente con Arquitectura.

## Base documental

- Código fuente disponible en `proyecto-1/src/`.
- Implementación del IDE en `ide/`.
- Especificación oficial **Proyecto Grupal II: Análisis de Impacto de Memoria Caché y Optimizaciones de Compilador en Procesador Propio tipo VLIW**, CE4301/CE1108, II semestre de 2026.
- Documentación del proyecto anterior en `proyecto-1/docs/` y `README.md`.
