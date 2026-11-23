# 1. Introducción y ambiente de desarrollo

## 1.1 Introducción

El Proyecto Grupal II de Compiladores e Intérpretes busca extender el compilador CDFF desarrollado previamente. Además de analizar un programa fuente, el compilador debe producir una representación intermedia de tres direcciones y aplicar transformaciones para mejorar el código, respetando el comportamiento original del programa. El trabajo se vincula con el procesador VLIW propio desarrollado en Arquitectura de Computadores I.

Las transformaciones estudiadas son **loop unrolling**, **eliminación de código muerto**, **renombramiento y reordenamiento seguro** y **spilling de registros**. Se pretende que estas técnicas faciliten la ejecución eficiente y la calendarización estática de instrucciones, conservando las dependencias necesarias para el procesador.

Este documento explica la organización del software, las decisiones observables en la implementación, las instrucciones de uso, las pruebas disponibles y las limitaciones que deben resolverse antes de la defensa final. No presupone que todas las funciones estén integradas.

## 1.2 Objetivos

**Objetivo general.** Desarrollar y validar un compilador con representación intermedia y optimizaciones que permita evaluar su efecto sobre código compatible con la arquitectura VLIW propia.

**Objetivos específicos.**

1. Conservar el análisis léxico, sintáctico y semántico del lenguaje CDFF y generar IR de tres direcciones.
2. Construir bloques básicos y un grafo de flujo de control (CFG) para sustentar análisis de dependencias y de variables vivas.
3. Implementar transformaciones IR con comprobaciones de seguridad y pruebas de equivalencia de resultados.
4. Integrar un IDE con autocompletado, detección de errores, correcciones conservadoras y detección de clones Tipo I y Tipo II.
5. Validar la emisión de ensamblador y bundles VLIW y, cuando corresponda, comparar las métricas solicitadas por el enunciado.

## 1.3 Ambiente y herramientas

El código fue estructurado para ejecutarse en GNU/Linux, incluido **Ubuntu desde WSL**. El entorno exacto, versiones utilizadas y equipos de cada integrante deben agregarse al cierre.

| Herramienta | Aplicación en el repositorio |
|---|---|
| C y GCC | Compilador, análisis de IR y utilidades auxiliares del IDE. |
| Flex | Analizador léxico a partir de `lexer.l`. |
| Bison | Analizador sintáctico y acciones semánticas en `parser.y`. |
| GNU Make | Construcción de `cdff`, ejecutables auxiliares y pruebas. |
| Python 3 | Servidor HTTP del IDE, scripts de empaquetado y validación. |
| HTML, CSS y JavaScript | Interfaz gráfica del IDE. |
| LL(1) tabular en C | Predicción contextual de completado y propuestas de corrección. |
| Git | Control de versiones y colaboración mediante ramas. |
| Graphviz | Visualización opcional del AST. |
| SystemVerilog e Icarus Verilog | Validaciones que requieren el repositorio de Arquitectura. |

Instalación de dependencias de compilación desde Ubuntu/WSL:

```bash
sudo apt update
sudo apt install -y build-essential flex bison make python3 graphviz
```

Para ejecutar simulaciones RTL, también se pueden requerir `iverilog`, `gtkwave` y el repositorio correspondiente a Arquitectura.

## 1.4 Estructura del software

```text
CompiladoresIIS2026/
├── README.md                  # Documentación histórica del Proyecto I
├── ide/
│   ├── app.py                 # Servidor local y endpoints HTTP
│   ├── index.html             # Editor web
│   ├── ll1_predict.c          # Predicción LL(1) tabular
│   ├── ide_syntax.c           # Diagnósticos y correcciones
│   ├── ide_completion.c       # Autocompletado contextual
│   ├── ide_clones.c           # Detección de clones
│   └── tests/                 # Pruebas del IDE
└── proyecto-1/
    ├── docs/
    │   └── proyecto-2/       # Esta documentación
    └── src/
        ├── lexer.l / parser.y
        ├── ast.c / semantico.c / tabla_simbolos.c
        ├── ir.c / ir_ast.c / basic_blocks.c
        ├── liveness.c / dead_code_elimination.c
        ├── loop_unrolling.c
        ├── dependencias.c / dag.c
        ├── renombramiento.c / reordenamiento.c
        ├── grafo_interferencia.c / asignacion_registros.c
        ├── spill_transform.c / optimizaciones_ir.c
        ├── codegen.c / codegen_ir.c
        ├── scheduler.c / vliw_packer.c / binario.c
        └── pruebas/           # Pruebas unitarias y de integración local
```

Los componentes que vienen del Proyecto I se conservan y amplían. El IDE es una interfaz separada que utiliza el ejecutable `cdff` para compilar y herramientas propias en C para completar y analizar código.

## 1.5 Límites de este avance

La revisión del 22 de noviembre de 2026, con archivos empaquetados a las 01:15 UTC del 23 de noviembre, incorpora las **cinco fases IR** dentro de `opt_ir_aplicar_config()`: loop unrolling, DCE, renombramiento, reordenamiento y spilling. El backend IR ya contiene traducción de los accesos internos de spilling, pero aún hace falta un caso reproducible que compruebe las cinco fases sobre el mismo programa fuente hasta un `.bin` ejecutado en el procesador VLIW. Las pruebas modulares sí se ejecutaron y se registran en [04 — Uso y validación](04_USO_Y_VALIDACION.md). Ver [07 — Pendientes](07_PENDIENTES.md).
