# CDFF Studio — IDE con LL(1) estático para autocompletado y sugerencias

**El compilador permanece congelado.** La gramática base, Flex/Bison, la semántica, el ensamblador y el código VLIW están en `proyecto-1/` y no se modifican. `cdff` sigue siendo quien valida y compila. El editor, sus controles y el detector de clones conservan su funcionamiento.

## Arranque

```bash
# Desde la raíz del proyecto
python3 ide/app.py
# Navegador: http://127.0.0.1:8877
```

Si existe una versión anterior ejecutándose, detenla (`Ctrl+C`) antes de iniciar el nuevo servidor; recarga la página con `Ctrl+F5`. Se necesitan `python3` y un compilador de C (`cc`). El IDE recompila sus programas C cuando cambian sus fuentes. Si se desea usar **Compilar**, `proyecto-1/src/cdff` debe estar construido con el procedimiento original.

## Arquitectura del IDE

| Archivo | Función |
|---|---|
| `ll1_table.h` | Tabla **estática** M[A,a], producciones, FIRST/FOLLOW y anulabilidad. |
| `ll1_predict.c/.h` | Tokenización auxiliar + algoritmo predictivo no recursivo con pila. |
| `ide_completion.c` | Palabras reservadas que la pila LL(1) permite **en ese contexto**. |
| `ide_syntax.c` | Diagnóstico y propuesta de cierres según los **terminales esperados por LL(1)**, simulando la edición con otra consulta al analizador. |
| `app.py` | Servidor HTTP; no decide qué corregir o completar. |
| `index.html` | Interfaz conservada; únicamente envía contexto al predictor y aplica sugerencias de C. |
| `ide_clones.c` | Detector de clones independiente de LL(1). |
| `tools/generate_ll1.py` | Generador **offline** de tabla estática, FIRST y FOLLOW; no se ejecuta al editar. |
| `LL1_TABLA.csv` | Matriz para inspección humana, no se utiliza en producción. |

No queda un motor anterior de autocompletado basado en `lexer.l` ni un segundo detector heurístico de sintaxis. El conjunto de tokens esperados nace de `LL1_TABLE` y de la pila; el mapa de etiquetas en `ide_completion.c` **no decide predicciones**. Se usan formatos `LL1_INSERT`, `LL1_REPLACE` y `NO_FIX`. Si no se puede determinar una corrección sintáctica segura, se informa sin editar código.

## Defensa y pruebas

Toda la explicación de FIRST, FOLLOW, M[A,a], pila, ejemplos y ubicación de funciones está en **[`DEFENSA_LL1.md`](DEFENSA_LL1.md)**. Para ver las producciones numeradas, abrir `LL1_CONJUNTOS_Y_PRODUCCIONES.md`.

```bash
python3 ide/tests/test_ll1.py
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
node ide/tests/test_frontend.js    # si tienen Node.js
```

Los diagnósticos de LL(1) son **sintácticos** y no sustituyen el compilador. En código temporalmente incompleto, las sugerencias pueden ser limitadas para evitar predicciones incorrectas. La gramática auxiliar se comprobó con el corpus de prueba, pero sin una prueba formal de equivalencia para todos los programas posibles.
