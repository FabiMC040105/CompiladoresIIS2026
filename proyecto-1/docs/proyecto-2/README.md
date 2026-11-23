# Proyecto Grupal II — Documentación técnica de Compiladores e Intérpretes

**Instituto Tecnológico de Costa Rica · CE1108 · II semestre de 2026**

Esta documentación describe la implementación disponible del compilador **CDFF**, su IDE y su integración con un backend VLIW. Está redactada a partir del último ZIP recibido el **22 de noviembre de 2026** (archivos internos con fecha del 23 de noviembre de 2026 a las 01:15 UTC), de la especificación oficial del Proyecto Grupal II y de pruebas ejecutadas sobre esa misma copia del código.

## Índice

| Documento | Tema |
|---|---|
| [01 — Introducción y ambiente](01_INTRODUCCION_AMBIENTE.md) | Contexto, objetivos y herramientas. |
| [02 — Diseño y diagramas](02_DISENO_DIAGRAMAS.md) | Arquitectura del compilador, UML y funcionalidades del IDE. |
| [03 — IR y optimizaciones](03_OPTIMIZACIONES.md) | CFG, DCE, renombramiento, reordenamiento, spilling y loop unrolling. |
| [04 — Uso y validación](04_USO_Y_VALIDACION.md) | Comandos WSL, banderas, pruebas realmente ejecutadas y tablas para métricas. |
| [05 — Atributos CI y PM](05_ATRIBUTOS_CI_PM.md) | Preguntas y respuestas exigidas por el profesor. |
| [06 — Bitácora y cierre](06_BITACORA_CIERRE.md) | Horas por integrante por completar, conclusiones, recomendaciones y referencias. |
| [07 — Pendientes](07_PENDIENTES.md) | Validaciones y evidencias que aún hacen falta. |
| [08 — Cambios del avance](08_CAMBIOS_AVANCE.md) | Comparación puntual de los seis archivos cambiados y trazabilidad de las pruebas. |

Los fuentes editables de diagramas UML se encuentran en [diagramas/](diagramas/).

## Estado de esta revisión

Las cinco fases IR están integradas en el coordinador: **Loop Unrolling → DCE → Renombramiento → Reordenamiento → Spilling**. El código más reciente también añadió al backend IR la reserva de slots internos de spilling y su traducción a instrucciones de memoria. Las pruebas modulares y las diez pruebas autónomas del backend pasaron, junto con 49 pruebas Python del IDE.

**Esto no implica que la integración final en RTL esté comprobada.** Falta generar y ejecutar programas de demostración de las cinco fases desde archivos fuente `.e`, registrar las métricas y completar la bitácora real. Además, en el entorno de análisis utilizado faltaba `bison`, por lo que `make rebuild` no pudo completarse aquí.

## Cómo agregar esta carpeta al repositorio desde WSL

Este ZIP contiene únicamente la ruta `proyecto-1/docs/proyecto-2/`; no reemplaza archivos `.c`, `.h`, `parser.y` ni `Makefile`.

Desde la raíz de un clon actualizado y **sin cambios locales pendientes**, descomprimir el ZIP descargado:

```bash
unzip '/mnt/c/Users/TU_USUARIO/Downloads/documentacion_compiladores_actualizada.zip' -d .
git status --short
git add proyecto-1/docs/proyecto-2
git diff --cached --stat
git commit -m 'docs: actualizar documentacion del Proyecto II con integracion IR'
git push origin main
```

La ruta de Descargas debe ajustarse al nombre real del usuario de Windows. Antes de hacer `git pull --ff-only origin main`, confirmar que está en la rama correcta y que no haya trabajo sin guardar o cambios del equipo por integrar. Si el repositorio requiere pull request, respetar ese flujo en vez de hacer push directo.

## Fuente y límites de verificación

- Especificación oficial del Proyecto Grupal II de CE1108/CE4301, II semestre 2026.
- Archivos de `proyecto-1/src/`, `ide/` y documentación existente del repositorio.
- Pruebas realizadas localmente sobre la copia recibida, **no** sobre el hash final de `main`.
- Los marcadores como `[Nombre]`, `[Horas]` y `Pendiente` deben completarse con información real del grupo; no se inventaron resultados ni nombres de otros integrantes.
