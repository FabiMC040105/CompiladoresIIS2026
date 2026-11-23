# Suite de pruebas de optimizaciones VLIW

Esta carpeta contiene las 15 pruebas mínimas solicitadas para evaluar de forma aislada las optimizaciones del compilador CDFF.

## Organización

- `loop_unrolling/`: N constante, N variable pequeña y N grande.
- `dce/`: código muerto mezclado con llamadas que tienen efectos observables.
- `renombramiento/`: dependencias WAW/WAR y reutilización de nombres.
- `reordenamiento/`: saltos condicionales, funciones y ciclos; debe preservar dependencias RAW/WAR/WAW y de control.
- `spilling/`: presión de 10, 12 y 14 valores para exceder los 9 registros físicos `x4-x12`.

Cada programa deja su resultado verificable en la variable global `resultado`, ubicada en la dirección 0 de memoria.

## Ejecución completa

Desde `proyecto-1/src`:

```bash
make test-suite-optimizaciones
```

El runner:

1. compila una versión base y una optimizada de cada programa;
2. ejecuta ambas con el simulador VLIW funcional;
3. comprueba el valor esperado;
4. exige cero riesgos RAW;
5. verifica que la optimización realmente haya aplicado una transformación;
6. registra instrucciones IR, tamaño de código, ciclos, tiempo de compilación y cantidad de transformaciones;
7. genera CSV, JSON, HTML, Markdown y gráficas SVG.

Los resultados resumidos se guardan en:

```text
proyecto-1/docs/metricas-optimizaciones/
```

Los logs y ASM intermedios quedan en `src/evidencia-vliw/metricas-optimizaciones/`, directorio ignorado por Git.

## Línea base de spilling

Los tres programas de spilling exceden deliberadamente los nueve registros físicos `x4-x12`. Por eso la línea base usa el modo diagnóstico `--codegen-ir-mem`, que conserva temporales en memoria. La versión optimizada utiliza Chaitin y `--spill-ir`, de modo que únicamente los valores derramados se materializan en memoria.
