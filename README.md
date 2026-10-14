# CompiladoresIIS2026 — Proyecto 1

## Descripción del proyecto

Proyecto desarrollado para el curso de **Compiladores e Intérpretes** del Instituto Tecnológico de Costa Rica (TEC), correspondiente al segundo semestre de 2026.

Consiste en la implementación de un compilador para un lenguaje de programación propio, denominado CDFF, capaz de procesar código fuente, realizar análisis léxico, sintáctico y semántico, generar código ensamblador y producir instrucciones de máquina compatibles con una arquitectura VLIW desarrollada para el curso de Arquitectura de Computadores I.

El proyecto integra las distintas etapas de compilación con la generación de instrucciones de 128 bits, permitiendo ejecutar programas en la arquitectura diseñada por el equipo.

## Tecnologías utilizadas

- **C:** implementación de los componentes principales del compilador.
- **Flex:** generación del analizador léxico.
- **Bison:** generación del analizador sintáctico.
- **GCC:** compilación de los archivos fuente en C.
- **GNU Make:** automatización de compilación y pruebas.
- **Python 3:** herramientas auxiliares de empaquetamiento y validación.
- **Graphviz:** visualización del árbol de sintaxis abstracta (AST).
- **SystemVerilog:** implementación y verificación de la arquitectura.
- **Icarus Verilog:** simulación del hardware.
- **GTKWave:** visualización de señales durante las simulaciones.
- **Git y GitHub:** control de versiones y colaboración.

## Funcionalidades implementadas

El compilador incorpora las siguientes etapas:

1. **Análisis léxico:** identificación de palabras reservadas, identificadores, operadores, constantes y demás componentes del lenguaje.
2. **Análisis sintáctico:** verificación de las estructuras gramaticales mediante Bison.
3. **Árbol de sintaxis abstracta (AST):** representación estructurada de los programas.
4. **Análisis semántico:** verificación de tipos, declaraciones, ámbitos, funciones y operaciones.
5. **Tabla de símbolos:** administración de variables, funciones, tipos y direcciones de memoria.
6. **Generación de ensamblador:** traducción de las estructuras del lenguaje a instrucciones de la arquitectura.
7. **Resolución de etiquetas y saltos:** cálculo de referencias y desplazamientos en el ensamblador.
8. **Calendarización de instrucciones:** consideración de dependencias entre instrucciones.
9. **Empaquetamiento VLIW:** agrupación de operaciones compatibles en paquetes de instrucciones.
10. **Generación de código máquina:** codificación de las instrucciones en paquetes de 128 bits.

## Arquitectura VLIW

El compilador genera código compatible con una arquitectura **Very Long Instruction Word (VLIW)** de 128 bits.

Cada paquete de instrucciones (*bundle*) contiene cuatro slots de 32 bits:

| Unidad | Tamaño | Función |
|---|---|---|
| ALU | 32 bits | Operaciones aritméticas y lógicas |
| LSU | 32 bits | Acceso a memoria |
| BRU | 32 bits | Saltos y control de flujo |
| FEIST | 32 bits | Operaciones criptográficas |

Los slots que no contienen una operación se representan mediante ceros.

La representación hexadecimal de cada paquete utiliza 32 dígitos hexadecimales, siguiendo el orden:

`FEIST | BRU | LSU | ALU`

El empaquetamiento permite aprovechar el paralelismo entre operaciones independientes, respetando las dependencias y restricciones de la arquitectura.

## Estructura del repositorio

```text
CompiladoresIIS2026/
├── LICENSE
├── README.md
└── proyecto-1/
    ├── docs/
    └── src/
        ├── Makefile
        ├── lexer.l
        ├── parser.y
        ├── ast.c / ast.h
        ├── semantico.c / semantico.h
        ├── tabla_simbolos.c / tabla_simbolos.h
        ├── memoria.c / memoria.h
        ├── codegen.c / codegen.h
        ├── codegen_tabla.c / codegen_tabla.h
        ├── asm_emitter.c / asm_emitter.h
        ├── scheduler.c / scheduler.h
        ├── etiquetas.c / etiquetas.h
        ├── resolucion_etiquetas.c / .h
        ├── binario.c / binario.h
        ├── vliw_packer.c / .h
        ├── pruebas/
        └── archivos de prueba y validación
```

## Requisitos de instalación

Se recomienda utilizar **Ubuntu o WSL con Ubuntu**.

Para instalar las herramientas principales:

```bash
sudo apt update
sudo apt install -y build-essential flex bison python3
```

Para las funciones adicionales de visualización y simulación:

```bash
sudo apt install -y graphviz iverilog gtkwave
```

## Compilación del proyecto

Clonar el repositorio:

```bash
git clone https://github.com/FabiMC040105/CompiladoresIIS2026.git
```

Ingresar al directorio del proyecto:

```bash
cd CompiladoresIIS2026/proyecto-1/src
```

Compilar utilizando el Makefile:

```bash
make
```

Este comando genera los ejecutables principales:

- `cdff`: compilador del lenguaje.
- `resolver_asm`: herramienta auxiliar para resolución de etiquetas.

Para realizar una compilación desde cero:

```bash
make rebuild
```

## Ejecución del compilador

Para compilar un programa de ejemplo:

```bash
make run FILE=factorial.e
```

También es posible ejecutar directamente el compilador:

```bash
./cdff factorial.e
```

Para visualizar información adicional durante la compilación:

```bash
make run FILE=factorial.e ARGS="-v -m"
```

Para conservar el ensamblador generado:

```bash
make run FILE=factorial.e ARGS="-s"
```

Para especificar el nombre del archivo de salida:

```bash
make run FILE=factorial.e ARGS="-o resultado.bin"
```

Para imprimir los paquetes generados:

```bash
make run FILE=factorial.e ARGS="-p"
```

### Opciones de línea de comandos

| Opción | Descripción |
|---|---|
| `-v` | Muestra información detallada de las fases de compilación |
| `-t` | Imprime el AST y genera `arbol.dot` |
| `-m` | Muestra la tabla de símbolos, ámbitos y firmas |
| `-o` | Especifica el archivo de salida |
| `-s` | Conserva el ensamblador final |
| `-x` | Ensambla directamente un archivo `.asm` |
| `-p` | Muestra los paquetes en ensamblador, hexadecimal y binario |
| `-h`, `--help` | Muestra la ayuda del compilador |

Para consultar todas las opciones:

```bash
./cdff --help
```

## Generación de código máquina

El compilador produce archivos `.bin` con la representación hexadecimal de los paquetes VLIW.

Cada línea contiene un paquete de **128 bits**, expresado mediante 32 caracteres hexadecimales.

También es posible ensamblar directamente un archivo `.asm`:

```bash
./cdff -x demo_vliw.asm -o resultado.bin
```

Esto permite comprobar la codificación de instrucciones independientemente de las etapas de análisis del lenguaje fuente.

## Visualización del AST

Para generar y visualizar el árbol de sintaxis abstracta:

```bash
make arbol FILE=factorial.e
```

El comando utiliza Graphviz para generar `arbol.png` a partir de `arbol.dot`.

## Pruebas y validación

El proyecto incluye pruebas individuales y de integración para comprobar el comportamiento de sus componentes.

### Pruebas automatizadas del Makefile

```bash
make test
```

Este comando ejecuta las pruebas de etiquetas, resolución de saltos, codificación binaria y empaquetamiento VLIW.

También pueden ejecutarse individualmente:

```bash
make test-etiquetas
make test-resolucion
make test-binario
make test-vliw
```

### Pruebas semánticas

Para ejecutar la batería de 34 pruebas semánticas:

```bash
bash probar_semantico.sh
```

Estas pruebas incluyen declaraciones, tipos de datos, ámbitos, funciones, retornos, ciclos, listas, matrices y manejo de errores.

### Validaciones de integración con Arquitectura

Se incluyen scripts para validar la compatibilidad entre el código generado y la arquitectura implementada en SystemVerilog.

Por ejemplo:

```bash
bash validar_feist_4r.sh
```

Esta validación requiere disponer también del proyecto de Arquitectura y de las herramientas de simulación correspondientes.

Cuando el repositorio de Arquitectura se encuentre en otra ubicación, se puede especificar mediante la variable `ARQ_ROOT`.

Las pruebas de integración permiten verificar el comportamiento de las instrucciones, el empaquetamiento VLIW y los resultados observados durante la simulación.

## Comandos disponibles del Makefile

| Comando | Descripción |
|---|---|
| `make` | Compila los ejecutables principales |
| `make all` | Compila el proyecto |
| `make run FILE=archivo.e` | Ejecuta el compilador sobre un programa |
| `make rebuild` | Limpia y recompila el proyecto |
| `make clean` | Elimina ejecutables y archivos intermedios definidos en el Makefile |
| `make test` | Ejecuta las pruebas automatizadas principales |
| `make test-etiquetas` | Prueba el manejo de etiquetas |
| `make test-resolucion` | Prueba la resolución de saltos |
| `make test-binario` | Prueba la codificación binaria |
| `make test-vliw` | Prueba el empaquetamiento VLIW |
| `make arbol FILE=archivo.e` | Genera y visualiza el AST |
| `make resolver ENTRADA=a.asm SALIDA=b.asm` | Resuelve etiquetas de un ensamblador |
| `make help` | Muestra la ayuda del Makefile |

## Ejemplo de programa

El lenguaje utiliza palabras reservadas propias y el delimitador `:)` para finalizar instrucciones.

```text
defina vacio principal() {
    entero x = 5 :)
    entero y = 10 :)
    entero resultado = x + y :)
}:)
```

El archivo puede guardarse con extensión `.e` y compilarse mediante:

```bash
./cdff programa.e
```

## Licencia

Este repositorio incluye una licencia MIT, disponible en el archivo [LICENSE](LICENSE).

---

**Instituto Tecnológico de Costa Rica — II Semestre 2026**

Proyecto 1 — Compiladores e Intérpretes

Integración con Arquitectura de Computadores I.
