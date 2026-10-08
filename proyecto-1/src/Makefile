CC := gcc
FLEX := flex
BISON := bison

TARGET := compilador
LEXER := lexer.l
PARSER := parser.y

CFLAGS := -Wall -Wextra -g -O0
BISONFLAGS := -d -Wall

.PHONY: all run clean rebuild

# Archivo de prueba que se usa si no se indica otro
FILE ?= pru1.txt

all: $(TARGET)

# Generar el parser y su archivo de cabecera
parser.tab.c: $(PARSER) ast.h
	$(BISON) $(BISONFLAGS) -o $@ $(PARSER)

# Generar el lexer usando los tokens de Bison
lex.yy.c: $(LEXER) parser.tab.c parser.tab.h ast.h
	$(FLEX) -o $@ $(LEXER)

# Compilar el lexer, el parser y las funciones del AST
$(TARGET): parser.tab.c lex.yy.c ast.c ast.h
	$(CC) $(CFLAGS) -o $@ parser.tab.c lex.yy.c ast.c

# Ejecutar el compilador con un archivo de prueba
run: $(TARGET)
	./$(TARGET) < $(FILE)

# Recompilar todo desde cero
rebuild:
	$(MAKE) clean
	$(MAKE) all

# Eliminar los archivos generados
clean:
	rm -f $(TARGET) $(TARGET).exe parser.tab.c parser.tab.h lex.yy.c
