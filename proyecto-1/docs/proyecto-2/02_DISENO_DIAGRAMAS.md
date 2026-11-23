# 2. Diseño del programa y diagramas

## 2.1 Arquitectura funcional

El sistema tiene dos componentes principales: el compilador y el IDE web. El compilador usa el frontend Flex/Bison, un árbol de sintaxis abstracta (AST), análisis semántico e IR de tres direcciones. La generación final puede usar el backend tradicional desde AST o, cuando se selecciona explícitamente, el backend desde IR.

```mermaid
flowchart TD
    FU[Archivo fuente .e] --> LEX[Lexer Flex + parser Bison]
    LEX --> AST[AST + tabla de símbolos]
    AST --> SEM[Análisis semántico]
    SEM --> IR{Se solicita IR, CFG u optimización?}
    IR -->|Sí| IR1[Generar IR para inspección]
    IR1 --> OPT1[Aplicar fases seleccionadas]
    OPT1 --> MOSTRAR[Mostrar IR y/o CFG]
    MOSTRAR --> CFGSTOP{--ir-cfg?}
    CFGSTOP -->|Sí| FINCFG[Finaliza sin ensamblar]
    CFGSTOP -->|No| BE{--codegen-ir?}
    IR -->|No| BE
    BE -->|No| ASTBE[Backend AST tradicional]
    BE -->|Sí| IR2[Regenerar IR desde AST]
    IR2 --> OPT2[Aplicar las mismas fases IR]
    OPT2 --> IRBE[codegen_ir.c: ASM desde IR]
    ASTBE --> ET[Resolver etiquetas]
    IRBE --> ET
    ET --> PACK[Empaquetar slots VLIW]
    PACK --> BIN[Generar .bin]
```

**Distinción importante:** en `parser.y` hay una IR para inspección y otra generada de nuevo cuando se selecciona `--codegen-ir`. Ambas se optimizan con `opt_ir_aplicar_config()`, por lo que `--unroll` y `--spill-ir` **sí llegan al backend IR** en esta revisión. Sin `--codegen-ir`, las fases de optimización IR no sustituyen al backend tradicional; la bandera `--spill` además habilita un mecanismo propio del backend AST. `--ir-cfg` imprime el CFG y termina sin emitir ensamblador, según la lógica actual.

**Nuevo en `fix-integracion-final`:** `pruebas/probar_integracion_rtl_actual.sh` propone la ruta completa `.e → IR optimizada → ASM → binario VLIW → RTL`; `pruebas/tb_integracion_rtl_actual.sv` observa el dato esperado en L2. La existencia de ese test no sustituye ejecutarlo en el RTL compatible.

## 2.2 Organización de módulos

| Componente | Archivos principales | Responsabilidad |
|---|---|---|
| Frontend y AST | `lexer.l`, `parser.y`, `ast.c` | Leer, analizar y estructurar el programa. |
| Semántica | `semantico.c`, `tabla_simbolos.c` | Validar tipos, declaraciones, funciones y ámbitos. |
| IR | `ir.c`, `ir_ast.c` | Mantener secuencia de instrucciones de tres direcciones. |
| CFG | `basic_blocks.c` | Delimitar bloques y relaciones de flujo. |
| Variables vivas | `liveness.c` | Calcular información USE/DEF e IN/OUT. |
| DCE | `dead_code_elimination.c` | Eliminar cálculos innecesarios conservando efectos relevantes. |
| Loop unrolling | `loop_unrolling.c` | Duplicar cuerpos de ciclos admitidos con factor y límites. |
| Dependencias/DAG | `dependencias.c`, `dag.c` | Modelar dependencias y regiones seguras. |
| Renombramiento | `renombramiento.c` | Cambiar nombres de temporales/definiciones admitidas. |
| Reordenamiento | `reordenamiento.c` | Reordenar operaciones respetando dependencias y barreras. |
| Spilling | `grafo_interferencia.c`, `asignacion_registros.c`, `spill_transform.c` | Detectar presión, colorear y reescribir accesos temporales. |
| Coordinador | `optimizaciones_ir.c` | Aplicar fases en orden sobre una copia del IR. |
| Backend | `codegen.c`, `codegen_ir.c`, `scheduler.c`, `vliw_packer.c`, `binario.c` | Traducir, calendarizar, empaquetar y codificar. |
| IDE | `ide/app.py`, `ide/index.html`, `ide/ide_*.c`, `ide/ll1_predict.c` | Edición, sugerencias, errores, clones y compilación. |
| Pruebas y medición | `pruebas/medir_suite_optimizaciones.py`, `pruebas/probar_importaciones.sh`, `pruebas/probar_integracion_rtl_actual.sh` | Comparar código base/optimizado, comprobar resultados y ejecutar integración con CE4301. |

## 2.3 Diagrama UML de clases/estructuras principales

Las estructuras en C representan módulos de datos y APIs, no clases con herencia. Este UML simplificado muestra sus relaciones conceptuales sin atribuir métodos no existentes.

```mermaid
classDiagram
    class IRProgram {
      +IRInstruction[] instructions
      +size_t count
      +size_t capacity
      +size_t next_temp
      +size_t next_label
    }
    class IRInstruction {
      +IROp op
      +char* arg1
      +char* arg2
      +char* result
      +char* extra
      +int source_line
      +NodoAST* nodo_origen
    }
    class BasicBlockCFG {
      +bloques basicos
      +aristas CFG
    }
    class OptIREstadisticas {
      +fases_ejecutadas
      +instrucciones_antes
      +instrucciones_despues
      +dce
      +renombramiento
      +reordenamiento
      +spilling
      +unrolling
    }
    class GrafoInterferencia {
      +nodos temporales
      +matriz de adyacencia
    }
    IRProgram "1" *-- "muchas" IRInstruction
    IRProgram ..> BasicBlockCFG : bb_build
    IRProgram ..> GrafoInterferencia : analisis de liveness
    OptIREstadisticas ..> IRProgram : mide transformaciones
```

También se incluye el código fuente editable en [diagrama_clases.puml](diagramas/diagrama_clases.puml).

## 2.4 Diagrama UML de secuencia: compilación desde el IDE

```mermaid
sequenceDiagram
    actor U as Usuario
    participant W as index.html
    participant H as app.py
    participant C as cdff
    participant D as Disco
    U->>W: Escribe y solicita compilar
    W->>H: POST /api/compile
    H->>D: Guarda archivo .e en workspace
    H->>C: Ejecuta cdff -s --emit-ir -o salida.bin archivo.e
    C-->>H: Estado, diagnósticos e IR
    H->>D: Consulta .asm/.bin generados
    H-->>W: JSON con resultado y artefactos
    W-->>U: Muestra errores, IR y archivos
```

[Secuencia en PlantUML](diagramas/secuencia_compilacion.puml).

## 2.5 Diagrama de funcionalidades del IDE

```mermaid
flowchart LR
    U[Usuario del IDE] --> E[Editar archivo .e]
    E --> A[Autocompletado LL1 y símbolos]
    E --> D[Verificar sintaxis]
    D --> R[Aplicar sugerencia segura o NO_FIX]
    E --> CL[Analizar clones Tipo I/II]
    E --> G[Guardar archivo]
    E --> C[Compilar con cdff]
    C --> I[Consultar IR y mensajes]
    C --> P[Ver o descargar .asm/.bin]
```

## 2.6 Restricciones de diseño relevantes

- En el VLIW, el empaquetamiento debe respetar cuatro slots de 32 bits (`ALU`, `LSU`, `BRU`, `FEIST`) y llenar los no utilizados con ceros.
- El hardware no realiza calendarización dinámica de dependencias entre bundles; el compilador debe producir un orden seguro e insertar NOP cuando corresponda.
- El IDE ejecuta `cdff` como proceso independiente. Su analizador LL(1) auxiliar está orientado a sugerencias/correcciones, y **no sustituye** la validación del parser Flex/Bison del compilador.
- El detector de clones opera desde el IDE y puede comparar archivos diferentes; el servidor presenta los resultados a la interfaz.
- Los detalles del hardware y sus contadores no pueden darse por comprobados a partir de los tests unitarios de IR.

## 2.7 Diagrama de integración con CE4301

El archivo [integracion_pruebas.puml](diagramas/integracion_pruebas.puml) separa el compilador, el simulador de ensamblador y el testbench del RTL externo de Arquitectura. Esta separación ayuda a identificar qué pruebas pueden ejecutarse sin repositorio de hardware y cuáles requieren código de ambas materias.
