# 7. Pendientes, riesgos y lista de validación

La implementación del ZIP nuevo incorporó la integración de unrolling y el soporte de memoria para spilling IR. Estos puntos **ya no deben describirse como funcionalidades ausentes**. Queda pendiente demostrar su funcionamiento completo en los escenarios exigidos por la especificación.

## 7.1 Validación técnica de extremo a extremo

- [ ] Ejecutar `make rebuild` en la WSL del equipo con Flex y Bison instalados; registrar resultado y versión de herramientas.
- [ ] Corregir la ayuda de `--unroll` en `parser.y`: ya no requiere `--ir-cfg`.
- [ ] Compilar un programa `.e` con `--codegen-ir --unroll 4`, comprobar que hay un ciclo realmente desenrollado y validar el valor ejecutado, no solo que aparezca la opción.
- [ ] Compilar un programa con presión suficiente para crear `cdff_spill_area`, comprobar direcciones, `LOAD/STORE`, integridad de memoria y equivalencia de resultados frente a una referencia sin optimizar.
- [ ] Ejecutar un programa que ejercite efectivamente **las cinco fases** de `--opt-ir`, confirmando sus contadores y comparando el resultado con el código sin optimizar.
- [ ] Verificar si el backend IR resuelve correctamente los **ámbitos locales y parámetros** de programas reales complejos. El parser configura `codegen_resolver_ir_global` y los tests independientes inyectan un resolvedor específico: ambos hechos no garantizan la misma cobertura.
- [ ] Comprobar dependencias, calendarización y NOPs entre bundles en el procesador VLIW. No asumir forwarding ni scoreboarding del hardware.
- [ ] Ejecutar binarios optimizados en la simulación RTL con cachés L1-D/L2 y comparar resultados con un oráculo.
- [ ] Comprobar que el script `probar_codegen_ir_backend.sh` puede ejecutarse mediante `make test-codegen-ir-backend` o documentar la invocación por `bash`.
- [ ] Actualizar comentarios de `parser.y`, `optimizaciones_ir.h` y `spill_transform.h` que todavía describen como futuras o no conectadas partes de la integración ya implementadas.

## 7.2 Casos mínimos que exige el profesor

La sección 6 del enunciado pide **al menos 15 programas representativos**, no solamente 15 aserciones dentro de pruebas C:

- [ ] Tres para Loop Unrolling: N constante, N variable pequeño y N grande.
- [ ] Tres con dependencias WAR/WAW que requieran renombramiento.
- [ ] Tres con código muerto mezclado con llamadas con efectos.
- [ ] Tres con saltos condicionales y funciones para reordenamiento seguro.
- [ ] Tres que verifiquen spilling de registros.

Para cada programa se debe registrar entrada, salida esperada, comando de ejecución, resultado, evidencia y métricas. La cobertura actual de pruebas modulares es una base, pero todavía hay que vincularla a este conjunto evaluable.

## 7.3 Documentación y evidencia cuantitativa

- [ ] Obtener conteos antes/después, bytes, ciclos estimados, tiempo de compilación y número de instrucciones eliminadas/reordenadas/renombradas.
- [ ] Generar gráficas comparativas. Si se usan métricas de RTL, indicar configuración y versión del procesador.
- [ ] Completar bitácora real de los cuatro integrantes, con fechas, actividades y total de horas.
- [ ] Completar nombres de autores, enlace del repositorio y hash del commit final de `main`.
- [ ] Revisar los diagramas contra la integración final de hardware y software.
- [ ] Preparar demostración final del IDE y compilación optimizada por terminal, distinguiendo sus rutas.

## 7.4 Matriz de estado a la fecha del ZIP

| Componente | Estado observado | Evidencia / reserva |
|---|---|---|
| AST, IR y CFG | Implementados | Código y pruebas existentes. |
| DCE | Implementado y probado modularmente | Pruebas de regresión PASS. |
| Renombramiento | Implementado y probado modularmente | Pruebas PASS. |
| Reordenamiento | Implementado y probado modularmente | Pruebas PASS. |
| Loop Unrolling | Conectado al coordinador IR | Pruebas aisladas PASS; falta demostrar todo el recorrido CLI → RTL. |
| Spilling IR | Conectado al coordinador IR | Grafo, recoloreo y reescritura PASS. |
| Spilling al ensamblador IR | **Código incorporado** | `codegen_ir.c` detecta slots y emite LOAD/STORE; falta prueba dedicada integrada. |
| Backend IR general | Pruebas autónomas PASS | Diez escenarios en simulador ASM; no representan RTL con cachés. |
| IDE y LL(1) | Pruebas automáticas PASS | 49 pruebas Python ejecutadas. |
| Compilación completa del frontend | No comprobada en esta revisión | Entorno de análisis sin `bison`/`flex`. |
| Benchmarks comparativos | Pendientes | Sin resultados medidos provistos para esta actualización. |

## 7.5 Observación sobre `--opt-ir`

`OPT_IR_TODAS` ya contempla las cinco fases. Sin embargo, varias pruebas existentes invocan el coordinador con un programa **sin ciclos reconocibles**, por lo que superar la prueba no certifica que ocurra un desenrollamiento efectivo. También se deben diferenciar casos donde spilling no fue necesario de los que obligan a insertar `LOAD/STORE`. La defensa debe demostrar al menos una transformación real de cada tipo.
