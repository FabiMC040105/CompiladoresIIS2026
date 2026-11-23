# 7. Pendientes y comprobaciones antes de entregar

## 7.1 Comprobaciones prioritarias en la WSL del grupo

- [ ] Ejecutar `make rebuild` con Flex/Bison disponibles y registrar el hash real de la revisión.
- [ ] Ejecutar `make test-entrega-local` y archivar su salida completa.
- [ ] Ejecutar `make test-suite-optimizaciones`, verificar **15/15** y revisar las cifras y gráficos generados: no confundir «suite creada» con «suite validada».
- [ ] Ejecutar `make test-spill-obligatorio` y comprobar `STORE/LOAD`, slots, `.asm`, `.bin` y, si corresponde, ejecución funcional completa.
- [ ] Ejecutar `make test-importaciones`, incluido el caso negativo de archivo inexistente.
- [ ] Ejecutar `make test-rtl-actual` contra la rama de Arquitectura que realmente contenga el `top.sv` requerido y sus contadores. Revisar `rtl.log` y valor esperado **35**.
- [ ] Validar que la transformación de las cinco fases se aplique **cuando corresponda** al programa elegido. Activar una bandera no significa que efectivamente transforme algo.
- [ ] Revisar si `--codegen-ir` resuelve correctamente los alcances locales, parámetros, arreglos y recursividad en los casos de defensa. Conservar tests de regresión.
- [ ] Corregir el texto desactualizado de `--unroll N` en `parser.y`, que aún indica que requiere `--ir-cfg`.
- [ ] Revisar la descripción del script `compilar_integracion_3_opt.sh` porque el comando activa cuatro banderas de optimización.
- [ ] Verificar y documentar el comportamiento de NOPs y los hazards RAW entre bundles, sin suponer forwarding ni scoreboarding dinámico.

## 7.2 Evidencia cuantitativa y análisis

- [ ] Añadir a la entrega real el reporte `proyecto-1/docs/metricas-optimizaciones/reporte.html` y el CSV que genere el equipo, si su ejecución termina correctamente.
- [ ] Discutir casos que aumenten el tamaño de código o el tiempo de compilación: no todas las optimizaciones reducen todos los indicadores.
- [ ] Distinguir **ciclos estimados por `simular_asm_ir.py`** de **ciclos reales de la simulación RTL con cachés**. No presentarlos como el mismo resultado.
- [ ] En las tres pruebas de spilling, aclarar que la línea base es `--codegen-ir-mem`, no la compilación estándar que utiliza Chaitin.
- [ ] Guardar el registro de versiones de herramientas y commits usados.

## 7.3 Elementos de documentación aún por completar

- [ ] Bitácora con nombres, actividades, fechas y horas reales de cada integrante.
- [ ] Enlace de repositorio, branch/commit final y evidencia de integración.
- [ ] Capturas del IDE en funcionamiento, si las solicitan durante la defensa.
- [ ] Revisar y confirmar las conclusiones con los resultados cuantitativos verdaderos.
- [ ] Corregir cualquier diferencia de rutas al pasar de la rama `fix-integracion-final` a `development`/`main`.

## 7.4 Estado observado en esta revisión

| Subsistema | Código / pruebas en ZIP | Validación realizada aquí |
|---|---|---|
| Lexer/Parser/semántica | Fuentes presentes | No se reconstruyó `cdff` por ausencia de Flex/Bison. |
| IR, CFG, DCE, renombramiento, reordenamiento, unrolling | Implementación y tests | **PASS** pruebas modulares ejecutadas. |
| Grafo de interferencia, coloreo y rewriting de spill | Implementación y tests | **PASS** pruebas modulares ejecutadas. |
| Backend IR y asignación Chaitin | Código y script backend | Script E2E no ejecutado con `cdff` nuevo. |
| 15 programas obligatorios | **15 `.e`** (tres de cada tipo) + runner de métricas | Existencia y lógica del runner revisadas; sin PASS de ejecución de los 15. |
| 3 pruebas de spilling `.e` con ASM/binario | Presentes | No ejecutadas sin `cdff` nuevo. |
| Importaciones entre archivos | Presentes | No ejecutadas sin `cdff` nuevo. |
| IDE | Frontend, backend y test | **58 Python + 6 JS PASS**. |
| Integración de compilador y RTL CE4301 | Script y testbench presentes | Sin ejecución RTL en esta revisión. |
