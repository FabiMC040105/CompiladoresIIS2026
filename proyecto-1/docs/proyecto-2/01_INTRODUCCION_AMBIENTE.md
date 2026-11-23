# 1. Introducción y ambiente de desarrollo

## 1.1 Introducción

El Proyecto Grupal II del curso Compiladores e Intérpretes consiste en ampliar el compilador CDFF desarrollado durante la primera etapa del curso. En esta segunda parte, el objetivo no es únicamente traducir un programa fuente, sino también analizar su representación intermedia y aplicar optimizaciones que permitan aprovechar mejor los recursos del procesador.

El proyecto se desarrolla en conjunto con los conocimientos adquiridos en Arquitectura de Computadores I, donde se trabaja con un procesador VLIW (*Very Long Instruction Word*). Esta arquitectura permite agrupar varias operaciones en una misma instrucción larga, siempre que no existan dependencias que impidan su ejecución simultánea. Por esta razón, el compilador debe identificar las relaciones entre instrucciones y organizar el código de manera que se aproveche el paralelismo disponible sin alterar los resultados del programa.

Para lograrlo, se incorporan distintas técnicas de optimización. Entre ellas se encuentran *loop unrolling*, eliminación de código muerto, renombramiento de registros, reordenamiento seguro de instrucciones y *spilling*. Cada técnica atiende una necesidad diferente: reducir el trabajo asociado con los ciclos, eliminar operaciones innecesarias, manejar dependencias, mejorar la organización de las instrucciones y administrar la disponibilidad de registros.

Uno de los principales retos del proyecto es asegurar que las transformaciones realizadas sobre el código intermedio conserven el comportamiento original. Una optimización no puede considerarse correcta solamente porque reduzca la cantidad de instrucciones o mejore alguna métrica; también debe producir los resultados esperados y respetar las restricciones de la arquitectura.

En este documento se presenta la organización del compilador, el ambiente utilizado para su desarrollo, las decisiones de implementación, las pruebas realizadas y los aspectos que todavía requieren revisión. También se describe la relación entre el compilador y el procesador VLIW, ya que esta integración es fundamental para evaluar el funcionamiento del proyecto.

## 1.2 Objetivos

### Objetivo general

Ampliar el compilador CDFF mediante la implementación de técnicas de optimización sobre una representación intermedia de tres direcciones, con el propósito de generar código compatible con el procesador VLIW desarrollado en Arquitectura de Computadores I y evaluar la corrección y el efecto de las transformaciones aplicadas.

### Objetivos específicos

1. Mantener las etapas de análisis léxico, sintáctico y semántico del compilador CDFF, incorporando la generación de una representación intermedia de tres direcciones que facilite el análisis y la transformación del código.

2. Identificar los bloques básicos y construir el grafo de flujo de control (CFG) para analizar las relaciones entre instrucciones, las dependencias de datos y la vida de las variables.

3. Implementar técnicas de optimización como *loop unrolling*, eliminación de código muerto, renombramiento, reordenamiento seguro y *spilling*, considerando las condiciones necesarias para conservar el comportamiento de los programas.

4. Integrar las funcionalidades del compilador con un entorno de desarrollo que permita escribir código, visualizar errores, recibir sugerencias de autocompletado e identificar posibles fragmentos duplicados de Tipo I y Tipo II.

5. Generar código ensamblador y su representación binaria para la arquitectura VLIW, utilizando pruebas funcionales y métricas que permitan evaluar el comportamiento del código original y optimizado.

6. Documentar las decisiones de diseño, los procedimientos de validación, los resultados obtenidos y las limitaciones identificadas durante el desarrollo.

## 1.3 Ambiente y herramientas de desarrollo

El desarrollo del proyecto se realiza principalmente en un ambiente GNU/Linux, utilizando Ubuntu mediante Windows Subsystem for Linux (WSL). Este entorno permite trabajar con las herramientas de compilación, ejecutar pruebas desde la terminal y mantener una estructura de archivos compatible con el flujo de desarrollo del proyecto.

El compilador utiliza C para implementar sus componentes principales. Flex y Bison se emplean en las etapas de análisis léxico y sintáctico, mientras que las estructuras relacionadas con la representación intermedia, las optimizaciones y la generación de código se desarrollan mediante módulos propios.

Para la interfaz del IDE se utilizan HTML, CSS y JavaScript, junto con Python 3 para el servidor local. Además, se incorporan componentes escritos en C que permiten realizar análisis complementarios del código fuente.

Las principales herramientas utilizadas son las siguientes:

| Herramienta | Función dentro del proyecto |
|---|---|
| C y GCC | Implementación y compilación de los módulos principales del compilador. |
| Flex | Reconocimiento de tokens y análisis léxico mediante `lexer.l`. |
| Bison | Análisis sintáctico y procesamiento de las reglas gramaticales mediante `parser.y`. |
| GNU Make | Automatización de la compilación y ejecución de pruebas. |
| Python 3 | Servidor local del IDE y ejecución de scripts de apoyo y validación. |
| HTML, CSS y JavaScript | Desarrollo de la interfaz gráfica del IDE. |
| LL(1) tabular en C | Apoyo al autocompletado contextual y a las propuestas de corrección sintáctica. |
| Git y GitHub | Control de versiones, organización del trabajo y colaboración entre integrantes. |
| Graphviz | Generación opcional de representaciones gráficas del árbol de sintaxis abstracta. |
| SystemVerilog e Icarus Verilog | Compilación y simulación de componentes RTL del procesador VLIW. |

Para preparar el ambiente de compilación en Ubuntu o WSL se pueden instalar las dependencias principales mediante:

```bash
sudo apt update
sudo apt install -y build-essential flex bison make python3 graphviz
```

En el caso de las pruebas relacionadas con la arquitectura, también se requiere disponer de Icarus Verilog y del repositorio del procesador. GTKWave puede utilizarse cuando sea necesario inspeccionar las señales generadas durante las simulaciones.

## 1.4 Estructura del software

El repositorio se organiza en diferentes directorios para separar la interfaz del IDE, los componentes internos del compilador, las pruebas y la documentación. Esta división facilita el trabajo entre integrantes y permite realizar modificaciones en módulos específicos sin concentrar toda la implementación en un único archivo.

La estructura general es la siguiente:

```text
CompiladoresIIS2026/
├── README.md
├── ide/
│   ├── app.py
│   ├── index.html
│   ├── ll1_predict.c
│   ├── ide_syntax.c
│   ├── ide_completion.c
│   ├── ide_clones.c
│   └── tests/
└── proyecto-1/
    ├── docs/
    │   └── proyecto-2/
    └── src/
        ├── lexer.l
        ├── parser.y
        ├── ast.c
        ├── semantico.c
        ├── tabla_simbolos.c
        ├── ir.c
        ├── ir_ast.c
        ├── basic_blocks.c
        ├── liveness.c
        ├── dead_code_elimination.c
        ├── loop_unrolling.c
        ├── dependencias.c
        ├── dag.c
        ├── renombramiento.c
        ├── reordenamiento.c
        ├── grafo_interferencia.c
        ├── asignacion_registros.c
        ├── spill_transform.c
        ├── optimizaciones_ir.c
        ├── codegen.c
        ├── codegen_ir.c
        ├── scheduler.c
        ├── vliw_packer.c
        ├── binario.c
        └── pruebas/
```

El directorio `ide/` contiene los componentes necesarios para utilizar el compilador mediante una interfaz web. Desde esta interfaz se puede trabajar con el código fuente y acceder a las funcionalidades de análisis y compilación disponibles.

Por su parte, `proyecto-1/src/` reúne los módulos responsables del procesamiento del lenguaje CDFF. Estos abarcan desde el reconocimiento de tokens hasta la generación de código para la arquitectura de destino.

Dentro de este directorio también se encuentran los módulos asociados con las optimizaciones. Su separación permite estudiar y probar cada transformación individualmente antes de evaluar su comportamiento al combinarse con las demás etapas.

Finalmente, el directorio `proyecto-1/docs/proyecto-2/` contiene la documentación correspondiente a esta segunda etapa, incluyendo los diagramas, las explicaciones de las optimizaciones, las instrucciones de uso, las pruebas y el registro de avances.

## 1.5 Estado de implementación y alcance de la validación

Al cierre de esta revisión, el compilador cuenta con módulos para las cinco fases de optimización contempladas en el proyecto: *loop unrolling*, eliminación de código muerto, renombramiento, reordenamiento y *spilling*. Estas fases se integran mediante la función `opt_ir_aplicar_config()`, que permite aplicar las transformaciones sobre la representación intermedia.

También se dispone de componentes para generar código ensamblador y binario orientado a la arquitectura VLIW. Sin embargo, es importante distinguir entre la implementación de una funcionalidad y su validación completa, ya que ambas forman parte del proceso de desarrollo.

En las pruebas locales se obtuvieron 34 casos semánticos aprobados. Asimismo, se ejecutaron 15 casos de evaluación funcional y se generaron archivos de resultados y métricas. Estas pruebas permitieron identificar comportamientos correctos y aspectos que todavía requieren revisión, especialmente en los casos relacionados con *spilling* y en el efecto de algunas transformaciones sobre los ciclos de ejecución.

En cuanto a la integración con el procesador, se logró compilar los testbenches RTL utilizados para la verificación. No obstante, la ejecución secuencial de la prueba FEIST alcanzó el límite de tiempo establecido sin confirmar la finalización esperada. Por lo tanto, esta prueba todavía no puede considerarse aprobada.

Estos resultados permiten identificar el avance alcanzado y, al mismo tiempo, establecer cuáles son las verificaciones necesarias antes de dar por concluida la integración. El propósito de la validación no es únicamente comprobar que el compilador genera archivos de salida, sino verificar que las transformaciones conservan el comportamiento de los programas y que el código producido puede ejecutarse correctamente en la arquitectura de destino.
Los procedimientos de prueba y sus resultados se desarrollan con mayor detalle en [04 — Uso y validación](04_USO_Y_VALIDACION.md). Los aspectos pendientes se presentan en [07 — Pendientes](07_PENDIENTES.md).
