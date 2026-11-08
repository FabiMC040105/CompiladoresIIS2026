# Pruebas del IDE con LL(1) estático

Este archivo reemplaza las pruebas antiguas de las heurísticas ya eliminadas.

Ejecutar desde la raíz del proyecto:

```bash
python3 ide/tests/test_ll1.py
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
node ide/tests/test_frontend.js
```

Casos para comprobar manualmente en el navegador:

| Contexto | Resultado esperado |
|---|---|
| `mie` al inicio del archivo | No sugerir `mientras`. |
| `mie` dentro de `principal() {` | Sugerir `mientras`. |
| `entero x = fa` | Sugerir `falso`; no sugerir `mientras`. |
| `entero x = 1` antes de una segunda instrucción | Sugerir insertar `:)` al final de la primera declaración. |
| `entero x = 1` y en la línea siguiente `+ 2 :)` | No insertar `:)` antes del `+`. |
| `entero x = 1 +` | Indicar tokens de expresión esperados; no autoinsertar `:)`. |
| `entero x = (2 + 3 :)` | Sugerir `)` antes de `:)`. |
| `si (...) { ... }` seguido de `sino { ... } :)` | No sugerir `:)` entre las dos ramas. |
| `{ ...` con cierre `} :)` faltante al final | Proponer el cierre si la cadena corregida lo verifica LL(1). |

Todos los cambios son sugerencias: requieren pulsar **Aplicar corrección** y se vuelven a comprobar inmediatamente. El compilador Bison/Flex continúa siendo independiente.
