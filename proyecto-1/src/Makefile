
CC := gcc
FLEX := flex
BISON := bison

TARGET := compilador
LEXER := lexer.l
PARSER := parser.y

CFLAGS := -Wall -Wextra -g -O0
BISONFLAGS := -d -Wall

.PHONY: all run clean rebuild resolver test-etiquetas test-resolucion test-integracion-etiquetas

# Archivo de prueba por defecto
FILE ?= factorial.e
ARGS ?=

all: $(TARGET) resolver_asm

# Generar parser con Bison
parser.tab.c: $(PARSER) ast.h tabla_simbolos.h semantico.h codegen.h codegen_tabla.h scheduler.h
	$(BISON) $(BISONFLAGS) -o $@ $(PARSER)

# Generar lexer con Flex
lex.yy.c: $(LEXER) parser.tab.c parser.tab.h ast.h
	$(FLEX) -o $@ $(LEXER)

# Compilar todos los componentes del compilador
$(TARGET): parser.tab.c lex.yy.c ast.c ast.h \
           tabla_simbolos.c tabla_simbolos.h \
           memoria.c memoria.h \
           semantico.c semantico.h \
           asm_emitter.c asm_emitter.h \
           scheduler.c scheduler.h \
           codegen.c codegen.h \
           codegen_tabla.c codegen_tabla.h
	$(CC) $(CFLAGS) -o $@ \
	    parser.tab.c lex.yy.c ast.c \
	    tabla_simbolos.c memoria.c semantico.c \
	    asm_emitter.c scheduler.c codegen.c codegen_tabla.c

# Resolver etiquetas del ensamblador YA calendarizado (sin cambiar los NOPs)
resolver_asm: resolver_asm_main.c resolucion_etiquetas.c resolucion_etiquetas.h etiquetas.c etiquetas.h
	$(CC) $(CFLAGS) -std=c11 -o $@ resolver_asm_main.c resolucion_etiquetas.c etiquetas.c

resolver: resolver_asm
	./resolver_asm salida.asm salida_resuelta.asm

# Ejecutar el compilador con un archivo y resolver las etiquetas
run: $(TARGET) resolver_asm
	./$(TARGET) $(ARGS) "$(FILE)"
	./resolver_asm "$(basename $(FILE)).asm" salida_resuelta.asm

# Pruebas antiguas de nuestra tabla de etiquetas
test-etiquetas:
	$(CC) $(CFLAGS) -std=c11 -o prueba_etiquetas etiquetas.c prueba_etiquetas.c
	./prueba_etiquetas

# Reconstruir el proyecto
rebuild:
	$(MAKE) clean
	$(MAKE) all

# Eliminar archivos generados
clean:
	rm -f $(TARGET) $(TARGET).exe resolver_asm prueba_etiquetas prueba_resolucion_asm prueba_codegen_integracion parser.tab.c parser.tab.h lex.yy.c salida.asm salida_resuelta.asm

# Verifica las dos pasadas directamente sobre texto VLIW calendarizado.
test-resolucion: resolver_asm
	$(CC) $(CFLAGS) -std=c11 -o prueba_resolucion_asm prueba_resolucion_asm.c resolucion_etiquetas.c etiquetas.c
	./prueba_resolucion_asm

# Reutiliza el test del codegen actual, sin parser y sin tocar sus producciones.
test-integracion-etiquetas: resolver_asm
	$(CC) $(CFLAGS) -std=c11 -o prueba_codegen_integracion test_codegen_integracion.c codegen.c codegen_tabla.c ast.c tabla_simbolos.c memoria.c asm_emitter.c scheduler.c
	./prueba_codegen_integracion
	./resolver_asm salida_codegen_integracion.asm salida_codegen_integracion_resuelta.asm
