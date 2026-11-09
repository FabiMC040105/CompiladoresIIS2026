# Guía de defensa — IDE con predicciones LL(1) estáticas (sin motor anterior)

## Alcance

El compilador `proyecto-1/` permanece **congelado**. El IDE no compila mediante LL(1); `cdff` sigue usando Flex/Bison y decide si un programa se acepta. LL(1) se usa exclusivamente para **autocompletado contextual y sugerencias/cierres sintácticos**. Se conservan el editor, diseño, botones, guardado, ejecución y detector de clones.

**No quedan dos motores de sugerencias**: se eliminó `ide_analyzer.c`, desapareció el modo que autocompletaba leyendo reglas sueltas de `lexer.l`, y `ide_syntax.c` se reescribió sin sus reglas antiguas basadas en líneas y caracteres. Las únicas palabras visibles en `ide_completion.c` son un mapa `token → palabra` para representar terminales que **LL(1) ya predijo**; ese mapa no determina en qué contexto aparece una palabra.

## Preguntas para la defensa: dónde está cada elemento

| Pregunta del profesor | Archivo / ubicación | Explicación |
|---|---|---|
| ¿Dónde está la gramática que usa el IDE? | `ide/tools/generate_ll1.py`, `GRAMMAR` | Forma apta para LL(1) de la sintaxis congelada, sin acciones semánticas. Se eliminaron recursiones izquierdas y factorizaron alternativas. |
| ¿Dónde está la tabla LL(1)? | **`ide/ll1_table.h`, `LL1_TABLE[N_COUNT][T_COUNT]`** | Matriz `static const`, filas A y columnas a. Cada celda contiene Pn o `-1`. |
| ¿Dónde están las producciones Pn? | `ide/ll1_table.h`, `LL1_PRODUCTIONS` | Arreglo de partes derechas, con comentarios que muestran su producción. |
| ¿Dónde están FIRST/FOLLOW? | `ide/tools/generate_ll1.py`, cálculo de `first`, `follow`, `nullable` | Generados offline. Datos conservados como `LL1_FIRST`, `LL1_FOLLOW`, `LL1_NULLABLE`. |
| ¿Cómo se verifica que sea LL(1)? | `ide/tools/generate_ll1.py`, comprobación de conflictos | Si una celda M[A,a] recibe dos producciones incompatibles, el generador falla. |
| ¿Dónde se tokeniza el texto? | `ide/ll1_predict.c`, `next_token()` | Buffer de tokens auxiliar; no ejecuta acciones de Bison. |
| ¿Dónde está la pila? | `ide/ll1_predict.c`, `ll1_analyze()` | Inicializa `EOF ($)` y `PROGRAM`, consume terminales y expande no terminales. |
| ¿Dónde **consulta la tabla**? | **`ide/ll1_predict.c`, `int production = LL1_TABLE[nt][token];`** | Regla de selección M[A,a] del algoritmo del profesor. |
| ¿Cómo deriva por la izquierda? | `ide/ll1_predict.c`, ciclo `for (int i=p->length-1; i>=0; --i)` | Apila la RHS al revés para procesar el símbolo de la izquierda primero. |
| ¿De dónde salen los terminales esperados? | `ide/ll1_predict.c`, `expected_from_stack()` | FIRST del resto de pila, propagando ε, no «palabras probables» por texto. |
| ¿Cómo decide **qué palabra autocompletar**? | `ide/ide_completion.c`, `ll1_expects(&r, token)` | Filtra *después* de procesar el contexto por LL(1). Sin contexto no hay sugerencias. |
| ¿Cómo detecta faltantes sin reglas viejas? | `ide/ide_syntax.c`, `generate()` → `propose()` → `expected_at_error()` | LL(1) detecta error y proporciona `expected` del estado. Solo evalúa cierres que estén en ese conjunto. |
| ¿Cómo comprueba una corrección? | `ide/ide_syntax.c`, `edit_score()` → **`ll1_analyze()`** | Simula la edición y vuelve a analizar toda la entrada. Si no mejora, no la propone. |
| ¿Dónde se muestran sugerencias? | `ide/app.py`, `syntax_from_c()`; `ide/index.html`, `renderIssues()` | Python solo traduce offsets UTF-8 a UTF-16. HTML solo muestra/aplica lo validado. |
| ¿Cómo se envía el cursor? | `ide/index.html`, `scheduleCompletion()` | Se envía texto antes del prefijo para que LL(1) procese únicamente tokens completos. |
| ¿Dónde está la prueba visible de M[A,a]? | `ide/ide_ll1_trace.c` | Muestra columnas PASO, TOPE, LOOKAHEAD y ACCIÓN, con `M[A,a] = Pn`. |

## Algoritmo igual a las diapositivas

1. **FIRST** de los símbolos y producciones (ε incluido).
2. **FOLLOW** de cada no terminal.
3. Construcción de `M[A,a]`: para `A → α`, colocar esa producción en columnas de `FIRST(α) - {ε}`. Si ε pertenece a FIRST(α), colocarla además en las columnas de `FOLLOW(A)`.
4. Comprobar ausencia de conflictos. La tabla generada contiene **105 producciones, 54 no terminales y 40 categorías de terminal**, sin conflictos.
5. Algoritmo **Top-Down LL(1) no recursivo**: pila `[ $, PROGRAM ]`, buffer de tokens, expansión de la producción M[A,a], coincidencia terminal, avance o error. Siempre se procesa el no terminal más a la izquierda; **derivación izquierda no significa recursión izquierda**.

La matriz se calcula **offline** para este lenguaje congelado y está incrustada como `static const`. No se recalculan FIRST/FOLLOW durante la edición. `LL1_TABLA.csv` es solo la vista amigable de la matriz; **en ejecución se consulta `ll1_table.h`, no el CSV**.

## Dos recorridos demostrables en código

### Autocompletado

Código del usuario: `defina vacio principal() {` y comienza a escribir `mie`.

`index.html` envía el código **antes de `mie`** → `app.py` llama a `ide_completion` → `ll1_analyze()` procesa tokens y consulta `LL1_TABLE` → `expected_from_stack()` devuelve `MIENTRAS` entre otros → `ll1_expects(T_MIENTRAS)` autoriza **mientras** → el prefijo `mie` solo filtra la etiqueta visible. Al principio del archivo `MIENTRAS` no está permitido, por lo que **no aparece**.

### Sugerencia de terminador

```
defina vacio principal() {
 entero x = 1
} :)
```

`ll1_analyze()` se detiene frente a `}`: no hay entrada en M[A,RC] durante la expresión/declaración pendiente. `expected_from_stack()` incluye **FIN (`:)`)**. `propose()` prueba exclusivamente los cierres que aparecen en ese conjunto, y `edit_score()` analiza una copia con ` :)` insertado tras `1`. El programa corregido queda aceptado; la interfaz recibe `LL1_INSERT` en la línea 2. Si el código fuese `entero x = 1\n + 2 :)`, **la gramática lo acepta completo**, así que no se propone cerrar anticipadamente.

En cambio, para `entero x = 1 +`, la tabla espera expresiones (`ID`, `NUM`, `TRUE`, `FALSE`, `LP`, `MINUS`, etc.), **no FIN**. El IDE muestra `NO_FIX` con los esperados sin insertar `:)` por su cuenta.

### Sobre las reparaciones

La tabla predice **terminales**, no coordenadas del editor. Se usan los offsets de tokens consumidos para ubicar una edición (por ejemplo, antes de un comentario); **nunca** se utiliza la pertenencia de una línea a una instrucción o la presencia de un carácter como justificación para inventar qué terminal falta. Se prueban un conjunto limitado de plantillas de cierre `:)`, `)`, `]`, `}` y combinaciones de ellas, todas subordinadas al `expected` actual y a un segundo análisis LL(1). No hay backtracking para elegir producciones; las simulaciones son un mecanismo separado para comprobar reparaciones. Una situación sin corrección verificable muestra `NO_FIX`.

**Limitaciones:** sin corrección semántica (ámbitos, tipos, variables), sin predicción de nombres definidos por el usuario, sin recuperación arbitraria de todos los errores; expresiones incompletas pueden impedir sugerencias posteriores. El desplegable completa palabras reservadas desde 2 letras; los símbolos se ofrecen en el panel de revisión. La gramática auxiliar está comprobada con el corpus `.e` y casos adicionales, pero no es una prueba formal de equivalencia universal con la gramática Bison.

## Comandos de defensa

Desde la carpeta raíz del ZIP:

```bash
python3 ide/app.py
# Navegador: http://localhost:8877

# Ver cada consulta M[A,a] del algoritmo
cc -std=c11 -Wall -Wextra -O2 -o ide/ide_ll1_trace ide/ide_ll1_trace.c ide/ll1_predict.c
./ide/ide_ll1_trace proyecto-1/docs/evidencias/factorial.e

# Ver la tabla legible
head -n 6 ide/LL1_TABLA.csv

# Regenerar/verificar la tabla solo cuando se quiera comprobar la construcción
python3 ide/tools/generate_ll1.py

# Probar sin tocar el compilador
python3 ide/tests/test_ll1.py
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
node ide/tests/test_frontend.js
```

`app.py` compila `ide_completion` e `ide_syntax` automáticamente cuando cambia el código C. Hay que detener un servidor previo con `Ctrl+C` y recargar el navegador con `Ctrl+F5`. El botón **Compilar** requiere el `cdff` ya construido en `proyecto-1/src`, como siempre.
