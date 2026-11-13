#include "ir.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*--------------------------------------------------------------------------------------------------------------------*/

/* Traduccion AST a IR */
/* Mantiene el contexto durante la generacion de IR.
   Contiene el programa, los errores y una pila
   de etiquetas de salida para manejar rompa
   en ciclos simples y anidados. */
typedef struct {
    IRProgram *program; /* Puntero al programa IR */
    IRError *error; /* Puntero a la estructura donde reportamos errores*/
    char **break_targets; /* Arreglo de etiquetas de salida de los ciclos activos */
    size_t break_count; /* Cantidad de destinos rompa almacenados*/
    size_t break_capacity; /* Capacidad reservada para esos destinos */
} Generator;

/*---------------------------------------------------------------------------------------------------------------------*/

/* Crea una copia independiente de una cadena
   en memoria dinamica durante la generacion de IR.
   Como ir_copy_text está como static entonces debemos
   hacer por diseño otra copia. */
static char *copy_str(const char *text)
{
    size_t len;
    char *out;
    if (!text) return NULL;
    len = strlen(text) + 1;
    out = malloc(len);
    if (out) memcpy(out, text, len);
    return out;
}

/*----------------------------------------------------------------------------------------------------------------------*/
/* Recordar que el AST no almacena los hijos en un arreglo, sino que usa los punteros hijo y siguiente*/
static const NodoAST *child(const NodoAST *n, size_t index) /* Su función es obtener un hijo específico de un nodo del AST */
{
    /* Obtiene el hijo de un nodo AST segun su indice,
   recorriendo los punteros siguiente.
   Retorna NULL si el hijo no existe. */
    const NodoAST *p = n ? n->hijo : NULL;
    while (p && index--) p = p->siguiente; /* Obtiene el primer hijo del nodo recibido, siempre que n exista */
    return p; /* Devuelve el nodo encontrado */
}

/*-----------------------------------------------------------------------------------------------------------------------*/
/* Registra el primer error durante la generacion
   de IR, guarda su mensaje y linea.
   Permite mensajes con argumentos variables.
   Devuelve 0 para señañar fallo. */
static int report(Generator *g, const NodoAST *node, const char *fmt, ...)
{
    va_list args;
    if (!g->error) return 0;
    if (g->error->message[0]) return 0;
    g->error->line = node ? node->linea : 0;
    va_start(args, fmt);
    vsnprintf(g->error->message, sizeof(g->error->message), fmt, args);
    va_end(args);
    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------*/
/* Envia una instruccion al programa IR usando
   ir_emit_ex. Obtiene la linea desde el AST y
   reporta un error si falla el almacenamiento. */
static int emit_ex(Generator *g, const NodoAST *n, IROp op,
                   const char *a, const char *b, const char *r, const char *extra)
{
    if (!ir_emit_ex(g->program, op, a, b, r, extra, n ? n->linea : 0))
        return report(g, n, "Memoria insuficiente al generar IR");
    return 1;
}
/* Lo mismo pero sin necesitar un argumento extra*/
static int emit(Generator *g, const NodoAST *n, IROp op,
                const char *a, const char *b, const char *r)
{
    return emit_ex(g, n, op, a, b, r, NULL);
}

/*----------------------------------------------------------------------------------------------------------------------*/
/* Aquí reutilizamos igual funciones de ir.c pero incorporamos el contexto del generador y el manejo de errores*/
static char *temp(Generator *g, const NodoAST *n)
{
    char *s = ir_new_temp(g->program);
    if (!s) report(g, n, "No se pudo crear un temporal IR");
    return s;
}

static char *label(Generator *g, const NodoAST *n)
{
    char *s = ir_new_label(g->program);
    if (!s) report(g, n, "No se pudo crear una etiqueta IR");
    return s;
}

/*----------------------------------------------------------------------------------------------------------------------*/
/* Esta función convierte un num entero de tipo size_t en una cadena de texto
   Se hace porque IRInstruction recibe los campos como cadenas*/
static char *index_string(size_t index)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%zu", index);
    return copy_str(buffer); /* Hace copy_str() porque buffer es local y deja de ser válido cuando termina la función*/
}

/*-----------------------------------------------------------------------------------------------------------------------*/
/* Declaraciones anticipadas de las funciones
   utilizadas durante el recorrido del AST. */
static int statements(Generator *g, const NodoAST *first);
static int statement(Generator *g, const NodoAST *node);
static char *expression(Generator *g, const NodoAST *node);
static int jump_true(Generator *g, const NodoAST *node, const char *target);
static int jump_false(Generator *g, const NodoAST *node, const char *target);

/*------------------------------------------------------------------------------------------------------------------------*/

static IROp operation(const char *symbol)
{
    /* Traduce los operadores del AST a las operaciones
   definidas en el enumerado IROp. */
    if (!symbol) return (IROp)-1;
    if (!strcmp(symbol, "+")) return IR_ADD; /* Compara dos cadenas de texto*/
    if (!strcmp(symbol, "-")) return IR_SUB;
    if (!strcmp(symbol, "*")) return IR_MUL;
    if (!strcmp(symbol, "/")) return IR_DIV;
    if (!strcmp(symbol, "%")) return IR_MOD;
    if (!strcmp(symbol, "**")) return IR_POW;
    if (!strcmp(symbol, "==")) return IR_EQ;
    if (!strcmp(symbol, "!=")) return IR_NE;
    if (!strcmp(symbol, "<")) return IR_LT;
    if (!strcmp(symbol, "<=")) return IR_LE;
    if (!strcmp(symbol, ">")) return IR_GT;
    if (!strcmp(symbol, ">=")) return IR_GE;
    return (IROp)-1;
}

/*--------------------------------------------------------------------------------------------------------------------------*/

/* Su responsabilidad es traducir las llamadas a funciones del AST a código de tres direcciones*/
/* Traduce las llamadas a funciones del AST a IR.
   needs_result indica si se requiere un temporal
   para almacenar el valor retornado. */
static char *call(Generator *g, const NodoAST *n, int needs_result) /* needs_result indica si necesitamos guardar el resultado de la función */
{
    const NodoAST *arg; /* Recorre los argumentos en el AST */
    char **values = NULL; /* Almacena temporalmente las cadenas que representan los operandos */
    size_t count = 0, capacity = 0, i; /* Cantidad de argumentos almacenados y espacios reservados para argumentos*/
    char *result = NULL; /* Nombre del temporal que recibirá el resultado de la llamada */
    char size_text[64];
    int ok = 1; /* Indica si la generación continúa correctamente */


    if (!n->valor) { report(g, n, "Llamada sin nombre de funcion"); return NULL; }
    for (arg = n->hijo; arg; arg = arg->siguiente) {   /* El for comienza en el primer argumento y avanza utilizando siguiente hasta que no quedan más */
        char **more;
        char *value;
        if (count == capacity) { /* Esto es lo mismo de ir.c, reservamos memoria dinámicamente y duplicamos la capacidad cuando el arreglo se llena, pero acá empezamos con 4*/
            size_t next = capacity ? capacity * 2 : 4; /* Empieza en 4 porque una llamada no necesita tanto espacio como la cantidad de instrucciones*/
            if (next < capacity || next > SIZE_MAX / sizeof(*values)) {
                report(g, n, "Demasiados argumentos"); ok = 0; break;
            }
            more = realloc(values, next * sizeof(*values)); /* Tener espacio para guardar los operandos de los argumentos*/
            if (!more) { report(g, n, "Sin memoria para argumentos"); ok = 0; break; }
            values = more;
            capacity = next;
        }
        /* Traduce cada argumento a IR y obtiene
            Cel operando que representa su resultado. */
        value = expression(g, arg);
        if (!value) { ok = 0; break; }
        
        /* Hay que congelar las variables que pueden cambiar */
        if (value[0] != '%' && value[0] != '-' &&
            !(value[0] >= '0' && value[0] <= '9')) {
            char *snap = temp(g, arg); /* Generamos un nuevo temporal*/
            if (!snap || !emit(g, arg, IR_COPY, value, NULL, snap)) {  /* Generamos una instr que copia el valor de la variable temporal */
                free(snap); free(value); ok = 0; break;
            }
            free(value);
            value = snap; /* Liberamos la cadena que contenía el nombre anterior y ahora utilizamos el nombre del temporal */
        }
        /* Guardamos el operando resultante en nuestro arreglo 
           de argumentos e incrementamos el contador.
        */
        values[count++] = value; 
    }
    /* Emite consecutivamente los parametros de la
       llamada una vez preparados todos los argumentos. */
    if (ok) {
        for (i = 0; i < count; ++i)
            if (!emit(g, n, IR_PARAM, values[i], NULL, NULL)) { ok = 0; break; }
    }
    if (ok && needs_result) { result = temp(g, n); if (!result) ok = 0; } /* Genera el temporal de retorno (o sea si vamos a necesitar el resultado de la operación) */
    if (ok) { /* Aquí finalmente generamos la instrucción que representa la llamada */
        snprintf(size_text, sizeof(size_text), "%zu", count); /* snprintf() convierte la cantidad de argumentos a una cadena, porque los op de la IR son textos*/
        ok = emit(g, n, IR_CALL, n->valor, size_text, result);
    }
    /* Libera memoria y retorna */
    for (i = 0; i < count; ++i) free(values[i]);
    free(values);
    if (!ok) { free(result); return NULL; }
    /* En llamadas usadas como sentencias no se requiere valor. */
    return needs_result ? result : copy_str("");
}
/* Liberamos las cadenas auxiliares de los argumentos y después del arreglo values??? */

/*----------------------------------------------------------------------------------------------------------------------------------*/

/* Genera saltos hacia target cuando una
   condicion es falsa, aplicando cortocircuito
   para las operaciones logicas. */

static int jump_false(Generator *g, const NodoAST *n, const char *target)
{
    char *skip = NULL;
    char *condition = NULL;
    int ok = 0;
    if (!n) return report(g, n, "Condicion vacia");
    /* Si cualquiera de los operandos del AND
       es falso, salta a la etiqueta de destino. */
    if (n->tipo == AST_OPERACION && n->valor && !strcmp(n->valor, "AND"))
        return jump_false(g, child(n, 0), target) && jump_false(g, child(n, 1), target);

    /* Si el primer operando del OR es verdadero,
       utiliza una etiqueta auxiliar para omitir
       la evaluacion del segundo operando.
       Si ambos son falsos, salta a target. */    
    if (n->tipo == AST_OPERACION && n->valor && !strcmp(n->valor, "OR")) {
        skip = label(g, n); /* Se genera la etiqueta auxiliar @L1*/
        if (!skip) return 0;
        ok = jump_true(g, child(n, 0), skip) && /* Generamos el salto hacia skip si la primera condición es verdadera*/
             jump_false(g, child(n, 1), target) && /* Generamos el salto hacia la salida si la segunda condición es falsa. target: Salir del si porque toda la condición es falsa*/
             emit(g, n, IR_LABEL, NULL, NULL, skip); /*skip: saltarme la segunda comparación porque la primera ya es verdadera */
             /* Colocamos la etiqueta auxiliar donde corresponde */
        free(skip);
        return ok;
    }
    condition = expression(g, n);
    if (condition) ok = emit(g, n, IR_IF_FALSE, condition, NULL, target);
    free(condition);
    return ok;
}

/* jump_true y jump_false trabajan en conjunto
   para generar los saltos del flujo de control
   y permitir el cortocircuito de AND y OR. */


/* Genera saltos cuando una condicion es verdadera,
   aplicando cortocircuito para AND y OR. */
static int jump_true(Generator *g, const NodoAST *n, const char *target)
{
    char *skip = NULL;
    char *condition = NULL;
    int ok = 0;
    if (!n) return report(g, n, "Condicion vacia");
    if (n->tipo == AST_OPERACION && n->valor && !strcmp(n->valor, "OR"))
        return jump_true(g, child(n, 0), target) &&
               jump_true(g, child(n, 1), target); /* se emiten los dos saltos */
    if (n->tipo == AST_OPERACION && n->valor && !strcmp(n->valor, "AND")) {
        skip = label(g, n);
        if (!skip) return 0;
        ok = jump_false(g, child(n, 0), skip) &&
             jump_true(g, child(n, 1), target) &&
             emit(g, n, IR_LABEL, NULL, NULL, skip);
        free(skip);
        return ok;
    }
    condition = expression(g, n);
    if (condition) ok = emit(g, n, IR_IF_TRUE, condition, NULL, target);
    free(condition);
    return ok;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------*/
/* Traduce las expresiones del AST a instrucciones
   de tres direcciones y retorna el operando
   que representa su resultado. */
static char *expression(Generator *g, const NodoAST *n)
{
    char *a = NULL, *b = NULL, *r = NULL;
    char *true_label = NULL, *end_label = NULL;
    IROp op;
    int ok = 0;
    const NodoAST *head;
    if (!n) { report(g, n, "Expresion incompleta"); return NULL; }
    switch (n->tipo) {
        /* Los numeros y variables ya son operandos,
           por lo que no requieren generar instrucciones. */
        case AST_NUMERO:
        case AST_VARIABLE:
            if (!n->valor) { report(g, n, "Expresion sin valor"); return NULL; }
            r = copy_str(n->valor);
            if (!r) report(g, n, "Sin memoria para expresion");
            return r;
        /* Representa verdadero como 1 y falso como 0. */    
        case AST_BOOLEANO:
            if (!n->valor) { report(g, n, "Booleano sin valor"); return NULL; }
            r = copy_str(!strcmp(n->valor, "verdadero") ? "1" :
                         !strcmp(n->valor, "falso") ? "0" : n->valor);
            if (!r) report(g, n, "Sin memoria para booleano");
            return r;
        /* Genera la llamada solicitando su valor de retorno. */    
        case AST_LLAMADA:
            return call(g, n, 1);
        /* Genera instrucciones de lectura de listas
        y matrices, obteniendo sus indices desde el
        AST y almacenando el resultado en un temporal. */
        case AST_ACCESO_LISTA:
        case AST_ACCESO_MATRIZ:
            head = child(n, 0);
            if (!head || !head->valor) { report(g, n, "Acceso sin arreglo"); return NULL; }
            a = expression(g, child(n, 1));
            if (!a) goto cleanup;
            if (n->tipo == AST_ACCESO_MATRIZ) {
                b = expression(g, child(n, 2));
                if (!b) goto cleanup;
            }
            r = temp(g, n);
            if (!r) goto cleanup;
            ok = emit_ex(g, n, n->tipo == AST_ACCESO_LISTA ? IR_ARRAY_LOAD : IR_MATRIX_LOAD,
                         head->valor, a, r, b);
            goto cleanup;
        case AST_OPERACION:
            if (!n->valor) { report(g, n, "Operacion sin operador"); return NULL; }
            /* Traduce expresiones logicas con cortocircuito
               y genera un temporal con resultado 0 o 1. */    
            if (!strcmp(n->valor, "AND") || !strcmp(n->valor, "OR")) {
                true_label = label(g, n);
                end_label = label(g, n);
                r = temp(g, n);
                if (!true_label || !end_label || !r) goto cleanup;
                ok = jump_true(g, n, true_label) &&
                     emit(g, n, IR_COPY, "0", NULL, r) &&
                     emit(g, n, IR_GOTO, NULL, NULL, end_label) &&
                     emit(g, n, IR_LABEL, NULL, NULL, true_label) &&
                     emit(g, n, IR_COPY, "1", NULL, r) &&
                     emit(g, n, IR_LABEL, NULL, NULL, end_label);
                goto cleanup;
            }
            /* Genera la negacion de un operando
               y almacena su resultado en un temporal. */
            if (!strcmp(n->valor, "unario -")) {
                a = expression(g, child(n, 0));
                if (!a) goto cleanup;
                r = temp(g, n);
                if (!r) goto cleanup;
                ok = emit(g, n, IR_NEG, a, NULL, r);
                goto cleanup;
            }
            /* Traduce recursivamente ambos operandos,
               genera un temporal y emite la operacion
               binaria correspondiente. */
            op = operation(n->valor);
            if ((int)op == -1) { report(g, n, "Operador IR no soportado: %s", n->valor); return NULL; }
            a = expression(g, child(n, 0));
            if (!a) goto cleanup;
            b = expression(g, child(n, 1));
            if (!b) goto cleanup;
            r = temp(g, n);
            if (!r) goto cleanup;
            ok = emit(g, n, op, a, b, r);
            goto cleanup;
        default:
            report(g, n, "Nodo no valido como expresion (tipo %d)", n->tipo);
            return NULL;
    }
cleanup:
    free(a); free(b); free(true_label); free(end_label);
    if (!ok) { free(r); return NULL; }
    return r;
} /* Libera la memoria auxiliar y devuelve el operando resultante si todo salió bien. */

/*-------------------------------------------------------------------------------------------------------------------------------------------*/
/* Traduce asignaciones obteniendo el destino
   y el operando resultante de la expresion.
   Soporta variables, listas y matrices. */
static int assignment(Generator *g, const NodoAST *n)
{
    const NodoAST *dst = child(n, 0);
    char *value = NULL, *i = NULL, *j = NULL;
    int ok = 0;
    if (!dst) return report(g, n, "Asignacion sin destino");
    value = expression(g, child(n, 1));
    if (!value) return 0;
    if (dst->tipo == AST_VARIABLE && dst->valor) {
        ok = emit(g, n, IR_COPY, value, NULL, dst->valor);
    } else if (dst->tipo == AST_ACCESO_LISTA || dst->tipo == AST_ACCESO_MATRIZ) {
        const NodoAST *base = child(dst, 0);
        if (!base || !base->valor) { report(g, n, "Destino arreglo sin nombre"); goto done; }
        i = expression(g, child(dst, 1));
        if (!i) goto done;
        if (dst->tipo == AST_ACCESO_MATRIZ) {
            j = expression(g, child(dst, 2));
            if (!j) goto done;
            ok = emit_ex(g, n, IR_MATRIX_STORE, i, j, base->valor, value);
        } else {
            ok = emit(g, n, IR_ARRAY_STORE, i, value, base->valor);
        }
    } else report(g, n, "Destino de asignacion no soportado");
done:
    free(value); free(i); free(j);
    return ok;
}
/* Genera la declaracion de una variable y,
   si tiene un valor inicial, genera tambien
   su asignacion mediante IR_COPY. */
static int declaration(Generator *g, const NodoAST *n)
{
    const NodoAST *id = child(n, 0);
    char *value = NULL;
    int ok;
    if (!id || !id->valor) return report(g, n, "Declaracion sin identificador");
    ok = emit(g, n, IR_DECL, n->valor, NULL, id->valor);
    if (!ok || !child(n, 1)) return ok;
    value = expression(g, child(n, 1));
    if (!value) return 0;
    ok = emit(g, n, IR_COPY, value, NULL, id->valor);
    free(value);
    return ok;
}

/*-----------------------------------------------------------------------------------------------------------------------------*/
/* Algo parecido a declaration() pero para listas y matrices.
   Su propósito es generar la declaración del arreglo, sus dimensiones y 
   la inicialización de sus elementos, si existe.
   Recorre dinámicamente el arreglo. */
static int indexed_declaration(Generator *g, const NodoAST *n)
{
    const NodoAST *id = child(n, 0);
    const NodoAST *initializer;
    const NodoAST *row, *element;
    char *dim1 = NULL, *dim2 = NULL, *row_text = NULL, *col_text = NULL, *value = NULL;
    size_t ri, ci;
    int matrix = n->tipo == AST_DECL_MATRIZ;
    int ok = 0;
    if (!id || !id->valor) return report(g, n, "Declaracion de arreglo sin nombre");
    dim1 = expression(g, child(n, 1));
    if (!dim1) goto done;
    if (matrix) {
        dim2 = expression(g, child(n, 2));
        if (!dim2) goto done;
    }
    if (!emit(g, n, matrix ? IR_MATRIX_DECL : IR_ARRAY_DECL,
              dim1, dim2, id->valor)) goto done;
    initializer = child(n, matrix ? 3 : 2);
    if (!initializer) { ok = 1; goto done; }
    if (matrix && initializer->tipo != AST_MATRIZ) {
        report(g, n, "Inicializador de matriz invalido"); goto done;
    }
    if (!matrix && initializer->tipo != AST_LISTA) {
        report(g, n, "Inicializador de lista invalido"); goto done;
    }
    if (!matrix) {
        for (element = initializer->hijo, ci = 0; element; element = element->siguiente, ++ci) {
            col_text = index_string(ci);
            value = expression(g, element);
            if (!col_text || !value || !emit(g, element, IR_ARRAY_STORE, col_text, value, id->valor))
                goto done;
            free(col_text); col_text = NULL;
            free(value); value = NULL;
        }
    } else {
        for (row = initializer->hijo, ri = 0; row; row = row->siguiente, ++ri) {
            if (row->tipo != AST_LISTA) { report(g, row, "Fila de matriz invalida"); goto done; }
            row_text = index_string(ri);
            if (!row_text) { report(g, row, "Sin memoria para indice"); goto done; }
            for (element = row->hijo, ci = 0; element; element = element->siguiente, ++ci) {
                col_text = index_string(ci);
                value = expression(g, element);
                if (!col_text || !value ||
                    !emit_ex(g, element, IR_MATRIX_STORE, row_text, col_text, id->valor, value))
                    goto done;
                free(col_text); col_text = NULL;
                free(value); value = NULL;
            }
            free(row_text); row_text = NULL;
        }
    }
    ok = 1;
done:
    free(dim1); free(dim2); free(row_text); free(col_text); free(value);
    if (!ok && g->error && !g->error->message[0]) report(g, n, "Sin memoria al inicializar arreglo");
    return ok;
}

/*-------------------------------------------------------------------------------------------------------------------------------------*/
/* Agrega la etiqueta de salida de un ciclo
   a la pila break_targets, ampliando su
   capacidad cuando sea necesario. */
/* Es más que todo para saber de que ciclo salirme segun la pila */
static int push_break(Generator *g, const NodoAST *n, const char *target)
{
    char **items;
    char *copy;
    size_t new_cap;
    if (g->break_count == g->break_capacity) {
        if (g->break_capacity > SIZE_MAX / sizeof(char *) / 2)
            return report(g, n, "Demasiados ciclos anidados");
        new_cap = g->break_capacity ? g->break_capacity * 2 : 8;
        items = realloc(g->break_targets, new_cap * sizeof(char *));
        if (!items) return report(g, n, "Sin memoria para ciclos");
        g->break_targets = items;
        g->break_capacity = new_cap;
    }
    copy = copy_str(target);
    if (!copy) return report(g, n, "Sin memoria para etiqueta break");
    g->break_targets[g->break_count++] = copy;
    return 1;
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/

static int conditional(Generator *g, const NodoAST *n)
{
    /* Las tres partes del condicional*/
    const NodoAST *condition = child(n, 0);
    const NodoAST *then_body = child(n, 1);
    const NodoAST *otherwise = child(n, 2);
    char *false_label = label(g, n); /* Inicio del sino o salida si no hay sino*/
    char *end_label = otherwise ? label(g, n) : NULL;
    int ok = 0;
    if (!false_label || (otherwise && !end_label)) goto done;
    if (!jump_false(g, condition, false_label) ||
        !statement(g, then_body)) goto done;
    if (otherwise && !emit(g, n, IR_GOTO, NULL, NULL, end_label)) goto done;
    if (!emit(g, n, IR_LABEL, NULL, NULL, false_label)) goto done;
    if (otherwise && (!statement(g, otherwise) ||
                      !emit(g, n, IR_LABEL, NULL, NULL, end_label))) goto done;
    ok = 1;
done:
    free(false_label); free(end_label);
    return ok;
}

/*----------------------------------------------------------------------------------------------------------------------------------------*/

static int loop(Generator *g, const NodoAST *n, int is_for) /* Traduce ciclos mientras y para mediante etiquetas y saltos condicionales */
{
    /* El parámetro is_for hace la diferenciación entre un para y un mientras 
       Ambos comparten la misma lógica de saltos. El para simplemente agrega la inicialización y el incremento*/
    const NodoAST *init = is_for ? child(n, 0) : NULL;
    const NodoAST *condition = child(n, is_for ? 1 : 0);
    const NodoAST *step = is_for ? child(n, 2) : NULL;
    const NodoAST *body = child(n, is_for ? 3 : 1);
    char *start = NULL, *end = NULL;
    int ok = 0, pushed = 0;
    if (init && !statement(g, init)) return 0;
    start = label(g, n);
    end = label(g, n);
    if (!start || !end || !emit(g, n, IR_LABEL, NULL, NULL, start) ||
        !jump_false(g, condition, end) || !push_break(g, n, end)) goto done; /* Usamos un push break para guardar la etiqueta de salida del ciclo si encontramos un rompa */
    pushed = 1;
    if (!statement(g, body)) goto done;
    if (step && !statement(g, step)) goto done;
    if (!emit(g, n, IR_GOTO, NULL, NULL, start) ||
        !emit(g, n, IR_LABEL, NULL, NULL, end)) goto done;
    ok = 1;
done:
    if (pushed) free(g->break_targets[--g->break_count]);
    free(start); free(end);
    return ok;
}
/* Recordar los dibujos de las etiquetas y saltos */
/*-------------------------------------------------------------------------------------------------------------------------------------------*/

/* Traduce la definición de una función a IR, incluyendo sus parámetros y cuerpo */
static int function(Generator *g, const NodoAST *n, int is_main)
{
    /* Obtiene el nombre, los parámetros y el cuerpo de la función desde el AST*/
    const NodoAST *name = is_main ? NULL : child(n, 0); /* El parámetro is main permite distinguir la función principal de las demás */
    const NodoAST *params = is_main ? NULL : child(n, 1);
    const NodoAST *body = child(n, is_main ? 0 : 2);
    const NodoAST *param;
    const char *function_name = is_main ? "principal" : (name ? name->valor : NULL);
    if (!function_name || !body) return report(g, n, "Funcion sin nombre o cuerpo");
    if (!emit(g, n, IR_FUNC_BEGIN, NULL, NULL, function_name)) return 0;
    for (param = params ? params->hijo : NULL; param; param = param->siguiente) {
        const NodoAST *id = child(param, 0);
        char *dim1 = NULL, *dim2 = NULL;
        int ok;
        if (!id || !id->valor || param->tipo != AST_PARAMETRO)
            return report(g, param, "Parametro de funcion invalido");
        if (child(param, 1)) {
            dim1 = expression(g, child(param, 1));
            if (!dim1) return 0;
        }
        if (child(param, 2)) {
            dim2 = expression(g, child(param, 2));
            if (!dim2) { free(dim1); return 0; }
        }
        ok = emit_ex(g, param, IR_PARAM_DECL, param->valor, dim1, id->valor, dim2);
        free(dim1); free(dim2);
        if (!ok) return 0;
    }
    if (!statement(g, body)) return 0;
    return emit(g, n, IR_FUNC_END, NULL, NULL, function_name);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* Básicamente el distribuidor de sentencias.
   Recibe un nodo AST, identifica su tipo mediante un switch y llama a la función encargada de traducirlo */
static int statement(Generator *g, const NodoAST *n)
{
    char *value;
    int ok;
    if (!n) return 1;
    switch (n->tipo) {
        case AST_PROGRAMA:
        case AST_BLOQUE:
            return statements(g, n->hijo);
        case AST_IMPORTACION:
            /* El parser resuelve importaciones; no generan operaciones ejecutables. */
            return 1;
        case AST_PRINCIPAL:
            return function(g, n, 1);
        case AST_FUNCION:
            return function(g, n, 0);
        case AST_DECLARACION:
            return declaration(g, n);
        case AST_DECL_LISTA:
        case AST_DECL_MATRIZ:
            return indexed_declaration(g, n);
        case AST_ASIGNACION:
            return assignment(g, n);
        case AST_IF:
            return conditional(g, n);
        case AST_WHILE:
            return loop(g, n, 0);
        case AST_FOR:
            return loop(g, n, 1);
        case AST_BREAK:
            if (!g->break_count) return report(g, n, "rompa fuera de ciclo");
            return emit(g, n, IR_GOTO, NULL, NULL,
                        g->break_targets[g->break_count - 1]);
        case AST_RETURN:
            value = expression(g, child(n, 0));
            if (!value) return 0;
            ok = emit(g, n, IR_RETURN, value, NULL, NULL);
            free(value);
            return ok;
        case AST_LLAMADA:
            value = call(g, n, 0);
            if (!value) return 0;
            free(value);
            return 1;
        default:
            return report(g, n, "Sentencia AST no soportada (tipo %d)", n->tipo);
    }
}

/* Recorrer y traducir una secuencia completa de sentencias 
   statements() recorre todas las sentencias y utiliza statement() para traducirlas individualmente */
static int statements(Generator *g, const NodoAST *first)
{
    const NodoAST *n;
    for (n = first; n; n = n->siguiente)
        if (!statement(g, n)) return 0;
    return 1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* Función principal del generador de IR 
   Recibe tres elementos, root (raíz del AST), out (programa donde entregaremos la IR generada), e info de posibles errores*/
int ir_generate(const NodoAST *root, IRProgram *out, IRError *error)
{
    IRProgram generated;
    Generator g = {0};
    size_t i;
    int ok;
    if (error) { error->line = 0; error->message[0] = '\0'; }
    if (!root || !out) {
        if (error) snprintf(error->message, sizeof(error->message),
                            "AST o salida IR nula");
        return 0;
    }
    /* Primero prepara un programa IR nuevo y el contexto de generación*/
    ir_init(&generated);
    g.program = &generated;
    g.error = error;
    /* Ahora viene la línea más importante, donde comienza la traducción de todo el AST */
    ok = statements(&g, root);
    for (i = 0; i < g.break_count; ++i) free(g.break_targets[i]);
    free(g.break_targets);
    /* Aquí comprueba si la generación fue exitosa 
       Si falla, libera la IR incompleta y conserva el contenido anterior de out.
       Si tiene éxito, reemplaza out con la nueva IR. Para esto, out tuvo que haberse inicializado previamente*/
    if (!ok) { ir_free(&generated); return 0; }
    ir_free(out);
    *out = generated;
    return 1;
}
