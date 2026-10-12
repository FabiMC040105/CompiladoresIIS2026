
CC := gcc
FLEX := flex
BISON := bison

TARGET := compilador
LEXER := lexer.l
PARSER := parser.y

CFLAGS := -Wall -Wextra -g -O0
BISONFLAGS := -d -Wall

.PHONY: all run clean rebuild

# Archivo de prueba por defecto
FILE ?= pru1.txt

all: $(TARGET)

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

# Ejecutar el compilador con un archivo
run: $(TARGET)
	./$(TARGET) < $(FILE)

# Reconstruir el proyecto
rebuild:
	$(MAKE) clean
	$(MAKE) all

# Eliminar archivos generados
clean:
	rm -f $(TARGET) $(TARGET).exe parser.tab.c parser.tab.h lex.yy.c salida.asm
