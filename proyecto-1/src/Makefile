
CC := gcc
FLEX := flex
BISON := bison

TARGET := cdff
LEXER := lexer.l
PARSER := parser.y

CFLAGS := -Wall -Wextra -g -O0
BISONFLAGS := -d -Wall

FILE ?= factorial.e
ARGS ?=

ENTRADA ?=
SALIDA ?=

.PHONY: all help run clean rebuild resolver arbol ast test \
        test-etiquetas test-resolucion test-integracion-etiquetas \
        test-binario

all: $(TARGET) resolver_asm

# Mostrar ayuda de los comandos disponibles.
help:
	@printf '%s\n' \
	  '==================================================' \
	  ' COMPILADORES - COMANDOS DISPONIBLES' \
	  '==================================================' \
	  '' \
	  'Compilacion:' \
	  '  make                  Compila cdff y resolver_asm' \
	  '  make rebuild          Limpia y vuelve a compilar' \
	  '  make clean            Elimina archivos generados' \
	  '' \
	  'Ejecucion del compilador:' \
	  '  make run FILE=factorial.e' \
	  '  make run FILE=factorial.e ARGS="-v"' \
	  '  make run FILE=factorial.e ARGS="-v -m"' \
	  '  make run FILE=factorial.e ARGS="-t"' \
	  '  make run FILE=factorial.e ARGS="-s -o factorial.bin"' \
	  '  make run FILE=factorial.e ARGS="-p"' \
	  '  ./cdff -x programa.asm -o programa.bin' \
	  '' \
	  'Pruebas de etiquetas y saltos:' \
	  '  make test-etiquetas' \
	  '  make test-resolucion' \
	  '  make test' \
	  '' \
	  'Resolver etiquetas de un ensamblador:' \
	  '  make resolver ENTRADA=programa.asm SALIDA=programa_resuelto.asm' \
	  '' \
	  'Visualizar el AST con Graphviz:' \
	  '  dot -Tpng arbol.dot -o arbol.png' \
	  '' \
	  'Opciones implementadas en cdff:' \
	  '  -v -t -m -o -s -x -p' \
	  '' \
	  'Use ./cdff --help para ver la descripcion completa.'

# Generar parser con Bison.
parser.tab.c: $(PARSER) ast.h tabla_simbolos.h semantico.h \
              codegen.h codegen_tabla.h scheduler.h \
              resolucion_etiquetas.h etiquetas.h binario.h
	$(BISON) $(BISONFLAGS) -o $@ $(PARSER)

# Generar lexer con Flex.
lex.yy.c: $(LEXER) parser.tab.c parser.tab.h ast.h
	$(FLEX) -o $@ $(LEXER)

# Compilar el compilador.
$(TARGET): parser.tab.c lex.yy.c ast.c ast.h \
           tabla_simbolos.c tabla_simbolos.h \
           memoria.c memoria.h \
           semantico.c semantico.h \
           asm_emitter.c asm_emitter.h \
           scheduler.c scheduler.h \
           codegen.c codegen.h \
           codegen_tabla.c codegen_tabla.h \
           resolucion_etiquetas.c resolucion_etiquetas.h \
           etiquetas.c etiquetas.h \
           binario.c binario.h
	$(CC) $(CFLAGS) -o $@ \
	    parser.tab.c lex.yy.c ast.c \
	    tabla_simbolos.c memoria.c semantico.c \
	    asm_emitter.c scheduler.c codegen.c codegen_tabla.c \
	    resolucion_etiquetas.c etiquetas.c binario.c

# Compilar el programa auxiliar de resolucion de etiquetas.
resolver_asm: resolver_asm_main.c \
              resolucion_etiquetas.c resolucion_etiquetas.h \
              etiquetas.c etiquetas.h
	$(CC) $(CFLAGS) -o $@ \
	    resolver_asm_main.c resolucion_etiquetas.c etiquetas.c

# Ejecutar el compilador con el archivo indicado.
run: $(TARGET)
	./$(TARGET) $(ARGS) $(FILE)


# Generar el AST, convertirlo a PNG y abrir la imagen
arbol: $(TARGET)
	./$(TARGET) -t "$(FILE)"
	@command -v dot >/dev/null 2>&1 || { echo "Error: Graphviz no esta instalado."; exit 1; }
	dot -Tpng arbol.dot -o arbol.png
	@if command -v explorer.exe >/dev/null 2>&1; then \
		explorer.exe "$$(wslpath -w arbol.png)"; \
	elif command -v xdg-open >/dev/null 2>&1; then \
		xdg-open arbol.png >/dev/null 2>&1 & \
	else \
		echo "Imagen generada en arbol.png; abre ese archivo manualmente."; \
	fi

ast: arbol

# Resolver las etiquetas de un ensamblador ya calendarizado.
resolver: resolver_asm
	@test -n "$(ENTRADA)" || \
	    { echo "Uso: make resolver ENTRADA=programa.asm SALIDA=resuelto.asm"; exit 2; }
	@test -n "$(SALIDA)" || \
	    { echo "Error: debe indicar SALIDA=archivo.asm"; exit 2; }
	./resolver_asm "$(ENTRADA)" "$(SALIDA)"

# Pruebas unitarias de la tabla de etiquetas.
prueba_etiquetas: prueba_etiquetas.c etiquetas.c etiquetas.h
	$(CC) $(CFLAGS) -o $@ prueba_etiquetas.c etiquetas.c

test-etiquetas: prueba_etiquetas
	./prueba_etiquetas

# Pruebas de integracion de la resolucion de saltos en ensamblador.
prueba_resolucion_asm: prueba_resolucion_asm.c \
                       resolucion_etiquetas.c resolucion_etiquetas.h \
                       etiquetas.c etiquetas.h
	$(CC) $(CFLAGS) -o $@ \
	    prueba_resolucion_asm.c resolucion_etiquetas.c etiquetas.c

test-resolucion: prueba_resolucion_asm
	./prueba_resolucion_asm

# Alias descriptivo para las pruebas de integracion de etiquetas.
test-integracion-etiquetas: test-resolucion

# Ejecutar ambas suites de pruebas.
test: test-etiquetas test-resolucion test-binario

# Prueba del codificador binario integrado.
prueba_binario: prueba_binario.c binario.c binario.h
	$(CC) $(CFLAGS) -o $@ prueba_binario.c binario.c

# Ejecutar la prueba del ensamblador/binario.
test-binario: prueba_binario
	./prueba_binario

# Reconstruir los ejecutables.
rebuild:
	$(MAKE) clean
	$(MAKE) all

# Eliminar ejecutables y archivos intermedios.
# Se conservan los archivos .asm y .bin para no borrar resultados utiles.
clean:
	rm -f cdff cdff.exe compilador compilador.exe resolver_asm \
	      prueba_etiquetas prueba_resolucion_asm prueba_binario \
	      parser.tab.c parser.tab.h lex.yy.c \
	      prueba_etiquetas_entrada.tmp prueba_etiquetas_resuelta.tmp