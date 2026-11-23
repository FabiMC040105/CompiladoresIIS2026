# Spilling Register

## Objetivo

La implementación resuelve presión de registros sobre los temporales de la
representación intermedia mediante análisis de liveness, construcción de un
grafo de interferencia, coloreo y vaciado selectivo de temporales a memoria.

Se consideran nueve registros disponibles para temporales:

- color 0 -> x4
- color 1 -> x5
- color 2 -> x6
- color 3 -> x7
- color 4 -> x8
- color 5 -> x9
- color 6 -> x10
- color 7 -> x11
- color 8 -> x12

## Flujo

IR
-> CFG
-> liveness
-> grafo de interferencia
-> coloreo
-> detección de presión superior a K
-> selección de spill
-> STORE/LOAD
-> nuevo liveness
-> nuevo grafo
-> recoloreo

El proceso se repite hasta que el grafo completo pueda colorearse.

## Liveness

El análisis existente en `liveness.c` calcula los conjuntos IN y OUT por
bloque e instrucción.

Dos temporales que permanecen vivos simultáneamente no pueden ocupar el
mismo registro.

## Grafo de interferencia

La construcción se realiza en:

`grafo_interferencia.c`

Función:

`gi_construir()`

Cada temporal es un nodo. Se agrega una arista entre temporales que están
vivos simultáneamente.

El grafo puede mostrarse textualmente mediante:

`gi_imprimir_grafo()`

También puede exportarse en formato Graphviz DOT mediante:

`gi_exportar_dot()`

## Coloreo

El coloreo ocurre explícitamente en:

`grafo_interferencia.c`

Función:

`gi_colorear()`

El algoritmo utiliza las fases:

1. SIMPLIFY
2. SELECT / COLOR

Cuando existe un nodo con grado menor que K se retira temporalmente.

Cuando no existe ninguno, se selecciona como candidato de spill un nodo de
alta presión de interferencia.

Durante SELECT/COLOR se reconstruye el grafo y se asigna el primer color
disponible.

Si un nodo no puede recibir ninguno de los K colores se marca:

`spill = 1`

## Asignación

`asignacion_registros.c` transforma el resultado del coloreo en:

`temporal -> registro físico`

o:

`temporal -> slot de spill`

La función principal es:

`ar_construir()`

La asignación final también puede obtenerse mediante:

`spill_asignacion_final()`

## Transformación de spilling

La transformación se encuentra en:

`spill_transform.c`

Función:

`spill_resolver_programa()`

Por cada temporal marcado para spill:

1. Se inserta un STORE después de su definición.
2. Se libera la presión sobre su registro.
3. Se inserta un LOAD antes de cada uso posterior.
4. Se generan nuevos temporales para los reloads.
5. Se recalculan CFG, liveness y grafo.
6. Se colorea nuevamente.

Los accesos se representan en IR mediante el área reservada:

`cdff_spill_area`

No corresponde a una variable declarada por el usuario.

## Backend IR

`codegen_ir.c` detecta los accesos a `cdff_spill_area`, reserva memoria para
los slots de spilling y los traduce a operaciones reales LSU LOAD/STORE.

Por lo tanto el flujo de spilling llega a:

IR optimizada
-> ASM
-> scheduler
-> bundles VLIW
-> código binario

## Heurística

Durante SIMPLIFY se priorizan nodos con grado menor que K.

Cuando no existe ninguno, se selecciona como candidato el nodo activo de
mayor grado.

El coloreo es optimista: un candidato de spill todavía puede recibir color
durante SELECT/COLOR si al reconstruir el grafo existe uno disponible.

## Evidencia

### Caso de nueve temporales

Con nueve temporales simultáneamente vivos y nueve registros, el grafo puede
colorearse completamente.

### Caso de diez temporales

Con diez temporales simultáneamente vivos y solamente nueve registros, al
menos un nodo queda sin color y debe derramarse.

### Programas obligatorios

Se incluyen tres programas independientes:

- `spill_01_10_argumentos.e`
- `spill_02_12_argumentos.e`
- `spill_03_14_argumentos.e`

Los casos incrementan progresivamente la presión de registros.

Resultados observados:

- 10 argumentos: 2 STORE, 2 LOAD, 2 slots
- 12 argumentos: 4 STORE, 4 LOAD, 4 slots
- 14 argumentos: 6 STORE, 6 LOAD, 6 slots

Los tres programas completan generación IR, transformación de spilling,
backend IR, resolución de etiquetas, empaquetado VLIW y generación binaria.

## Comandos de validación

Prueba del grafo y coloreo:

`make test-grafo-spill`

Transformación y recoloreo:

`make test-spill`

Integración de optimizaciones:

`make test-opt-ir`

Backend IR:

`make test-codegen-ir-backend`

Tres casos obligatorios:

`make test-spill-obligatorio`

Validación completa:

`make test-spill-completo`

## Defensa

Para señalar dónde se colorea el grafo:

`grafo_interferencia.c -> gi_colorear()`

Dentro de esa función se encuentran explícitamente:

`SIMPLIFY`

y:

`SELECT / COLOR`

El resultado de cada color se traduce conceptualmente a x4-x12.

Cuando no existe un color disponible el nodo se marca para spill. La
transformación posterior genera STORE/LOAD y repite el análisis hasta que
el grafo final sea coloreable.
