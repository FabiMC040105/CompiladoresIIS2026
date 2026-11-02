# CDFF Studio — Iteración 1, revisión integral V5

IDE local para el lenguaje `.e` del Proyecto 2 de Compiladores CE1108. **La lógica del compilador permanece en C/Flex/Bison** dentro de `proyecto-1/src`. El IDE no modifica esos archivos.

## Arquitectura

| Parte | Lenguaje | Función |
|---|---|---|
| `proyecto-1/src` | C, Flex, Bison | Lexer, parser, semántico, generación de bundles y binario VLIW |
| `ide_completion.c` | C | Autocompletado leyendo palabras del `lexer.l` real |
| `ide_syntax.c` | C | Detección conservadora de cierres y propuestas de edición exacta |
| `ide_clones.c` | C | Clones Tipo I y II entre `.e` guardados (y ejemplos opcionales) |
| `app.py` | Python estándar | Servidor local y puente a los programas escritos en C |
| `index.html` | HTML/CSS/JavaScript | Editor, resaltado, paneles, botones y aplicación explícita de propuestas |

## Instalación en WSL

Extraer **solo la carpeta `ide`** del ZIP junto a `proyecto-1/`. Reemplazar los archivos anteriores, sin borrar `ide/workspace/` ni modificar `proyecto-1/src`.

Desde la raíz de `CompiladoresIIS2026`:

```bash
# Detener primero cualquier servidor anterior con Ctrl+C
rm -f ide/ide_syntax ide/ide_completion ide/ide_clones ide/ide_analyzer
make -C proyecto-1/src
python3 ide/app.py
```

En Chrome/Edge: <http://localhost:8877>. Recargar con **Ctrl+F5** y comprobar que el encabezado diga **V5 · QA**. Se requiere Python 3 y `cc` (GCC/Clang); no requiere instalar bibliotecas Python externas. Los módulos auxiliares en C se compilan automáticamente al arrancar.

## Funcionamiento

- **Nuevo** abre siempre un editor vacío. Los `.e` de `proyecto-1/src` se abren como copia. Guardar y compilar escriben en `ide/workspace/`.
- **Compilar** usa el ejecutable C `cdff`, muestra diagnósticos y genera los artefactos correspondientes.
- **Sugerencias en vivo** está activado por defecto y analiza con C tras una pausa de aproximadamente 900 ms. Se puede desactivar con su casilla. También está el botón **Revisar sintaxis**.
- **Autocompletado** ofrece palabras reconocidas por el lexer; usa Tab/Enter para aceptar, Esc para cerrar, flechas para moverse.
- **Corrección de cierres:** reconoce faltas evidentes de `:)`, `)`, `}` y combinaciones, incluidas las formas parciales `} :`, `} )`, `:)`, `:` y `)` cuando el contexto respalda un cierre de bloque.
- Una propuesta está vinculada a una posición y al texto original. **Si cambias el código, debe recalcularse; nunca se aplica a ciegas.** Tras aplicar una sugerencia, el IDE vuelve a analizar el documento.
- **Buscar clones** compara archivos `.e` guardados, opcionalmente los ejemplos; los fragmentos demasiado cortos no se incluyen.

## Límites deliberados

- El analizador auxiliar es **heurístico**, no un segundo parser completo. Si no puede decidir dónde va un cierre, muestra una advertencia **sin proponer una reparación**. Esto evita insertar símbolos incorrectos en casos ambiguos.
- No ofrece corrección automática de **cualquier** error semántico, de tipos o de gramática. El diagnóstico definitivo sigue siendo el de `cdff`.
- La detección de clones usa un umbral mínimo: fragmentos con 3+ líneas y 18+ tokens. No garantiza detectar cualquier similitud funcional o reordenamiento arbitrario.
- El IDE no implementa por sí mismo el **enlazado real entre archivos `#traigase`**; esto debe resolverse en el compilador.
- Falta validar con el equipo las correcciones de memoria y saltos del Proyecto 1 y su integración con Arquitectura.

## Guía y pruebas de regresión

Abrir [`PRUEBAS_ITERACION1.md`](PRUEBAS_ITERACION1.md) y seguir las pruebas de principio a fin.

```bash
python3 ide/tests/test_ide.py
python3 ide/tests/test_correcciones_matriz.py
# Opcional: si tienen Node.js instalado
node ide/tests/test_frontend.js
```

Las dos suites Python comprueban HTTP/C y una matriz extensa de errores. La prueba JS comprueba la aplicación precisa de cambios. **No sustituyen probar visualmente el IDE en Chrome/Edge con su compilador real.**

## Repositorio y seguridad

- El servidor escucha exclusivamente en `127.0.0.1`.
- El servidor llama a procesos con listas de argumentos (sin `shell=True`).
- Solo acepta nombres simples terminados en `.e` y archivos de máximo 1 MB.
- `ide/.gitignore` excluye los archivos de `workspace/`, binarios auxiliares y `__pycache__`.
- Antes de subir: verificar `git status`, las pruebas manuales, la rama y las observaciones del profesor.
