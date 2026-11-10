# Correcciones automáticas y detección de clones

Este documento describe cómo funciona el sistema de correcciones automáticas del IDE y el detector de clones Tipo I y Tipo II, y qué pruebas lo respaldan.

## Correcciones guiadas por LL(1)

Al principio las correcciones dependían de reglas puntuales del editor. Eso tenía un problema: podía proponer arreglos que la gramática no justificaba, o tocar código que ya estaba bien. Por eso movimos la decisión al analizador LL(1): el IDE solo propone una corrección si la gramática la respalda.

El proceso es el siguiente:

1. Se analiza el código con `ll1_analyze()`.
2. Si el programa es válido, no se propone nada.
3. Si hay un error, se obtienen los terminales que la gramática esperaba en ese punto.
4. `ide_syntax.c` genera candidatos de reparación usando únicamente esos terminales.
5. Cada candidato se aplica sobre una copia del código y se vuelve a analizar.
6. Solo se propone la corrección si el análisis de la copia mejora.
7. Si ningún candidato es suficientemente seguro, se devuelve `NO_FIX`.

El IDE maneja tres acciones:

- `LL1_INSERT`: inserta un símbolo que faltaba.
- `LL1_REPLACE`: reemplaza un símbolo incorrecto.
- `NO_FIX`: reporta el error sin modificar el código.

### Qué se corrige

Se corrigen delimitadores que la gramática permite deducir:

- el terminador de instrucción `:)`
- el paréntesis de cierre `)`
- el corchete de cierre `]`
- la llave de cierre `}`

Por ejemplo, con este código:

```text
entero x = 1
```

si la gramática indica que en ese punto se esperaba `:)`, el IDE genera un `LL1_INSERT` con ese símbolo.

### Qué no se corrige (`NO_FIX`)

Decidimos que el corrector fuera conservador y no inventara información. No se corrigen automáticamente:

- operandos o identificadores faltantes
- nombres de variables
- expresiones incompletas
- casos con varias reparaciones posibles
- errores semánticos
- cambios que no mejoren el análisis LL(1)

Por ejemplo:

```text
entero x = 1 +
```

Aquí la gramática espera un operando, pero no hay forma de saber cuál quería escribir el usuario. En lugar de inventar un número o un identificador, el resultado es `NO_FIX`.

### Código que ya es válido

El corrector no debe tocar código sintácticamente correcto. En esos casos la lista de reparaciones tiene que quedar vacía. Esto se prueba con programas completos, expresiones en varias líneas, comentarios y estructuras de control válidas.

## Cambios en el frontend

Se eliminó la ruta especial `APPEND_BLOCK`, que venía de la implementación anterior y se basaba en decisiones particulares del editor. Ahora el frontend solo trabaja con `LL1_INSERT`, `LL1_REPLACE` y `NO_FIX`, de modo que la decisión de qué corrección es válida queda en el analizador y no se repite en JavaScript.

También agregamos protección contra sugerencias obsoletas. Cuando se muestra una corrección, se guarda el contenido del código que generó el diagnóstico. Antes de aplicarla se revisa que:

- el código no haya cambiado desde que se calculó la sugerencia, y
- el texto que se va a reemplazar siga existiendo exactamente en la posición indicada.

Así evitamos que una corrección calculada para una versión anterior del archivo modifique código nuevo por accidente.

## Detección de clones

### Tipo I

Son fragmentos con los mismos tokens significativos que solo difieren en aspectos superficiales: espacios, tabulaciones, formato o comentarios. Dos fragmentos así deben seguir detectándose como clones Tipo I.

### Tipo II

Son fragmentos con la misma estructura sintáctica pero con identificadores y literales renombrados. Por ejemplo, estos dos fragmentos son clones Tipo II:

```text
entero x = 1 :)
entero y = 2 :)
x = x + y :)
```

```text
entero valor = 10 :)
entero total = 20 :)
valor = valor + total :)
```

El detector mantiene una correspondencia entre los identificadores de ambos fragmentos para comprobar que el renombramiento sea consistente.

### Falsos positivos

No todo código parecido es un clon, así que agregamos pruebas para evitar clasificaciones incorrectas. Se verifica que:

- un renombramiento inconsistente no se considere Tipo II
- una operación distinta no se considere clon
- los fragmentos demasiado pequeños no se reporten
- los rangos de líneas reportados coincidan con los fragmentos detectados
- la detección funcione entre archivos diferentes

Por ejemplo, cambiar `x = x + y :)` por `a = a - b :)` modifica un operador significativo, por lo que no se reporta como el mismo clon estructural.

## Pruebas

| Archivo | Qué valida |
|---|---|
| `ide/tests/test_clones.py` | Tipo I con cambios de espacios y comentarios, Tipo II con renombrado de identificadores y literales, consistencia del renombrado, falsos positivos, operadores distintos, fragmentos pequeños y rangos de líneas |
| `ide/tests/test_correcciones.py` | Falta de `:)`, `)` y `}`, múltiples errores corregibles, expresiones incompletas con `NO_FIX`, que no se inventen identificadores, código válido sin cambios, que no se dupliquen terminadores y expresiones válidas en varias líneas |
| `ide/tests/test_frontend.js` | Aplicación de `LL1_INSERT` y `LL1_REPLACE`, y rechazo de sugerencias obsoletas |

### Cómo ejecutarlas

Pruebas de Python:

```bash
python3 -m unittest discover -s ide/tests -p "test_*.py" -v
```

Resultado actual:

```text
Ran 58 tests
OK
```

Pruebas del frontend:

```bash
node ide/tests/test_frontend.js
```

Resultado actual:

```text
6 comprobaciones frontend LL(1) correctas.
```

Con esto confirmamos que los cambios son compatibles con las pruebas existentes del LL(1), el autocompletado, la integración del IDE y la detección de clones.
