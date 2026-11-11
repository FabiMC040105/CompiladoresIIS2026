#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdio.h>
#include "ast.h"

/* IR de tres direcciones, ojo que los nombres internos %tN y @LN no son
   identificadores validos del lenguaje fuente y no colisionan con el usuario. */
typedef enum {
    IR_DECL,
    IR_COPY, IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD, IR_POW, IR_NEG,
    IR_EQ, IR_NE, IR_LT, IR_LE, IR_GT, IR_GE,
    IR_LABEL, IR_GOTO, IR_IF_FALSE, IR_IF_TRUE,
    IR_FUNC_BEGIN, IR_FUNC_END, IR_PARAM_DECL, IR_PARAM, IR_CALL, IR_RETURN,
    IR_ARRAY_DECL, IR_ARRAY_LOAD, IR_ARRAY_STORE,
    IR_MATRIX_DECL, IR_MATRIX_LOAD, IR_MATRIX_STORE
} IROp; /* Que operación queremos realizar */

/* Cada instruccion utiliza los campos segun su
   operacion, por ejemplo, una suma necesita dos
   operandos y un resultado, mientras que un salto necesita
   una etiqueta. Los campos no utilizados pueden ser NULL,
   IRProgram administra la memoria de las
   cadenas que almacena. */
typedef struct {
    IROp op;
    char *arg1;
    char *arg2;
    char *result;
    char *extra;
    int source_line;
} IRInstruction; /* Instruccion individual */

typedef struct {
    IRInstruction *instructions; /* Puntero que podrá apuntar a un arreglo de instrucciones */
    size_t count;
    size_t capacity;
    size_t next_temp;
    size_t next_label;
} IRProgram; /* El conjunto ordenado de instrucciones o sea la secuencia completa*/

typedef struct {
    int line;
    char message[256];
} IRError; /* Me permite reportar errores de generación */

void ir_init(IRProgram *p); /* Inicializa un programa IR vacío */
void ir_free(IRProgram *p); /* Libera la memoria reservada */
int ir_emit(IRProgram *p, IROp op, const char *arg1, const char *arg2,
            const char *result, int source_line); /* Añade una instrucción al programa */
int ir_emit_ex(IRProgram *p, IROp op, const char *arg1, const char *arg2,
               const char *result, const char *extra, int source_line); /* Añade una instrucción con un campo extra*/
char *ir_new_temp(IRProgram *p);  /* Genera un nombre temporal único dentro del programa */
char *ir_new_label(IRProgram *p); /* Genera una nueva etiqueta */
void ir_print(const IRProgram *p, FILE *out); /* Imprime las instruciones para poder visualizarlas*/

int ir_generate(const NodoAST *root, IRProgram *out, IRError *error); /* Recorre el AST y produce la IR */

#endif
