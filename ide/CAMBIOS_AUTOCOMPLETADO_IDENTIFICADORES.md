# Iteración: autocompletado de identificadores (LL(1) + símbolos IDE)

## Objetivo
Añadir nombres de variables, parámetros, funciones, listas y matrices al autocompletado **sin alterar** la gramática congelada ni `LL1_TABLE`. No se modifica Flex, Bison, cdff, HTML o CSS.

## Diseño y defensa

1. `ll1_predict.c`, función `ll1_analyze()`: usa pila predictiva y consulta la matriz `LL1_TABLE[nt][token]`. Al llegar al cursor calcula los terminales permitidos con `expected_from_stack()`.
2. `ide_completion.c`, función `main()`: llama `ll1_analyze` y solo cuando `ll1_expects(&r, T_ID)` es verdadero invoca `ide_visible_symbols()`. Los nombres están **subordinados** al resultado LL(1), nunca reemplazan la tabla.
3. `ide_symbols.c`, función `ide_visible_symbols()`: construye una tabla auxiliar temporal a partir del texto anterior al cursor. Lee tokens mediante `ll1_next_token()`, detecta definiciones de funciones, parámetros, declaraciones de variables/listas/matrices, maneja bloques `{...}` y filtra nombres visibles según el ámbito.
4. `ll1_predict.c`, función `ll1_next_token()`: tokenizador existente expuesto como función pública compartida para evitar duplicar reglas léxicas.
5. `app.py`, `build_completion()`: compila el autocompletado incluyendo `ide_symbols.c` y sus dependencias.

### Respuestas cortas al profesor
- **¿La tabla LL(1) cambió?** No. Los 105 números de producción, FIRST, FOLLOW y matriz M[A,a] siguen intactos.
- **¿Quién autoriza sugerir variables?** FIRST de la pila predictiva debe incluir `ID`; `ll1_expects(&r, T_ID)` habilita la consulta de nombres.
- **¿De dónde salen los nombres?** De la tabla temporal del IDE en `ide_symbols.c`, no de un conjunto codificado manualmente ni de la tabla semántica de cdff.
- **¿Cómo se respetan ámbitos?** Cada `{` crea un ámbito con padre, `}` lo cierra. Los parámetros se agregan al ámbito de la función; las variables del `para` se limitan al ciclo. Se filtran descendientes, ámbitos hermanos y nombres ocultos.
- **¿Qué ocurre ante errores sintácticos anteriores al cursor?** No aparecen sugerencias si `ll1_analyze` devuelve un error; no hay fallback de palabras.

## Archivos creados
- `ide_symbols.h`: interfaz de la tabla auxiliar.
- `ide_symbols.c`: extracción y visibilidad de identificadores.
- `tests/test_symbol_completion.py`: 12 pruebas de regresión.

## Archivos editados
- `ll1_predict.h`: declara `ll1_next_token`.
- `ll1_predict.c`: expone el mismo tokenizador usado antes por LL(1) (sin cambiar sus reglas).
- `ide_completion.c`: combina palabras clave autorizadas por LL(1) con identificadores del IDE, también autorizados por LL(1).
- `app.py`: compila el nuevo archivo C junto al motor de autocompletado.

## Pruebas manuales
1. `defina vacio principal() { entero resultado = 3 :)\n res` → sugerir `resultado` (etiqueta `entero`).
2. `entero globales = 3 :)\ndefina vacio principal() {\n glo` → `globales`.
3. `defina entero sumar(entero primero, entero segundo) {\n pri` → `primero` (etiqueta `parámetro`).
4. `defina entero sumar(entero a) { devuelva a :) } :)\ndefina vacio principal() {\n sum` → `sumar` (etiqueta `función`).
5. `defina vacio uno() { entero secreto = 1 :) } :)\ndefina vacio principal() {\n sec` → NO sugerir `secreto`.
6. Dentro de `para (entero indice = 0, indice < 3, indice = indice + 1) { ... }` → `indice` disponible; fuera del ciclo no.

## Limitaciones deliberadas
- Solo se analizan declaraciones **anteriores al cursor**; funciones y variables definidas más adelante no se incluyen en esta iteración, aunque el compilador pueda permitir funciones declaradas después.
- La tabla auxiliar es para autocompletar, **no** hace verificación semántica de tipos ni reemplaza la tabla de símbolos del compilador.
- No se sugieren identificadores con prefijos de menos de dos caracteres, como en la versión anterior.
- En el caso general `ID`, la tabla LL(1) reconoce la categoría; la sugerencia de un nombre concreto no garantiza que su uso sea correcto semánticamente.

## Ejecutar
Desde la raíz del proyecto:

```bash
python3 ide/app.py
python3 ide/tests/test_symbol_completion.py
python3 ide/tests/test_ll1.py
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
node ide/tests/test_frontend.js
```

Reiniciar el servidor existente y recargar la página (Ctrl+F5) para usar el ejecutable de autocompletado recién compilado.
