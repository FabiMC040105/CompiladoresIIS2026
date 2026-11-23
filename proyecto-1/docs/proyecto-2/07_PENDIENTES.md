# 7. Pendientes y comprobaciones antes de entrega

Este listado distingue el funcionamiento que existe como **módulo/prototipo probado** de los requisitos de la entrega **totalmente integrada**. No sustituye una revisión del grupo sobre la última versión de `main`.

## 7.1 Integración técnica

- [ ] **Loop unrolling al backend:** `--unroll N` actualmente requiere `--ir-cfg`, y esa ruta no genera ASM final. Conectar la transformación con la ruta de código máquina y verificar el resultado.
- [ ] **Spilling IR al backend:** `--codegen-ir` rechaza `OPT_IR_SPILL`; implementar el descenso de `cdff_spill_area` a operaciones y direcciones de memoria válidas.
- [ ] **Prueba completa de `--opt-ir`:** la máscara solicita DCE + renombramiento + reordenamiento + spilling. Demostrar un programa real que pase por todas y termine en binario ejecutable.
- [ ] **Backend IR y ámbitos:** `parser.y` establece `codegen_resolver_ir_global`, que resuelve variables globales; validar y ampliar los casos de variables locales y parámetros antes de afirmar compatibilidad total.
- [ ] **Integración con VLIW:** ejecutar el binario optimizado en la implementación real del procesador, no solo en pruebas de IR ni en el simulador abstracto de ASM.
- [ ] **Calendarización y NOPs:** verificar el manejo de dependencias RAW y de control entre bundles, sin atribuir al hardware forwarding o scoreboarding inexistentes.
- [ ] **Corregir/comprobar permiso del script:** si `make test-codegen-ir-backend` falla por `Permission denied`, revisar `chmod +x` y el bit de ejecución registrado en Git.

## 7.2 Casos mínimos del enunciado

La sección 6 solicita, al menos:

- [ ] Tres programas de loop unrolling: N constante, N variable pequeño y N grande.
- [ ] Tres programas con dependencias que obliguen a renombramiento WAR/WAW.
- [ ] Tres programas con código muerto mezclado con llamadas con efectos.
- [ ] Tres programas con saltos condicionales y funciones para comprobar reordenamiento seguro.
- [ ] Tres programas que verifiquen spilling de registros.
- [ ] Verificar que esos escenarios estén cubiertos mediante **programas representativos** y que puedan demostrarse aisladamente. Tener numerosas aserciones en una prueba C no demuestra por sí solo los programas de entrada solicitados.

## 7.3 Evidencia cuantitativa y documentación

- [ ] Medir por prueba instrucciones antes/después, tamaño de código, ciclos estimados, tiempo de compilación y número de eliminadas/reordenadas/renombradas.
- [ ] Preparar gráficas y comparaciones entre configuraciones, sin inventar cifras.
- [ ] Contrastar el efecto de optimizaciones con métricas del procesador/cachés cuando la integración esté disponible.
- [ ] Completar la bitácora de los cuatro integrantes con actividades, fechas, horas reales y totales.
- [ ] Incorporar integrantes, repositorio y hash final; revisar ortografía y referencias.
- [ ] Actualizar tablas de estado y conclusiones tras corregir el backend.
- [ ] Exportar un PDF final si el formato de entrega por TEC Digital así lo requiere, conservando los diagramas y la pregunta/respuesta de atributos.

## 7.4 Riesgos conocidos del estado observado

| Hallazgo | Evidencia observada | Consecuencia documental |
|---|---|---|
| `--unroll` no llega a ASM final | Validación de flags y liberación del IR en `parser.y`. | Mostrarlo como transformación IR, no como binario optimizado. |
| `--codegen-ir` rechaza spilling | Mensaje explícito en `parser.y`. | No afirmar soporte end-to-end para `--opt-ir`. |
| IDE compila sin optimizaciones | Ruta `/api/compile` en `ide/app.py`. | No presentar clic en «Compilar» como prueba de optimizaciones activas. |
| Resultados de pruebas aisladas | Testbenches C, Python y script de simulación ASM. | Registrar pruebas por separado de la simulación RTL VLIW. |
| Falta medición comparativa consolidada | No se encontró una tabla de benchmark final en los archivos revisados. | Mantener tablas de rendimiento como pendientes. |

Para comprobar nuevos cambios, repetir las pruebas de [04 — Uso y validación](04_USO_Y_VALIDACION.md) y anotar el hash y la fecha de ejecución.
