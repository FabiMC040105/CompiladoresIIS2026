# 6. Bitácora, conclusiones y referencias

## 6.1 Bitácora de actividades por integrante

**Pendiente de completar con datos reales del grupo.** Las etiquetas siguientes son campos de registro, no nombres ni asignaciones de responsabilidades.

| Integrante | Fecha | Actividad realizada | Horas | Evidencia (commit, PR, prueba, issue) |
|---|---|---|---:|---|
| [Nombre 1] | [Fecha] | [Descripción] | [Horas] | [Enlace o hash] |
| [Nombre 2] | [Fecha] | [Descripción] | [Horas] | [Enlace o hash] |
| [Nombre 3] | [Fecha] | [Descripción] | [Horas] | [Enlace o hash] |
| [Nombre 4] | [Fecha] | [Descripción] | [Horas] | [Enlace o hash] |

| Resumen | Horas totales |
|---|---:|
| [Nombre 1] | [Total] |
| [Nombre 2] | [Total] |
| [Nombre 3] | [Total] |
| [Nombre 4] | [Total] |
| **Equipo** | **[Total real]** |

**Sugerencia:** consultar `git log --oneline --all --decorate` y las pull requests para reconstruir las actividades, pero verificar las horas con cada integrante. Los commits por sí solos no permiten estimar duración del trabajo.

## 6.2 Conclusiones provisionales

1. La representación intermedia de tres direcciones proporciona una base común para analizar el flujo del programa y aplicar transformaciones de manera independiente al código máquina.
2. Las pruebas locales del avance recibido muestran que las implementaciones de DCE, renombramiento, reordenamiento, spilling y loop unrolling superan los casos incluidos en sus baterías. Esto demuestra funcionamiento en esos escenarios, no corrección universal.
3. La organización por módulos permite mantener separadas las fases de frontend, análisis IR, optimización y backend. El coordinador incluye loop unrolling y el backend IR materializa operaciones de spilling mediante LSU. En este avance se añadieron programas de extremo a extremo y una suite de 15 casos; siguen requiriendo validación reproducible sobre el compilador y el hardware compatibles.
4. El IDE cuenta con pruebas automáticas de su predictor LL(1), correcciones conservadoras, clones y autocompletado (58 tests Python y 6 verificaciones de frontend JS superadas en la revisión). La compilación final realizada por `cdff` sigue siendo una ruta independiente.
5. No es posible concluir todavía que todas las optimizaciones mejoran el IPC o reducen los ciclos del procesador real. Ya existe un runner para obtener comparaciones cuantitativas y un script para la integración RTL, pero en esta revisión no se generaron ni verificaron sus resultados de ejecución completa.

## 6.3 Sugerencias y recomendaciones

1. Agregar una prueba integrada que ejecute `--codegen-ir --unroll N` sobre un programa `.e`, comprobando el ASM, el `.bin` y el resultado calculado.
2. Validar los pseudoaccesos de spilling ya incorporados a `codegen_ir.c`, incluida la reserva de slots, sus direcciones y los `LOAD`/`STORE`, en un programa real con presión de registros.
3. Mantener pruebas de regresión por optimización, con casos de ciclos, llamadas, arreglos, funciones y dependencias entre bloques.
4. Ejecutar el conjunto de 15 benchmarks ya agregado, conservar su CSV/HTML/gráficas y analizar los resultados antes/después exigidos por la especificación.
5. Ejecutar una validación final en WSL y en el procesador VLIW con cachés, asegurando que el binario respete la ISA y que la simulación termine correctamente.
6. Actualizar la documentación conforme se integren las correcciones, incluyendo fecha y hash del commit usado para obtener cada resultado.

## 6.4 Referencias

1. Instituto Tecnológico de Costa Rica, Escuela de Ingeniería en Computadores. *Especificación Proyecto Grupal II: Análisis de Impacto de Memoria Caché y Optimizaciones de Compilador en la Organización de Procesador Propio tipo VLIW*. CE4301 y CE1108, II semestre 2026, 19 pp.
2. Repositorio del proyecto **CompiladoresIIS2026**, archivos de `proyecto-1/src/` y `ide/`, revisión del avance `fix-integracion-final` recibido el 22 de noviembre de 2026. [Agregar URL del repositorio y hash de `main` usado para entrega].
3. Documentación interna existente: `README.md`, `ide/README.md`, `ide/DEFENSA_LL1.md`, `ide/CORRECCIONES_Y_CLONES.md`, `proyecto-1/docs/CAMBIOS_MEMORIA_Y_SALTOS.md`.

**Nota:** agregar cualquier bibliografía externa efectivamente consultada por el grupo y normalizar el formato de las referencias según solicite el profesor.
