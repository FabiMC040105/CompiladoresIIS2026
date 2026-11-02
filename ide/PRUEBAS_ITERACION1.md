# Pruebas de aceptación — CDFF Studio V5 (Iteración 1)

**No ejecutar estas pruebas sobre archivos originales.** Usar «Nuevo» o una copia abierta desde «Ejemplos». No borrar `ide/workspace/`. Tras cada prueba de autocorrección, pulsar **Compilar** para confirmar con el verdadero `cdff`.

## Antes de comenzar

Detener servidor anterior (`Ctrl+C`), actualizar la carpeta `ide`, ejecutar:

```bash
rm -f ide/ide_syntax ide/ide_completion ide/ide_clones ide/ide_analyzer
make -C proyecto-1/src
python3 ide/app.py
```

Abrir <http://localhost:8877>, pulsar **Ctrl+F5**, confirmar **V5 · QA** y **Compilador cdff encontrado**. Dejar seleccionada **Sugerencias en vivo**.

## Pruebas funcionales

| # | Qué hacer | Qué debe pasar |
|---|---|---|
| 1 | Abrir `factorial.e`, luego pulsar **Nuevo** | Editor totalmente vacío, nombre `programa.e` |
| 2 | Escribir `defi` y `mien` | Sugerencias `defina` y `mientras`; Tab/Enter completan |
| 3 | Abrir `factorial.e` y **Compilar** | `Compilación correcta`; `.asm` y `.bin` en `ide/workspace` |
| 4 | Quitar `:)` a `entero resultado = 1 :)` | Tras una pausa, sugiere `:)` en la línea afectada, no en la siguiente |
| 5 | Restaurar un solo símbolo de `} :)` | No debe agregar una `:)` duplicada |
| 6 | Falta `)` en `(2 + 3 :)` | Propone cerrar `)` antes de `:)` |
| 7 | Falta `)` en `principal( {` | Propone `)` antes de `{` |
| 8 | Quitar una `}` conservando `:)` | Ofrece añadir solo `}` delante de ese `:)` |
| 9 | Quitar `}` y `:` conservando `)` | Propone reconstruir `} :)` en esa misma línea |
| 10 | Quitar `}` y `)` conservando `:` | Propone reconstruir `} :)` en esa misma línea |
| 11 | Dejar el código incompleto `entero x = 1 +` | Advertencia sin corrección arbitraria |
| 12 | Desactivar **Sugerencias en vivo** | No aparecen nuevas revisiones hasta usar «Revisar sintaxis» |
| 13 | Corregir un error y luego compilar de nuevo | El programa no debe contener cierres duplicados |
| 14 | Guardar 2+ `.e` y buscar clones | Reporta Tipo I/II con enlaces a líneas |

## Pruebas exactas de cierres (una a la vez)

Para estas seis pruebas, escribe primero lo siguiente en un archivo nuevo:

```text
defina vacio principal() {
    entero edad = 21 :)
} :)
```

Borra o cambia únicamente la **última línea** según esta tabla. Deja que transcurra una breve pausa y pulsa **Aplicar corrección**. Siempre debe volver a `} :)` sin líneas extra.

| Caso | Línea que dejas | Resultado que debe proponer |
|---|---|---|
| A | `}` | Agregar ` :)` |
| B | `} :` | Agregar **solo** `)` |
| C | `} )` | Insertar **solo** `:` delante de `)` |
| D | `:)` | Insertar **solo** `} ` delante de `:)` |
| E | `:` | Reemplazar `:` por `} :)` |
| F | `)` | Reemplazar `)` por `} :)` |

Repite dos casos con un comentario final `^ comentario` y dos casos dentro de un `si (...) { ... } :)` anidado. La corrección debe conservar los comentarios y el resto de instrucciones.

## Pruebas en las que NO debe inventar correcciones

**Ejemplo con un operador sin completar:**

```text
defina vacio principal() {
    entero x = 2 +
} :)
```

**Ejemplo de terminador legítimo en línea siguiente:**

```text
defina vacio principal() {
    entero x = 5
    :)
}
:)
```

El primero debe mostrar una observación prudente; el segundo no debe ofrecer duplicar el terminador. También prueba una expresión con un paréntesis abierto en una línea y cerrado en la siguiente: no debe inventar cierres en medio de la expresión.

## Clones Tipo I/II

En una segunda terminal WSL, desde la raíz del repo:

```bash
mkdir -p ide/workspace
cp ide/pruebas/clon_tipoI_A.e ide/pruebas/clon_tipoI_B.e ide/pruebas/clon_tipoII_C.e ide/workspace/
```

Recarga la página y pulsa **Buscar clones** con «Incluir ejemplos existentes» desmarcado. Debe detectar Tipo I entre A/B y Tipo II entre A/C (puede identificar otras coincidencias).

## Tests automatizados

```bash
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
# Opcional si tienen Node.js:
node ide/tests/test_frontend.js
```

Se han probado las rutas HTTP, las reglas de C, sus aplicaciones exactas, casos anidados, comentarios, ediciones parciales, Unicode y clones. Si alguno falla, **no marcar la iteración como verificada**. Guardar capturas de compilación correcta, diagnóstico, autocorrección, autocompletado, clones y del encabezado con la versión.

**Pendientes externos al IDE:** enlazado real entre archivos, lista completa de mejoras señaladas en la defensa del Proyecto 1 y la integración con SystemVerilog; requieren pruebas aparte.
