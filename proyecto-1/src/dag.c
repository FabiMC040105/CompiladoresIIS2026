#include "dag.h"
#include <stdlib.h>
#include <string.h>

static const unsigned campos[DAG_CAMPOS] = {
    DEP_ARG1, DEP_ARG2, DEP_RESULT, DEP_EXTRA
};

int dag_indice_campo(unsigned campo)
{
    for (size_t i = 0; i < DAG_CAMPOS; ++i)
        if (campos[i] == campo) return (int)i;
    return -1;
}

/* Arreglos dinamicos: los IDs son indices, no punteros a memoria reasignada. */
static void *ampliar(void *datos, size_t *capacidad, size_t elemento)
{
    if (*capacidad > SIZE_MAX / elemento / 2) return NULL;
    size_t nueva = *capacidad ? *capacidad * 2 : 16;
    if (nueva > SIZE_MAX / elemento) return NULL;
    void *p = realloc(datos, nueva * elemento);
    if (p) *capacidad = nueva;
    return p;
}

static size_t agregar_nodo(DAGBloque *g, DAGNodo n)
{
    if (g->cantidad_nodos == g->capacidad_nodos) {
        DAGNodo *v = ampliar(g->nodos, &g->capacidad_nodos, sizeof(*v));
        if (!v) return DAG_NULO;
        g->nodos = v;
    }
    size_t id = g->cantidad_nodos++;
    g->nodos[id] = n;
    return id;
}

static DAGNodo nodo_vacio(DAGTipo tipo)
{
    DAGNodo n = {0};
    n.tipo = tipo;
    n.instruccion = DAG_NULO;
    for (size_t k = 0; k < DAG_CAMPOS; ++k) n.operandos[k] = DAG_NULO;
    return n;
}

static size_t buscar_nombre(const DAGBloque *g, const char *nombre)
{
    for (size_t k = 0; k < g->cantidad_nombres; ++k)
        if (!strcmp(g->nombres[k].nombre, nombre)) return k;
    return DAG_NULO;
}

const DAGNombre *dag_buscar_nombre(const DAGBloque *g, const char *nombre)
{
    if (!g || !nombre) return NULL;
    size_t k = buscar_nombre(g, nombre);
    return k == DAG_NULO || g->nombres[k].nodo == DAG_NULO ? NULL : &g->nombres[k];
}

static int vincular(DAGBloque *g, const char *nombre, size_t nodo, size_t def)
{
    size_t k = buscar_nombre(g, nombre);
    if (k == DAG_NULO) {
        if (g->cantidad_nombres == g->capacidad_nombres) {
            DAGNombre *v = ampliar(g->nombres, &g->capacidad_nombres, sizeof(*v));
            if (!v) return 0;
            g->nombres = v;
        }
        k = g->cantidad_nombres++;
    }
    g->nombres[k] = (DAGNombre){nombre, nodo, def};
    return 1;
}

/* Una constante no cambia; se comparte su hoja por texto exacto. */
static size_t constante(DAGBloque *g, const char *texto)
{
    for (size_t k = 0; k < g->cantidad_nodos; ++k)
        if (g->nodos[k].tipo == DAG_CONSTANTE && !strcmp(g->nodos[k].texto, texto))
            return k;
    DAGNodo n = nodo_vacio(DAG_CONSTANTE);
    n.texto = texto;
    return agregar_nodo(g, n);
}

/* Leer el valor vigente ANTES de registrar una eventual escritura. */
static int leer_nombre(DAGBloque *g, const char *nombre, DAGUso *uso)
{
    const DAGNombre *v = dag_buscar_nombre(g, nombre);
    if (v) {
        *uso = (DAGUso){v->nodo, v->definicion};
        return 1;
    }
    DAGNodo n = nodo_vacio(DAG_ENTRADA);
    n.texto = nombre;
    size_t id = agregar_nodo(g, n);
    if (id == DAG_NULO || !vincular(g, nombre, id, DAG_NULO)) return 0;
    *uso = (DAGUso){id, DAG_NULO};
    return 1;
}

/* dep_accesos excluye las constantes de lecturas. Recuperar exclusivamente
   las posiciones que pueden contener literales (no etiquetas ni metadatos).
   Los nombres y escrituras SIEMPRE se obtienen de dep_accesos. */
static unsigned campos_de_valor(IROp op)
{
    switch (op) {
        case IR_COPY: case IR_NEG: case IR_PARAM: case IR_RETURN:
        case IR_IF_TRUE: case IR_IF_FALSE: case IR_ARRAY_DECL:
            return DEP_ARG1;
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
        case IR_POW: case IR_EQ: case IR_NE: case IR_LT: case IR_LE:
        case IR_GT: case IR_GE: case IR_MATRIX_DECL: case IR_ARRAY_LOAD:
            return DEP_ARG1 | DEP_ARG2;
        case IR_PARAM_DECL: return DEP_ARG2 | DEP_EXTRA;
        case IR_MATRIX_LOAD: return DEP_ARG1 | DEP_ARG2 | DEP_EXTRA;
        case IR_ARRAY_STORE: return DEP_ARG1 | DEP_ARG2 | DEP_RESULT;
        case IR_MATRIX_STORE: return DEP_ARG1 | DEP_ARG2 | DEP_RESULT | DEP_EXTRA;
        default: return 0;
    }
}

/* Las llamadas/escrituras de memoria pueden modificar nombres del usuario.
   No se supone aliasing imposible. Los %tN son valores virtuales privados.
   Una declaracion tambien puede cambiar la identidad del nombre textual. */
static void invalidar(DAGBloque *g, const IRInstruction *inst, const DepAccesos *a)
{
    for (size_t k = 0; k < g->cantidad_nombres; ++k) {
        DAGNombre *v = &g->nombres[k];
        if ((a->escribe_memoria && v->nombre[0] != '%') ||
            (a->declaracion && inst->result && !strcmp(v->nombre, inst->result)) ||
            inst->op == IR_FUNC_BEGIN) {
            v->nodo = v->definicion = DAG_NULO;
        }
    }
}

/* Reuso solo estructural: mismo operador y mismos hijos en el mismo orden.
   No elimina instrucciones ni supone que una operacion opaca sea pura.
   Las definiciones textuales siguen siendo distintas aunque compartan nodo. */
static size_t operacion(DAGBloque *g, const DAGInstruccion *i, IROp op)
{
    DAGNodo n = nodo_vacio(i->accesos.barrera ? DAG_OPACA : DAG_OPERACION);
    n.op = op;
    n.instruccion = i->indice;
    for (size_t k = 0; k < DAG_CAMPOS; ++k) n.operandos[k] = i->usos[k].nodo;
    if (n.tipo == DAG_OPERACION) {
        for (size_t k = 0; k < g->cantidad_nodos; ++k) {
            const DAGNodo *anterior = &g->nodos[k];
            if (anterior->tipo == n.tipo && anterior->op == op &&
                !memcmp(anterior->operandos, n.operandos, sizeof(n.operandos))) return k;
        }
    }
    return agregar_nodo(g, n);
}

static int procesar(DAGBloque *g, const IRInstruction *inst, DAGInstruccion *i)
{
    if (!dep_accesos(inst, &i->accesos)) return 0;
    i->nodo = i->resultado = DAG_NULO;
    unsigned valores = campos_de_valor(inst->op);
    /* 1. Crear/encontrar los hijos con los valores previos. */
    for (size_t k = 0; k < DAG_CAMPOS; ++k) {
        DAGUso *u = &i->usos[k];
        *u = (DAGUso){DAG_NULO, DAG_NULO};
        const char *texto = dep_campo(inst, campos[k]);
        if (i->accesos.lecturas & campos[k]) {
            if (!leer_nombre(g, texto, u)) return 0;
        } else if ((valores & campos[k]) && texto) {
            /* dep_accesos ya valido que este operando sea una constante. */
            u->nodo = constante(g, texto);
            if (u->nodo == DAG_NULO) return 0;
        }
    }
    /* 2. Crear el nodo de la operacion. Una copia comparte el valor. */
    if (inst->op == IR_COPY) i->nodo = i->usos[0].nodo;
    else i->nodo = operacion(g, i, inst->op);
    if (i->nodo == DAG_NULO) return 0;
    /* 3. Invalidar nombres potencialmente modificados por efectos opacos. */
    invalidar(g, inst, &i->accesos);
    /* 4. Actualizar el nombre de destino sin eliminar el nodo anterior.
       En la IR actual todas las escrituras escalares usan result. */
    if (i->accesos.escrituras) {
        if (i->accesos.escrituras != DEP_RESULT) return 0;
        i->resultado = i->nodo;
        if (!vincular(g, inst->result, i->resultado, i->indice)) return 0;
    }
    return 1;
}

int dag_construir(const IRProgram *p, const BasicBlock *b, DAGBloque *out)
{
    if (!out) return 0;
    *out = (DAGBloque){0};
    if (!p || !b || !p->instructions || b->start > b->end || b->end >= p->count) return 0;
    size_t n = b->end - b->start + 1;
    if (n > SIZE_MAX / sizeof(*out->instrucciones)) return 0;
    out->bloque = *b;
    out->instrucciones = calloc(n, sizeof(*out->instrucciones));
    if (!out->instrucciones) goto error;
    out->cantidad_instrucciones = n;
    for (size_t k = 0; k < n; ++k) {
        DAGInstruccion *i = &out->instrucciones[k];
        i->indice = b->start + k;
        if (!procesar(out, &p->instructions[i->indice], i)) goto error;
    }
    return 1;
error:
    dag_liberar(out);
    return 0;
}

const DAGInstruccion *dag_buscar_instruccion(const DAGBloque *g, size_t indice)
{
    if (!g || indice < g->bloque.start || indice - g->bloque.start >= g->cantidad_instrucciones) return NULL;
    return &g->instrucciones[indice - g->bloque.start];
}

void dag_liberar(DAGBloque *g)
{
    if (!g) return;
    free(g->nodos);
    free(g->nombres);
    free(g->instrucciones);
    *g = (DAGBloque){0};
}

static const char *op_nombre(IROp op)
{
    static const char *const nombres[] = {
        "DECL","COPY","ADD","SUB","MUL","DIV","MOD","POW","NEG",
        "EQ","NE","LT","LE","GT","GE","LABEL","GOTO","IF_FALSE","IF_TRUE",
        "FUNC_BEGIN","FUNC_END","PARAM_DECL","PARAM","CALL","RETURN",
        "ARRAY_DECL","ARRAY_LOAD","ARRAY_STORE","MATRIX_DECL","MATRIX_LOAD","MATRIX_STORE"
    };
    return (unsigned)op < sizeof(nombres)/sizeof(nombres[0]) ? nombres[op] : "?";
}

void dag_imprimir(const DAGBloque *g, FILE *out)
{
    if (!g || !out) return;
    fprintf(out, "DAG B%zu [I%zu..I%zu]\n", g->bloque.id, g->bloque.start, g->bloque.end);
    for (size_t k = 0; k < g->cantidad_nodos; ++k) {
        const DAGNodo *n = &g->nodos[k];
        fprintf(out, "N%zu = ", k);
        if (n->tipo == DAG_ENTRADA || n->tipo == DAG_CONSTANTE)
            fprintf(out, "%s(%s)", n->tipo == DAG_ENTRADA ? "entrada" : "constante", n->texto);
        else {
            fprintf(out, "%s%s(", n->tipo == DAG_OPACA ? "opaca:" : "", op_nombre(n->op));
            int primero = 1;
            for (size_t j = 0; j < DAG_CAMPOS; ++j) if (n->operandos[j] != DAG_NULO) {
                fprintf(out, "%sN%zu", primero ? "" : ", ", n->operandos[j]); primero = 0;
            }
            fputc(')', out);
        }
        fputc('\n', out);
    }
    fputs("Usos y definiciones por instruccion:\n", out);
    for (size_t k = 0; k < g->cantidad_instrucciones; ++k) {
        const DAGInstruccion *i = &g->instrucciones[k];
        fprintf(out, "I%zu -> N%zu", i->indice, i->nodo);
        for (size_t j = 0; j < DAG_CAMPOS; ++j) if (i->usos[j].nodo != DAG_NULO) {
            static const char *const etiquetas[] = {"arg1", "arg2", "result", "extra"};
            fprintf(out, " %s=N%zu", etiquetas[j], i->usos[j].nodo);
            if (i->usos[j].definicion != DAG_NULO) fprintf(out, "(def I%zu)", i->usos[j].definicion);
        }
        if (i->accesos.barrera) fputs(" [barrera]", out);
        fputc('\n', out);
    }
    fputs("Nombres vigentes (no implica liveness):\n", out);
    for (size_t k = 0; k < g->cantidad_nombres; ++k) {
        const DAGNombre *v = &g->nombres[k];
        if (v->nodo == DAG_NULO) fprintf(out, "%s -> desconocido\n", v->nombre);
        else {
            fprintf(out, "%s -> N%zu", v->nombre, v->nodo);
            if (v->definicion != DAG_NULO) fprintf(out, " (def I%zu)", v->definicion);
            fputc('\n', out);
        }
    }
}
