# Correcciones de compilador: memoria compartida y saltos largos

## Alcance

Se modifico **exclusivamente el compilador**. El proyecto de Arquitectura se empleo para contrastar los formatos de instrucciones y los limites de memoria; no se editaron sus archivos SystemVerilog.

## 1. Memoria de datos sin particion 32 KiB / 32 KiB

Antes se reservaba `0x0000..0x7FFF` para globales y `0x8000..0xFFFF` para pila, aunque una region estuviera vacia. Ahora se utiliza la memoria compartida de 65536 bytes:

```
0x00000                                                0x10000
  |-- globales estaticas --|-- libre --|-- pila activa --|
                          ^           ^                 ^
                   fin_globales      SP actual      SP inicial
```

- Las variables globales comienzan en `0` y se asignan consecutivamente, a razon de 4 bytes por palabra. La tabla de simbolos puede asignar hasta 65536 bytes de globales (si el programa no necesita pila).
- El puntero de pila `x14` arranca en `0x10000` y disminuye al reservar variables locales, parametros, cabeceras y registros guardados. Se conserva `x15` como frame pointer.
- `parser.y` entrega al backend `fin_globales = tabla->siguiente_direccion_global` dentro de `CodegenConfig`.
- Antes de cada reserva de un nuevo marco, el backend calcula `nuevo_sp = SP - reserva` y compara `nuevo_sp >= fin_globales`. Si no se cumple, redirige a `error_pila` antes de escribir memoria. `pila_hay_espacio` realiza una validacion estatica equivalente para marcos que ni siquiera cabrian con la pila vacia.
- Se valida aritmetica de las reservas y sus limites. La comprobacion sigue ejecutandose en tiempo de ejecucion y por tanto contempla llamadas anidadas y recursividad.

**Esta propuesta es una particion flexible entre datos globales estaticos y pila dinamica**, no un `malloc` ni un recolector de basura: no se incorporo heap; la cantidad de locales de cada funcion continua calculandose estaticamente por la tabla de simbolos.

**Caso importante:** Si se ocupan los 65536 bytes con globales, el programa no podra reservar un marco con tamanio positivo. En esos casos, el compilador puede rechazar una llamada imposible o la comprobacion de ejecucion enviara a `error_pila`.

## 2. Resolucion automatica de saltos lejanos

El generador **ya utilizaba `JMP`** para ciclos, `rompa`, llamadas, retorno al epilogo y el flujo principal. El problema era que un salto condicional a una etiqueta a mas de aproximadamente 2 KiB hacia delante o atras fallaba durante la resolucion.

Rangos definidos por la ISA: `BEQ/BNE/BGE/BLE` utilizan inmediatos firmados de 12 bits ([-2048, 2047] bytes). `JMP` utiliza 20 bits firmados ([-524288, 524287] bytes). El desplazamiento es relativo al **PC del bundle actual**, de 16 bytes por bundle.

Se anadio una fase iterativa de **relajacion de branches** dentro de `resolucion_etiquetas.c`, posterior al scheduler y anterior al empaquetador VLIW:

**BEQ/BNE lejanos**: se invierte la condicion y se salta sobre un `JMP` con mayor alcance:

```
# Original:
BRU: BEQ x4, x5, DESTINO_LEJANO

# Expansion:
BRU: BNE x4, x5, __branch_largo_fin_0
BRU: JMP DESTINO_LEJANO
__branch_largo_fin_0:
```

**BGE/BLE lejanos**: la ISA no tiene BLT ni BGT para negar estrictamente estas condiciones; se usa un pequeno trampolin que funciona incluso cuando los operandos son iguales:

```
BRU: BGE x4, x5, __branch_largo_tramp_0
BRU: JMP __branch_largo_fin_0
__branch_largo_tramp_0:
BRU: JMP DESTINO_LEJANO
__branch_largo_fin_0:
```

Se vuelven a calcular las posiciones y offsets despues de cada expansion, porque insertar bundles puede causar que otro branch pase a estar fuera de rango. Los saltos normales de alcance corto quedan intactos.

**Restricciones conservadas:** La memoria de instrucciones continua limitada a 64 KiB (4096 bundles) por el hardware entregado, aunque la instruccion `JMP` tenga un campo con mayor rango teorico. Las referencias `ADDI rd, x1, etiqueta` no se relajan a `JMP`, porque sirven para calcular direcciones de retorno y cambiar su significado romperia la convencion de llamadas. Las etiquetas simbolicas son las que habilitan la relajacion automatica; los offsets numericos escritos manualmente no se recolocan.

## Archivos modificados

- `src/memoria.h` y `src/memoria.c`: verificaciones de espacio con frontera global real.
- `src/tabla_simbolos.c`: rango global de 64 KiB y maxima reserva local dependiente de los globales.
- `src/codegen.h` y `src/parser.y`: propagacion de `fin_globales` desde tabla de simbolos.
- `src/codegen.c`: checks runtime que comparan con `fin_globales`; comentario de inicio de memoria.
- `src/resolucion_etiquetas.c`: relajacion y recalculo iterativo de branches largos.
- `src/prueba_resolucion_asm.c`: pruebas BEQ/BNE/BGE/BLE hacia delante, backwards y cascada.
- `src/pruebas/test_limite.c`, `src/pruebas/test_global_exacto.c`, `src/pruebas/test_memoria_dinamica.c`: pruebas del limite de 64 KiB y proteccion de pila.
- `src/test_codegen_integracion.c`: prueba del limite real > 32 KiB traspasado al backend.
- `src/Makefile`: objetivo `test-memoria` incorporado a `test`.

## Validaciones efectuadas

Pasaron las pruebas C aisladas de memoria, resolucion de etiquetas (36 comprobaciones, 0 fallos), codificacion de binarios, scheduler, empaquetador VLIW, etiquetas y la integracion de `codegen` con tabla de simbolos. Se verifico ademas un flujo `resolver_etiquetas_asm -> vliw_empaquetar_archivo -> binario_ensamblar_archivo` con dos branches largos y **197 bundles codificados**.

La prueba de memoria comprobo 32772 y 65536 bytes globales como validos y 65540 como no validos; tambien escenarios de pila con globales que superan los 32 KiB y pila superior a los antiguos 32 KiB.

En el entorno de revision no estaban instalados Flex/Bison ni Icarus Verilog. Por tanto, **no se pudo repetir el comando `make all`, el parser/lexer completo, ni una simulacion RTL de la CPU**. Los archivos relevantes de C compilaron con GCC y las pruebas aisladas descritas funcionaron, pero para dar la integracion completa por validada deben ejecutar en su entorno:

```sh
cd proyecto-1/src
make rebuild
make test
make test-completo ARQ_ROOT=/ruta/al/repositorio/Arquitectura1-IIS2026
```

`make test-completo` incluye las pruebas RTL de integracion, siempre que se instalen las herramientas requeridas.
