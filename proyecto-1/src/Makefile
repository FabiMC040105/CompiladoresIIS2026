# ================================================================
# Proyecto 1 - Compiladores e Interpretes (cdff + VLIW)
# Ejecutar desde proyecto-1/src con GNU Make >= 4.3
# ================================================================

.DEFAULT_GOAL := all

CC         := gcc
FLEX       := flex
BISON      := bison
PYTHON     := python3

CFLAGS     ?= -Wall -Wextra -g -O0
CPPFLAGS   ?=
LDFLAGS    ?=
LDLIBS     ?=
BISONFLAGS ?= -d -Wall

TARGET     := cdff
RESOLVER   := resolver_asm
LEXER      := lexer.l
PARSER     := parser.y

# Las carpetas de resultados estan ignoradas por el .gitignore actual.
RESULTADOS := evidencia-vliw/salidas-make
TEST_DIR   := evidencia-vliw/.make-tests

FILE       ?= factorial.e
ASM        ?= demo_vliw.asm
ARGS       ?=
OUT        ?=
ENTRADA    ?=
SALIDA     ?=
BIN        ?= demo_vliw.bin
VCD        ?= ondas.vcd
# El repositorio de Arquitectura puede encontrarse en otra carpeta.
ARQ_ROOT   ?= /mnt/c/Users/fmoreno/Desktop/Arquitectura1-IIS2026

SRC := ast.c tabla_simbolos.c memoria.c semantico.c \
       asm_emitter.c scheduler.c codegen.c codegen_tabla.c \
       resolucion_etiquetas.c etiquetas.c binario.c vliw_packer.c

HEADERS := ast.h tabla_simbolos.h memoria.h semantico.h \
           asm_emitter.h scheduler.h codegen.h codegen_tabla.h \
           resolucion_etiquetas.h etiquetas.h binario.h vliw_packer.h

.PHONY: all help check check-arqui versions run bin asm ensamblar \
        simbolos arbol ast resolver wave check-bin \
        test test-etiquetas test-resolucion test-integracion-etiquetas \
        test-binario test-vliw test-scheduler test-semantico \
        test-integracion test-feist-auto sim test-completo \
        clean rebuild

all: $(TARGET) $(RESOLVER)

# Reglas agrupadas: Bison genera ambos archivos UNA sola vez, tambien con make -j.
parser.tab.c parser.tab.h &: $(PARSER) $(HEADERS)
	$(BISON) $(BISONFLAGS) -o parser.tab.c $(PARSER)

lex.yy.c: $(LEXER) parser.tab.h ast.h
	$(FLEX) -o $@ $(LEXER)

# Compilador completo: lexer, parser, AST, tabla, semantica,
# generacion de ASM, saltos, scheduler, VLIW y binario.
$(TARGET): parser.tab.c parser.tab.h lex.yy.c $(SRC) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
	    parser.tab.c lex.yy.c $(SRC) $(LDFLAGS) $(LDLIBS)

$(RESOLVER): resolver_asm_main.c resolucion_etiquetas.c resolucion_etiquetas.h \
             etiquetas.c etiquetas.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
	    resolver_asm_main.c resolucion_etiquetas.c etiquetas.c $(LDFLAGS) $(LDLIBS)

# ---------------------------------------------------------------
# Ejecucion del compilador y utilidades de la CLI
# ---------------------------------------------------------------
run: $(TARGET)
	@test -f "$(FILE)" || { echo "ERROR: no existe $(FILE)" >&2; exit 2; }
	./$(TARGET) $(ARGS) "$(FILE)"

# Deja el binario por defecto en una carpeta ignorada por Git.
bin: $(TARGET)
	@test -f "$(FILE)" || { echo "ERROR: no existe $(FILE)" >&2; exit 2; }
	@mkdir -p "$(RESULTADOS)"
	./$(TARGET) $(ARGS) -o "$(if $(strip $(OUT)),$(OUT),$(RESULTADOS)/$(basename $(notdir $(FILE))).bin)" "$(FILE)"

# Genera binario y conserva ensamblador .asm (comportamiento de cdff -s).
asm: $(TARGET)
	@test -f "$(FILE)" || { echo "ERROR: no existe $(FILE)" >&2; exit 2; }
	@mkdir -p "$(RESULTADOS)"
	./$(TARGET) -s $(ARGS) -o "$(if $(strip $(OUT)),$(OUT),$(RESULTADOS)/$(basename $(notdir $(FILE))).bin)" "$(FILE)"

# Ensambla directamente un .asm; NO necesita pasar por Flex/Bison en ejecucion.
ensamblar: $(TARGET)
	@test -f "$(ASM)" || { echo "ERROR: no existe $(ASM)" >&2; exit 2; }
	@mkdir -p "$(RESULTADOS)"
	./$(TARGET) -x "$(ASM)" -o "$(if $(strip $(OUT)),$(OUT),$(RESULTADOS)/$(basename $(notdir $(ASM))).bin)" $(ARGS)

simbolos: $(TARGET)
	@test -f "$(FILE)" || { echo "ERROR: no existe $(FILE)" >&2; exit 2; }
	@mkdir -p "$(RESULTADOS)"
	./$(TARGET) -m $(ARGS) -o "$(RESULTADOS)/$(basename $(notdir $(FILE))).bin" "$(FILE)"

arbol: $(TARGET)
	@command -v dot >/dev/null 2>&1 || { echo 'ERROR: instalar graphviz (dot).' >&2; exit 2; }
	@test -f "$(FILE)" || { echo "ERROR: no existe $(FILE)" >&2; exit 2; }
	@mkdir -p "$(RESULTADOS)"
	./$(TARGET) -t $(ARGS) -o "$(RESULTADOS)/$(basename $(notdir $(FILE))).bin" "$(FILE)"
	dot -Tpng arbol.dot -o "$(RESULTADOS)/arbol.png"
	@mv arbol.dot "$(RESULTADOS)/arbol.dot"
	@echo "AST: $(RESULTADOS)/arbol.png"
	@if command -v explorer.exe >/dev/null 2>&1 && command -v wslpath >/dev/null 2>&1; then \
	    explorer.exe "$$(wslpath -w "$(RESULTADOS)/arbol.png")"; \
	elif command -v xdg-open >/dev/null 2>&1; then \
	    xdg-open "$(RESULTADOS)/arbol.png" >/dev/null 2>&1 & \
	fi

ast: arbol

resolver: $(RESOLVER)
	@test -n "$(ENTRADA)" || { echo 'ERROR: indique ENTRADA=archivo.asm' >&2; exit 2; }
	@test -n "$(SALIDA)"  || { echo 'ERROR: indique SALIDA=resuelto.asm' >&2; exit 2; }
	@test -f "$(ENTRADA)" || { echo "ERROR: no existe $(ENTRADA)" >&2; exit 2; }
	./$(RESOLVER) "$(ENTRADA)" "$(SALIDA)"

# Validar que cada linea de un .bin es una palabra hex de 128 bits (32 digitos).
check-bin:
	@$(PYTHON) -c 'import pathlib,re,sys; p=pathlib.Path(sys.argv[1]); rows=p.read_text().splitlines(); ok=bool(rows) and all(re.fullmatch(r"[0-9a-fA-F]{32}",s) for s in rows); print(str(p)+": "+str(len(rows))+" bundles VLIW de 128 bits; formato: "+("OK" if ok else "INVALIDO")); sys.exit(0 if ok else 1)' "$(BIN)"

# Abre un archivo de ondas ya existente; la prueba RTL debe generarlo.
wave:
	@command -v gtkwave >/dev/null 2>&1 || { echo 'ERROR: falta gtkwave.' >&2; exit 2; }
	@test -f "$(VCD)" || { echo "ERROR: no existe $(VCD). Indique VCD=ruta.vcd" >&2; exit 2; }
	gtkwave "$(VCD)"

# ---------------------------------------------------------------
# Pruebas C (en directorio ignorado, sin pisar binarios versionados)
# ---------------------------------------------------------------
$(TEST_DIR):
	@mkdir -p "$@"

$(TEST_DIR)/prueba_etiquetas: prueba_etiquetas.c etiquetas.c etiquetas.h | $(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ prueba_etiquetas.c etiquetas.c $(LDFLAGS) $(LDLIBS)

test-etiquetas: $(TEST_DIR)/prueba_etiquetas
	./$(TEST_DIR)/prueba_etiquetas

$(TEST_DIR)/prueba_resolucion_asm: prueba_resolucion_asm.c \
                                  resolucion_etiquetas.c resolucion_etiquetas.h \
                                  etiquetas.c etiquetas.h | $(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
	    prueba_resolucion_asm.c resolucion_etiquetas.c etiquetas.c $(LDFLAGS) $(LDLIBS)

test-resolucion: $(TEST_DIR)/prueba_resolucion_asm
	./$(TEST_DIR)/prueba_resolucion_asm

test-integracion-etiquetas: test-resolucion

$(TEST_DIR)/prueba_binario: prueba_binario.c binario.c binario.h | $(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ prueba_binario.c binario.c $(LDFLAGS) $(LDLIBS)

test-binario: $(TEST_DIR)/prueba_binario
	./$(TEST_DIR)/prueba_binario

$(TEST_DIR)/prueba_vliw_packer: prueba_vliw_packer.c vliw_packer.c vliw_packer.h \
                                scheduler.c scheduler.h | $(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
	    prueba_vliw_packer.c vliw_packer.c scheduler.c $(LDFLAGS) $(LDLIBS)

test-vliw: $(TEST_DIR)/prueba_vliw_packer
	./$(TEST_DIR)/prueba_vliw_packer

$(TEST_DIR)/test_scheduler: test_scheduler.c scheduler.c scheduler.h | $(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ test_scheduler.c scheduler.c $(LDFLAGS) $(LDLIBS)

test-scheduler: $(TEST_DIR)/test_scheduler
	./$(TEST_DIR)/test_scheduler

# 34 pruebas de sintaxis y semantica (script ya incluido en el proyecto).
test-semantico: $(TARGET)
	bash ./probar_semantico.sh

# Suite local, no depende de instalar/descargar Arquitectura.
test: all test-etiquetas test-resolucion test-binario test-vliw \
      test-scheduler test-semantico
	@echo 'PASS: finalizaron las pruebas locales.'

# ---------------------------------------------------------------
# Integracion REAL con la arquitectura (requiere repo externo)
# ---------------------------------------------------------------
check-arqui:
	@for t in $(PYTHON) bash iverilog vvp; do \
	    command -v "$$t" >/dev/null 2>&1 || { echo "ERROR: falta $$t" >&2; exit 2; }; \
	done
	@test -d "$(ARQ_ROOT)/src" || { echo "ERROR: ARQ_ROOT invalido: $(ARQ_ROOT)" >&2; exit 2; }
	@test -f "$(ARQ_ROOT)/python/assembler.py" || { echo 'ERROR: falta python/assembler.py en Arquitectura' >&2; exit 2; }

# Incluye FEIST 4+4, compilacion .e, ASM, paquetes y pruebas RTL.
test-integracion: all check-arqui
	ARQ_ROOT="$(ARQ_ROOT)" bash ./validar_feist_4r.sh

# Validacion aislada de los cuatro slots simultaneos.
test-feist-auto: all check-arqui
	ARQ_ROOT="$(ARQ_ROOT)" bash ./validar_feist_auto.sh

sim: test-integracion

test-completo: test test-integracion test-feist-auto
	@echo 'PASS: suites local y de arquitectura completadas.'

# ---------------------------------------------------------------
# Diagnostico, limpieza y ayuda
# ---------------------------------------------------------------
check:
	@for t in $(CC) $(FLEX) $(BISON) make bash; do \
	    command -v "$$t" >/dev/null 2>&1 || { echo "ERROR: falta $$t" >&2; exit 2; }; \
	done
	@echo 'OK: herramientas basicas instaladas.'

versions: check
	@$(CC) --version | head -n 1
	@$(FLEX) --version
	@$(BISON) --version | head -n 1
	@make --version | head -n 1

# Nunca elimina los .asm/.bin de ejemplo ni los ejecutables rastreados por Git.
clean:
	rm -f $(TARGET) $(TARGET).exe $(RESOLVER) parser.tab.c parser.tab.h lex.yy.c arbol.dot
	rm -rf "$(TEST_DIR)"
	@echo 'Limpieza completada. Los ejemplos, binarios versionados y evidencias se conservaron.'

rebuild:
	$(MAKE) clean
	$(MAKE) all

help:
	@printf '%s\n' \
	  '==========================================================' \
	  ' PROYECTO 1 - COMPILADOR + ISA VLIW' \
	  '==========================================================' \
	  'Compilacion:' \
	  '  make / make all                  Compilar cdff y resolver_asm' \
	  '  make rebuild                     Limpiar y compilar desde cero' \
	  '  make clean                       Limpiar sin borrar ejemplos' \
	  '  make check                       Verificar gcc, flex, bison, make y bash' \
	  '  make versions                    Ver versiones de herramientas' \
	  '' \
	  'Compilador (.e):' \
	  '  make run FILE=factorial.e' \
	  '  make run FILE=factorial.e ARGS="-v -m"' \
	  '  make bin FILE=factorial.e        Generar .bin en evidencia-vliw' \
	  '  make asm FILE=factorial.e        Generar .bin y conservar .asm' \
	  '  make simbolos FILE=factorial.e   Mostrar tabla de simbolos' \
	  '  make arbol FILE=factorial.e      Generar/abrir AST PNG (Graphviz)' \
	  '  make ensamblar ASM=demo_vliw.asm  Ensamblar .asm a .bin' \
	  '  make bin FILE=factorial.e OUT=salida.bin' \
	  '  make resolver ENTRADA=in.asm SALIDA=out.asm' \
	  '  make check-bin BIN=demo_vliw.bin  Revisar palabras VLIW 128 bits' \
	  '' \
	  'Pruebas locales:' \
	  '  make test                        Todas las pruebas locales' \
	  '  make test-etiquetas              Tabla de etiquetas' \
	  '  make test-resolucion             Saltos y resolucion de etiquetas' \
	  '  make test-binario                Codificador binario' \
	  '  make test-vliw                   Empaquetador VLIW' \
	  '  make test-scheduler              Dependencias y NOPs' \
	  '  make test-semantico              34 pruebas semanticas' \
	  '' \
	  'Integracion RTL (requiere repositorio Arquitectura):' \
	  '  make check-arqui                 Verificar dependencias de Arquitectura' \
	  '  make test-integracion            FEIST 4+4 y regresiones en Icarus' \
	  '  make test-feist-auto             Cuatro slots en paralelo' \
	  '  make sim                         Alias de test-integracion' \
	  '  make test-completo               Pruebas locales + RTL' \
	  '  make wave VCD=ruta/ondas.vcd     Abrir ondas existentes en GTKWave' \
	  '' \
	  'Variables: FILE=..., ASM=..., OUT=..., ARGS=..., ARQ_ROOT=...' \
	  'Ayuda del compilador: ./cdff --help' \
	  '=========================================================='
