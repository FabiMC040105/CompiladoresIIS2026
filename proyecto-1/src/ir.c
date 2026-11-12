#include "ir.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* Reserva memoria propia y crea una copia independiente de una cadena 
   char *: la func devuelve un puntero a caracteres, es decir, la dirección donde comienza la copia
   const char *: recibe una cadena que no debe modificar */

static char *ir_copy_text(const char *s) 
{
    size_t n; /* Cantidad de bytes necesarios*/
    char *copy; /* Apunta a la memoria donde se guardará el texto copiado */
    if (!s) return NULL; /* Si s es null, no existe una cadena que copiar, devuelve null*/
    n = strlen(s) + 1; /* cuenta caracteres, y sumamos uno para guardar el caracter \0 que marca el final de una cadena en C*/
    copy = malloc(n); /* malloc solicita n bytes de memoria dinámica, si no puede reservarlos devuelve null*/
    if (copy) memcpy(copy, s, n); /* si sí se pudo reservar, memcpy copia n bytes desde la cadena original hacia el espacio nuevo*/
    return copy; /* devuelve la dirección de la nueva cadena o null si no se pudo reservar memoria*/
}

/*--------------------------------------------------------------------------------------------------------------------------------------------*/

void ir_init(IRProgram *p) /* Inicializando el programa IR, recibe un puntero hacia la estrctura IRProgram que queremos inicializar */
{
    if (p) memset(p, 0, sizeof(*p)); /* Evita acceder a una dirección nula*/
}

/* memset me permite llenar una dirección de memoria con un valor determinado*/
/* Es: dirección donde comienza la estrcutura, valor que colocaremos en sus bytes, cantidad de bytes que ocupa un IRProgram */
/* Inicializa el programa IR colocando sus campos
   en cero. No reserva memoria todavia.
   Prepara la estructura para agregar instrucciones. */

/*---------------------------------------------------------------------------------------------------------------------------------------------*/

void ir_free(IRProgram *p) /* Liberar toda la memoria dinámica que pertenece al programa IR */
{
    size_t i; /* Variable para recorrer las instrucciones*/
    if (!p) return; /* Si el puntero es null, terminamos para evitar acceder a memoria inválida*/
    for (i = 0; i < p->count; ++i) {   /* Para recorrer todas las instrucciones almacenadas*/
        free(p->instructions[i].arg1); /* Liberamos los cuatro campos de texto que pudieron haberse reservado mediante ir_copy_text */
        free(p->instructions[i].arg2);
        free(p->instructions[i].result);
        free(p->instructions[i].extra);
    }
    free(p->instructions); /* Libera el arreglo completo de instrucciones */
    ir_init(p); /* Reinicia los campos de la estructura, dejando el programa IR vacío y listo para reutilizarse*/
    /* ir_init() no libera memoria por si sola, por eso se llama después de los free()*/
}

/*----------------------------------------------------------------------------------------------------------------------------------------------*/

int ir_emit_ex(IRProgram *p, IROp op, const char *arg1, const char *arg2,   /* Almacenamiento de instruciones */
               const char *result, const char *extra, int line)
{ 
    IRInstruction instr = {0}; /* Nueva instr local, es la instrucción que posteriormente agregaremos al programa */
    IRInstruction *expanded; /* Creamos un puntero auxiliar que utilizaremos cuando necesitemos ampliar el arreglo de instrucciones*/
    size_t new_capacity; /* variable donde calculamos la nueva capacidad del arreglo cuando necesitemos reservar más memoria*/
    if (!p) return 0; /* Si no existe un programa IR válido al cual apunta p, no podemos almacenar la instrucción*/
   
    /* LLenando los campos de nuestra nueva instrucción */
    instr.op = op;
    instr.source_line = line;
    instr.arg1 = ir_copy_text(arg1);
    instr.arg2 = ir_copy_text(arg2);
    instr.result = ir_copy_text(result);
    instr.extra = ir_copy_text(extra);

    /* Verifica que las cadenas requeridas se hayan
   copiado correctamente. Si alguna falla,
   libera la memoria reservada y retorna 0. */
    if ((arg1 && !instr.arg1) || (arg2 && !instr.arg2) ||
        (result && !instr.result) || (extra && !instr.extra)) goto fail;
    /* Agrega instrucciones hasta superar los 16 y aumenta capacity */    
    if (p->count == p->capacity) {
        /* Comprobación de seguridad para evitar un desbordamiento numérico cuando se calcula el tamaño de la nueva reserva de memoria */
        if (p->capacity > (SIZE_MAX / sizeof(*p->instructions)) / 2) goto fail;
        /* Operador ternario */
        new_capacity = p->capacity ? p->capacity * 2 : 16;
        /* Amplía la memoria reservada para el arreglo, conservando las instrucciones que ya existían */
        expanded = realloc(p->instructions, new_capacity * sizeof(*p->instructions));
        /* Comprobamos que sí funcionó */
        if (!expanded) goto fail;
        p->instructions = expanded;
        p->capacity = new_capacity;
        /* Utilizo un puntero auxiliar expanded para no perder el arreglo original si realloc() falla */
    }
    p->instructions[p->count++] = instr;
    return 1;
    /* Ahora sí aquí almacenamos la instrucción, incrementando el valor actual de count tipo índice */
fail: /* Si ocurrió algun error, liberamos las cadenas que sí conseguimos reservar y devolvemos 0*/
    free(instr.arg1); free(instr.arg2);
    free(instr.result); free(instr.extra);
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------------------------------*/

int ir_emit(IRProgram *p, IROp op, const char *a, const char *b,      /* Almacenamiento de instruciones */
            const char *result, int line)
{
    return ir_emit_ex(p, op, a, b, result, NULL, line);
}
/*Esta llama a ir_emit_ex (wrapper)*/
/*------------------------------------------------------------------------------------------------------------------------------------------------*/

static char *ir_new_name(size_t *counter, const char *prefix)  /* Counter es el puntero al contador que queremos utilizar e incrementar, prefix es el texto que se pone antes del num */
{
    char buf[64]; /* Arreglo temporal donde se construye el nombre*/
    int n; /* guarda el resultado de snprintf()*/
    if (*counter == SIZE_MAX) return NULL; /* Evitamos que el contador se desborde al incrementarlo*/
   
    /* snprintf() construye una cadena combinando:
    %s el prefix
    %zu el num del contador, tipo size_t
    (*counter)++ utiliza el valor actual del contador y después lo incrementa */
    n = snprintf(buf, sizeof(buf), "%s%zu", prefix, (*counter)++); 
    
    if (n < 0 || (size_t)n >= sizeof(buf)) return NULL;  /* Verifica que el nombre se haya construido correctly y que quepa dentro de buf*/
    return ir_copy_text(buf); /* Hay que hacer una copia en memoria dinámica antes de devolver el nombre ya que buf es una variable local de ir_new_name */
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/

char *ir_new_temp(IRProgram *p) /* Genera los nombres de las variables temporales, utilizando el contador next_temp y el prefijo %t */
{
    return p ? ir_new_name(&p->next_temp, "%t") : NULL;
}

char *ir_new_label(IRProgram *p) /* Practicamente igual que ir_new_temp, pero este es para etiquetas*/
{
    return p ? ir_new_name(&p->next_label, "@L") : NULL;
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/

/* Esto es para diagnostico 
   Enseña el código de tres direcciones*/
/* Recorre las instrucciones del programa IR
   y las muestra en un formato legible de tres
   direcciones, sin modificar el  contenido. */
   void ir_print(const IRProgram *p, FILE *out)
{
    size_t i;
    if (!p || !out) return;
    for (i = 0; i < p->count; ++i) {
        const IRInstruction *v = &p->instructions[i];
        const char *a = v->arg1 ? v->arg1 : ""; /* Recordar que aquí es operadores ternarios. Si arg1 contiene una cadena, se utiliza esa cadena, si es null entonces cadena vacía */
        const char *b = v->arg2 ? v->arg2 : "";
        const char *r = v->result ? v->result : "";
        const char *e = v->extra ? v->extra : "";
        const char *symbol = NULL;
        /* Determina el formato de salida segun
           el tipo de operacion de la instruccion. */
        switch (v->op) { /* ese v op contiene uno de los valores del enumerado IROp definido en ir.h*/
            case IR_DECL: fprintf(out, "decl %s %s", a, r); break;
            case IR_COPY: fprintf(out, "%s = %s", r, a); break;
            case IR_ADD: symbol = "+"; break;
            case IR_SUB: symbol = "-"; break;
            case IR_MUL: symbol = "*"; break;
            case IR_DIV: symbol = "/"; break;
            case IR_MOD: symbol = "%"; break;
            case IR_POW: symbol = "**"; break;
            case IR_EQ: symbol = "=="; break;
            case IR_NE: symbol = "!="; break;
            case IR_LT: symbol = "<"; break;
            case IR_LE: symbol = "<="; break;
            case IR_GT: symbol = ">"; break;
            case IR_GE: symbol = ">="; break;
            case IR_NEG: fprintf(out, "%s = -%s", r, a); break;
            case IR_LABEL: fprintf(out, "%s:", r); break;
            case IR_GOTO: fprintf(out, "goto %s", r); break;
            case IR_IF_FALSE: fprintf(out, "if_false %s goto %s", a, r); break;
            case IR_IF_TRUE: fprintf(out, "if_true %s goto %s", a, r); break;
            case IR_FUNC_BEGIN: fprintf(out, "func %s:", r); break;
            case IR_FUNC_END: fprintf(out, "endfunc %s", r); break;
            case IR_PARAM_DECL: fprintf(out, "param_decl %s %s", a, r); break;
            case IR_PARAM: fprintf(out, "param %s", a); break;
            case IR_CALL: if (v->result) fprintf(out, "%s = ", r);
                          fprintf(out, "call %s, %s", a, b); break;
            case IR_RETURN: fprintf(out, "return %s", a); break;
            case IR_ARRAY_DECL: fprintf(out, "array %s[%s]", r, a); break;
            case IR_ARRAY_LOAD: fprintf(out, "%s = %s[%s]", r, a, b); break;
            case IR_ARRAY_STORE: fprintf(out, "%s[%s] = %s", r, a, b); break;
            case IR_MATRIX_DECL: fprintf(out, "matrix %s[%s][%s]", r, a, b); break;
            case IR_MATRIX_LOAD: fprintf(out, "%s = %s[%s][%s]", r, a, b, e); break;
            case IR_MATRIX_STORE: fprintf(out, "%s[%s][%s] = %s", r, a, b, e); break;
        }
        if (symbol) fprintf(out, "%s = %s %s %s", r, a, symbol, b);
        fputc('\n', out); /* Esto imprime las operaciones binarias utilizando un mismo formato */
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------*/