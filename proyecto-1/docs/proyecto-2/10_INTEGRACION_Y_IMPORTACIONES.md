# 10. Integración del compilador, importaciones y RTL de Arquitectura

## 10.1 Enlace entre archivos con `#traigase`

La especificación requiere enlazado entre archivos. El repositorio incorpora `pruebas/importaciones/`, con un programa principal y módulos `utilidades.e` y `matematica.e`. El script `pruebas/probar_importaciones.sh` evalúa:

- Resolución de referencias a funciones de otros archivos y **cadenas de importación**.
- Ausencia de duplicados por importaciones repetidas.
- Emisión de `.asm` y ejecución del programa combinado con valor esperado **22** en `RAM[0]`.
- Verificación de **RAW=0** mediante `simular_asm_ir.py`.
- Error claro cuando un archivo importado no existe.

Comando: `make test-importaciones`. Requiere construir `cdff` en la misma revisión.

## 10.2 Comparación de optimizaciones juntas

```bash
cd proyecto-1/src
bash pruebas/compilar_integracion_3_opt.sh
```

El archivo `integracion_3_opt.e` se compila una vez como base y otra con:

```text
--codegen-ir --unroll 2 --dce --renombrar-ir --reordenar-ir --emit-ir -v
```

Por tanto, aunque su **nombre sea `integracion_3_opt`**, el comando activa **cuatro técnicas** (unrolling, DCE, renombramiento y reordenamiento), y no fuerza spilling. El script cuenta líneas de binario y compara los ASM con el simulador; la propia salida advierte que el valor matemático esperado es **35** y se necesita identificar/verificar su dirección de almacenamiento para afirmar equivalencia en RAM.

## 10.3 Integración real con el procesador VLIW

La prueba `pruebas/probar_integracion_rtl_actual.sh` compila `integracion_3_opt.e` con **las cinco fases**:

```bash
./cdff --codegen-ir --unroll 2 --dce --renombrar-ir --reordenar-ir --spill-ir \
    --emit-ir -s -v -o "evidencia-vliw/rtl-integracion-final/programa.bin" \
    "evidencia-vliw/rtl-integracion-final/programa.e"
```

A continuación:

1. Exige un binario cuyas líneas tengan **32 dígitos hexadecimales** (un bundle de **128 bits**).
2. Simula el ASM con `--validar-raw`: exige **RAM[0] = 35** y **0 riesgos RAW**.
3. Compila el testbench `tb_integracion_rtl_actual.sv` y los `.sv` encontrados bajo `$ARQ_ROOT/src`, usando `iverilog -g2012`.
4. Ejecuta `vvp` y espera `PASS RTL: binario de CE1108 ejecuto correctamente`.
5. El testbench observa la línea correspondiente en la **L2 con política write-back**, no una RAM antigua; además muestra contadores de ciclos, bundles, instrucciones, stalls y lecturas/escrituras.

**No se debe afirmar que todas las optimizaciones produjeron una transformación real solo porque se activaron sus banderas.** En particular, `--spill-ir` puede no introducir LOAD/STORE si el programa no genera presión suficiente. Las tres pruebas dedicadas de spilling ejercitan esa condición por separado.

## 10.4 Entorno compatible y ejecución

Desde `proyecto-1/src`:

```bash
# Usar el clon de Arquitectura que contenga los módulos y los puertos esperados
make test-rtl-actual ARQ_ROOT="$HOME/TEC/Arqui1/Proyecto2/Arquitectura1-IIS2026"
```

`ARQ_ROOT` por defecto en el Makefile es una ruta de otra máquina. **Siempre** es preferible fijarla explícitamente. Se exige `python/assembler.py` y un `src/top.sv` con las interfaces utilizadas por el testbench. Si `main` de Arquitectura aún no tiene la versión de `feature-integracion-metricas-final`, el test puede fallar por desajuste entre ramas. No se debe modificar el enunciado para ocultarlo.

## 10.5 Límites y reproducibilidad

Los tests de compilador validan una relación software→modelo funcional; el test RTL intenta agregar el procesador y las cachés. Deben registrar en la entrega:

- Hash del commit de Compiladores y de Arquitectura.
- Orden y parámetros de las optimizaciones.
- Archivo fuente ejecutado y valor esperado.
- Resultado del simulador funcional y del testbench RTL.
- Contadores de ciclos, stalls, L1/L2/memoria según los realmente exponga la simulación.

Si la integración RTL no se ha ejecutado, escribir **«script implementado, pendiente de validación en la rama integrada»**.
