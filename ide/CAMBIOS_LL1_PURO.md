# Cambios realizados — segunda entrega LL(1) sin lógica antigua

## Qué se quitó

- `ide_analyzer.c`: se eliminó el detector previo de sugerencias por salida de Bison y texto de líneas. También su compilación y referencia en `app.py`.
- `ide_completion.c`: se eliminó el modo que, sin contexto del cursor, leía reglas de `lexer.l` y ofrecía palabras solo por prefijo. **No hay fallback**: ahora el contexto es obligatorio.
- `ide_syntax.c`: se reemplazó completamente el código que reconocía líneas como declaraciones/cabeceras mediante `word()`, `statement()`, `is_header()`, `ends_operator()` y búsquedas de caracteres. No se llaman las funciones antiguas ni se decide una sugerencia con ese motor.
- `index.html`: se eliminó la ruta especial que aplicaba la antigua propuesta `APPEND_BLOCK`. El editor ahora aplica solamente las operaciones generales `LL1_INSERT` / `LL1_REPLACE`.
- Se actualizaron `README.md`, `PRUEBAS_ITERACION1.md`, `.gitignore` y los tests para que no enseñen ni invoquen la lógica borrada.

## Qué se usa ahora (flujo)

1. `ll1_predict.c` tokeniza el texto original y lo analiza con una pila LL(1).
2. **La línea `int production=LL1_TABLE[nt][token]` es la consulta de M[A,a]**. La tabla `static const` y sus producciones están en `ll1_table.h`.
3. `expected_from_stack()` obtiene FIRST de los símbolos pendientes, propagando ε, lo que produce los tokens esperados para el cursor o el primer error.
4. `ide_completion.c` usa `ll1_expects()` antes de mostrar una palabra del mapa de etiquetas de tokens. La lista no es un predictor independiente: define cómo escribir en pantalla un token **ya permitido** por LL(1).
5. `ide_syntax.c` ejecuta `generate()` y `propose()`, que consulta el conjunto `expected` del **primer error gramatical**. Solo evalúa insertar/reemplazar cierres (`:)`, `)`, `]`, `}`) que LL(1) espera.
6. `edit_score()` vuelve a pasar cada propuesta por `ll1_analyze()`. Solo aprueba las que permiten completar la entrada o avanzar en tokens originales; cuando el estado sigue ambiguo/incompleto, envía `NO_FIX` con los terminales esperados.
7. `app.py` entrega el resultado con su ubicación exacta; `index.html` no reinterpreta la gramática y requiere clic en **Aplicar corrección**.

## Archivos modificados

| Archivo | Modificación |
|---|---|
| `ide_completion.c` | Reescrito, ahora solo consulta predicciones LL(1) con contexto obligatorio. |
| `ide_syntax.c` | Reescrito, sin reglas antiguas; propuestas únicamente derivadas del estado del predictor y comprobadas con un segundo análisis. |
| `ll1_predict.c/.h` | Devuelve posiciones de tokens y distingue prefijo válido de entrada completa; expone la información necesaria para ubicar correcciones. |
| `app.py` | Elimina el constructor/ejecutor del analizador antiguo, su fallback, y recibe solamente las acciones `LL1_INSERT`, `LL1_REPLACE` y `NO_FIX`. |
| `index.html` | Conserva diseño y controles; simplifica aplicación de propuestas. |
| `tests/*` | Reemplaza pruebas de heurísticas anteriores por regresiones de LL(1), HTTP, ediciones y frontend. |
| `DEFENSA_LL1.md` | Actualiza preguntas de código con nombres de funciones que existen. |
| `README.md`, `PRUEBAS_ITERACION1.md`, `.gitignore` | Quita documentación y referencias antiguas. |

La tabla `ll1_table.h`, el generador `tools/generate_ll1.py`, las producciones, FIRST y FOLLOW **son los mismos** que en la primera entrega: no se cambió la gramática. El compilador (`proyecto-1/`) tampoco cambió; se verificó la identidad de sus 130 archivos con SHA-256.

## Resultados de pruebas de esta entrega

```text
python3 ide/tests/test_ll1.py                    13/13
python3 ide/tests/test_ide.py                      9/9
python3 ide/tests/test_correcciones_matriz.py      9/9
node ide/tests/test_frontend.js                    6/6
TOTAL                                             37/37
```

Casos de regresión de especial interés: una expresión `1\n + 2 :)` no recibe `:)` tras `1`; después de `1 +` se muestran FIRST de la expresión sin sugerir FIN; `si { } sino { } :)` no recibe terminador entre ramas; declaraciones sin `:)`, paréntesis y llaves ausentes tienen reparaciones comprobadas gramaticalmente; no se sugieren palabras reservadas fuera de contexto.

**Límites:** no se recuperan todos los errores posibles, no se completan identificadores del usuario y no se adivinan expresiones faltantes. Algunas combinaciones de cierres se ofrecen solo si el análisis de la cadena editada mejora. Es preferible mostrar `NO_FIX` a producir una edición que la tabla LL(1) no justifique.

Para el detalle de la defensa ver `DEFENSA_LL1.md`.
