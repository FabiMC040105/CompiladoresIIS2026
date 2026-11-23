# 11. Registro de verificación de esta revisión

## 11.1 Material verificado

Archivo analizado: `CompiladoresIIS2026-fix-integracion-final(1).zip`. Se comparó con `CompiladoresIIS2026-integration-ir-optimizaciones-local (1).zip` por hash y ruta interna, y se revisaron módulos C, scripts Shell/Python y programas `.e` del avance nuevo.

Los comandos se ejecutaron en una **copia extraída del ZIP**, no sobre la rama `main` del equipo. Las pruebas que no necesitan `cdff` se compilaron sobre la fuente C que viene en el ZIP.

## 11.2 Pruebas ejecutadas aquí

Desde `proyecto-1/src`:

```bash
make test-loop-unrolling test-opt-modulos test-grafo-spill test-spill
```

**Resultado: PASS** (exit 0). La salida incluye `PASS: Loop Unrolling y equivalencia de resultados`, `PASS integracion de optimizaciones IR`, pruebas de grafo/spill, renombramiento, reordenamiento y regresiones DCE. Los valores y el número de casos son los de los tests particulares, no representan todavía la suite `.e` de quince programas.

Desde la raíz del ZIP:

```bash
python3 -m unittest discover -s ide/tests -p 'test_*.py'
node ide/tests/test_frontend.js
```

**Resultado: PASS.** Python ejecutó **58 tests** (OK) y JavaScript reportó **6 comprobaciones frontend LL(1) correctas**.

## 11.3 Qué no se ejecutó en esta revisión

No se encontraron los ejecutables **Flex** ni **Bison**, ni un binario `cdff` precompilado en el ZIP. Por ello no se pudo reconstruir el frontend ni ejecutar los objetivos que dependen del ejecutable:

- `make test-codegen-ir-backend`
- `make test-importaciones`
- `make test-spill-obligatorio` / `make test-spill-completo`
- `make test-suite-optimizaciones` / `make metricas-optimizaciones`
- `make test-entrega-local`

Tampoco se ejecutó `make test-rtl-actual`, que requiere la implementación compatible de Arquitectura (Icarus Verilog y su repositorio externo). La existencia de los scripts **no demuestra** que esos tests produzcan PASS en la WSL del grupo.

## 11.4 Comprobación reproducible del grupo

```bash
cd ~/TEC/Compi/CompiladoresIIS2026/proyecto-1/src  # cambiar por ruta real
sudo apt install -y build-essential flex bison python3 iverilog
make rebuild
make test-entrega-local |& tee prueba_entrega_local.log
make test-rtl-actual ARQ_ROOT="$HOME/TEC/Arqui1/Proyecto2/Arquitectura1-IIS2026" |& tee prueba_rtl.log
```

La ruta inicial es solo ejemplo. Ejecutar sobre la **rama y commit correctos**. Un PASS obtenido con el código de una rama no debe atribuirse automáticamente a `main` si aún no se ha integrado.
