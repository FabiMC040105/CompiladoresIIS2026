CC := gcc
FLEX := flex
BISON := bison

TARGET := compilador
LEXER := lexer.l
PARSER := parser.y

CFLAGS := -g -O0
BISONFLAGS := -d 

.RECIPEPREFIX := >

.PHONY: all run clean rebuild

all: $(TARGET)

# Generar parser.tab.c y parser.tab.h
parser.tab.c: $(PARSER)
>$(BISON) $(BISONFLAGS) -o $@ $<

# Generar lex.yy.c
lex.yy.c: $(LEXER) parser.tab.c
>$(FLEX) -o $@ $(LEXER)

# Compilar lexer y parser juntos
$(TARGET): parser.tab.c lex.yy.c
>$(CC) $(CFLAGS) -o $@ parser.tab.c lex.yy.c -lfl

# Ejecutar un archivo de prueba
run: $(TARGET)
>./$(TARGET) < $(FILE)

# Recompilar desde cero
rebuild:
>$(MAKE) clean
>$(MAKE) all

# Eliminar archivos generados
clean:
>rm -f $(TARGET) $(TARGET).exe parser.tab.c parser.tab.h lex.yy.c
