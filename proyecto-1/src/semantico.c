#include "semantico.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* La tabla ya fue construida por Bison. Solo marcamos las declaraciones
   que se han encontrado durante el recorrido, sin alterar la tabla. */
typedef struct {
    const TablaSimbolos *tabla;
    SemanticoResolverAmbito resolver;
    void *contexto;
    unsigned char *declarados;
    size_t cantidad;
    const char *funcion_actual;
    TipoDato retorno_actual;
    int en_funcion;
    unsigned profundidad_bucle;
    unsigned retornos_encontrados;
} EstadoSemantico;

static const NodoAST *hijo_n(const NodoAST *n, unsigned pos)
{
    const NodoAST *h = n ? n->hijo : NULL;
    while (h && pos--) h = h->siguiente;
    return h;
}

static const Simbolo *buscar_en_orden(
    const NodoAST *ident, const EstadoSemantico *e, int *posterior)
{
    if (posterior) *posterior = 0;
    if (!ident || !ident->valor) return NULL;

    const Ambito *a = e->resolver(ident, e->contexto);
    while (a) {
        const Simbolo *s = ts_buscar_local(e->tabla, ident->valor, a);
        if (s) {
            if (s->categoria == SIM_VARIABLE) {
                if (s->id >= 0 && (size_t)s->id < e->cantidad &&
                    e->declarados[s->id]) return s;
                if (posterior) *posterior = 1;
            } else return s; /* Parametros, funciones y otros. */
        }
        a = a->padre;
    }
    return NULL;
}

static const Simbolo *buscar_funcion(const EstadoSemantico *e, const char *nombre)
{
    if (!nombre) return NULL;
    return ts_buscar_local(e->tabla, nombre, e->tabla->ambito_global);
}

static void registrar_declaracion(const NodoAST *n, EstadoSemantico *e)
{
    const NodoAST *id = hijo_n(n, 0);
    if (!id || id->tipo != AST_VARIABLE || !id->valor) return;
    const Ambito *a = e->resolver(id, e->contexto);
    if (!a) return;
    const Simbolo *s = ts_buscar_local(e->tabla, id->valor, a);
    if (s && s->categoria == SIM_VARIABLE && s->id >= 0 &&
        (size_t)s->id < e->cantidad) e->declarados[s->id] = 1;
}

static int es_arit(const char *s)
{
    return !strcmp(s, "+") || !strcmp(s, "-") || !strcmp(s, "*") ||
           !strcmp(s, "/") || !strcmp(s, "%") || !strcmp(s, "**");
}
static int es_orden(const char *s)
{
    return !strcmp(s, "<") || !strcmp(s, ">") ||
           !strcmp(s, "<=") || !strcmp(s, ">=");
}
static int es_igualdad(const char *s)
{
    return !strcmp(s, "==") || !strcmp(s, "!=");
}
static int es_logico(const char *s)
{
    return !strcmp(s, "AND") || !strcmp(s, "OR");
}

static int es_intrinseca_feist(const char *s);

/* La inferencia de tipos no emite errores: los emite la validacion del nodo. */
static TipoDato inferir_tipo(const NodoAST *n, const EstadoSemantico *e)
{
    if (!n) return TIPO_DESCONOCIDO;
    if (n->tipo == AST_NUMERO) return TIPO_ENTERO;
    if (n->tipo == AST_BOOLEANO) return TIPO_VOF;

    if (n->tipo == AST_VARIABLE) {
        const Simbolo *s = buscar_en_orden(n, e, NULL);
        return s && (s->categoria == SIM_VARIABLE ||
                     s->categoria == SIM_PARAMETRO) &&
               s->forma == FORMA_ESCALAR ? s->tipo : TIPO_DESCONOCIDO;
    }

    if (n->tipo == AST_LLAMADA) {
        if (es_intrinseca_feist(n->valor)) return TIPO_VACIO;
        const Simbolo *s = buscar_funcion(e, n->valor);
        return s && s->categoria == SIM_FUNCION ? s->tipo : TIPO_DESCONOCIDO;
    }

    if (n->tipo == AST_ACCESO_LISTA || n->tipo == AST_ACCESO_MATRIZ) {
        const NodoAST *id = hijo_n(n, 0);
        const Simbolo *s = buscar_en_orden(id, e, NULL);
        FormaSimbolo esperada = n->tipo == AST_ACCESO_LISTA
                                ? FORMA_LISTA : FORMA_MATRIZ;
        return s && (s->categoria == SIM_VARIABLE ||
                     s->categoria == SIM_PARAMETRO) &&
               s->forma == esperada ? s->tipo : TIPO_DESCONOCIDO;
    }

    if (n->tipo != AST_OPERACION || !n->valor) return TIPO_DESCONOCIDO;
    const char *op = n->valor;
    TipoDato izq = inferir_tipo(hijo_n(n, 0), e);
    if (!strcmp(op, "unario -"))
        return izq == TIPO_ENTERO ? TIPO_ENTERO : TIPO_DESCONOCIDO;
    TipoDato der = inferir_tipo(hijo_n(n, 1), e);
    if (es_arit(op))
        return izq == TIPO_ENTERO && der == TIPO_ENTERO
               ? TIPO_ENTERO : TIPO_DESCONOCIDO;
    if (es_orden(op))
        return izq == TIPO_ENTERO && der == TIPO_ENTERO
               ? TIPO_VOF : TIPO_DESCONOCIDO;
    if (es_igualdad(op))
        return izq == der && izq != TIPO_DESCONOCIDO && izq != TIPO_VACIO
               ? TIPO_VOF : TIPO_DESCONOCIDO;
    if (es_logico(op))
        return izq == TIPO_VOF && der == TIPO_VOF
               ? TIPO_VOF : TIPO_DESCONOCIDO;
    return TIPO_DESCONOCIDO;
}

/* API sencilla, util para pruebas unitarias independientes. */
int semantico_validar_variable(const NodoAST *v, const TablaSimbolos *t,
                              const Ambito *ambito)
{
    if (!v || !t || !ambito || !v->valor || v->tipo != AST_VARIABLE) {
        fprintf(stderr, "Error interno en analisis semantico.\n");
        return 1;
    }
    for (; ambito; ambito = ambito->padre) {
        const Simbolo *s = ts_buscar_local(t, v->valor, ambito);
        if (!s) continue;
        if (s->categoria == SIM_VARIABLE || s->categoria == SIM_PARAMETRO)
            return 0;
        fprintf(stderr, "Error semantico en linea %d: '%s' no es variable.\n",
                v->linea, v->valor);
        return 1;
    }
    fprintf(stderr, "Error semantico en linea %d: variable '%s' no declarada.\n",
            v->linea, v->valor);
    return 1;
}

static int validar_uso(const NodoAST *n, const EstadoSemantico *e)
{
    int posterior = 0;
    const Simbolo *s = buscar_en_orden(n, e, &posterior);
    if (!s) {
        fprintf(stderr, "Error semantico en linea %d: variable '%s' %s.\n",
                n->linea, n->valor ? n->valor : "?",
                posterior ? "utilizada antes de declararse" : "no declarada");
        return 1;
    }
    if (s->categoria != SIM_VARIABLE && s->categoria != SIM_PARAMETRO) {
        fprintf(stderr, "Error semantico en linea %d: '%s' no es variable.\n",
                n->linea, n->valor ? n->valor : "?");
        return 1;
    }
    return 0;
}

static int validar_operacion(const NodoAST *n, const EstadoSemantico *e)
{
    if (!n->valor) return 0;
    const char *op = n->valor;
    TipoDato a = inferir_tipo(hijo_n(n, 0), e);
    if (!strcmp(op, "unario -")) {
        if (a != TIPO_DESCONOCIDO && a != TIPO_ENTERO) {
            fprintf(stderr, "Error semantico en linea %d: negativo requiere entero.\n",
                    n->linea);
            return 1;
        }
        return 0;
    }
    if (!es_arit(op) && !es_orden(op) && !es_igualdad(op) && !es_logico(op))
        return 0;
    TipoDato b = inferir_tipo(hijo_n(n, 1), e);
    if (a == TIPO_DESCONOCIDO || b == TIPO_DESCONOCIDO) return 0;
    if (((es_arit(op) || es_orden(op)) &&
         (a != TIPO_ENTERO || b != TIPO_ENTERO)) ||
        (es_logico(op) && (a != TIPO_VOF || b != TIPO_VOF)) ||
        (es_igualdad(op) && (a != b || a == TIPO_VACIO))) {
        fprintf(stderr,
                "Error semantico en linea %d: operandos incompatibles para '%s'.\n",
                n->linea, op);
        return 1;
    }
    return 0;
}

static int validar_tipo_esperado(const NodoAST *n, const NodoAST *valor,
                                 TipoDato esperado, const EstadoSemantico *e,
                                 const char *descripcion)
{
    TipoDato recibido = inferir_tipo(valor, e);
    if (recibido == TIPO_DESCONOCIDO) return 0; /* Error interno del hijo, o tipo no soportado. */
    if (recibido != esperado) {
        fprintf(stderr, "Error semantico en linea %d: tipo incompatible %s.\n",
                n->linea, descripcion);
        return 1;
    }
    return 0;
}

static int validar_declaracion_escalar(const NodoAST *n, const EstadoSemantico *e)
{
    const NodoAST *valor = hijo_n(n, 1);
    if (!valor || !n->valor) return 0;
    TipoDato esperado = TIPO_DESCONOCIDO;
    if (!strcmp(n->valor, "entero")) esperado = TIPO_ENTERO;
    if (!strcmp(n->valor, "vof")) esperado = TIPO_VOF;
    if (esperado == TIPO_DESCONOCIDO) return 0;
    return validar_tipo_esperado(n, valor, esperado, e, "al declarar variable");
}

static int validar_destino_y_asignacion(const NodoAST *n, const EstadoSemantico *e)
{
    const NodoAST *dest = hijo_n(n, 0);
    const NodoAST *valor = hijo_n(n, 1);
    if (!dest || !valor) return 0;
    TipoDato esperado = inferir_tipo(dest, e);
    if (dest->tipo == AST_VARIABLE) {
        const Simbolo *s = buscar_en_orden(dest, e, NULL);
        if (s && s->forma != FORMA_ESCALAR) {
            fprintf(stderr, "Error semantico en linea %d: no se puede asignar una lista/matriz completa.\n",
                    n->linea);
            return 1;
        }
    }
    if (esperado == TIPO_DESCONOCIDO) return 0;
    return validar_tipo_esperado(n, valor, esperado, e, "en asignacion");
}

static int validar_acceso(const NodoAST *n, const EstadoSemantico *e)
{
    const NodoAST *id = hijo_n(n, 0);
    const Simbolo *s = buscar_en_orden(id, e, NULL);
    if (!s) return 0; /* validar_uso informa el error. */
    if (s->categoria != SIM_VARIABLE && s->categoria != SIM_PARAMETRO) return 0;
    FormaSimbolo esperado = n->tipo == AST_ACCESO_LISTA ? FORMA_LISTA : FORMA_MATRIZ;
    int errores = 0;
    if (s->forma != esperado) {
        fprintf(stderr, "Error semantico en linea %d: '%s' no tiene la forma requerida (lista/matriz).\n",
                n->linea, id && id->valor ? id->valor : "?");
        errores++;
    }
    for (const NodoAST *indice = id ? id->siguiente : NULL;
         indice; indice = indice->siguiente) {
        TipoDato tipo = inferir_tipo(indice, e);
        if (tipo != TIPO_DESCONOCIDO && tipo != TIPO_ENTERO) {
            fprintf(stderr, "Error semantico en linea %d: indice debe ser entero.\n",
                    indice->linea);
            errores++;
        }
    }
    return errores;
}

/* Lee dimensiones constantes solamente; dimensiones dinamicas no son evaluables aqui. */
static int dimension_constante(const char *str, long *numero)
{
    if (!str || !*str) return 0;
    char *fin;
    long n = strtol(str, &fin, 10);
    if (*fin || n < 0) return 0;
    *numero = n;
    return 1;
}

static int validar_dimensiones(const NodoAST *n, const EstadoSemantico *e)
{
    const NodoAST *id = hijo_n(n, 0);
    const Simbolo *s = id ? ts_buscar_local(
        e->tabla, id->valor, e->resolver(id, e->contexto)) : NULL;
    if (!s || s->categoria != SIM_VARIABLE) return 0;
    int errores = 0;
    for (int k = 0; k < s->cantidad_dimensiones && k < 2; ++k) {
        const NodoAST *d = hijo_n(n, (unsigned)k + 1);
        TipoDato t = inferir_tipo(d, e);
        if (t != TIPO_DESCONOCIDO && t != TIPO_ENTERO) {
            fprintf(stderr, "Error semantico en linea %d: dimension de '%s' debe ser entero.\n",
                    d->linea, id->valor);
            errores++;
        }
        /* Una dimension cero es no negativa; no se rechaza aqui.
           La capacidad y el uso de memoria se resuelven aparte. */
    }
    return errores;
}

static int validar_literal_coleccion(const NodoAST *n, const EstadoSemantico *e)
{
    const NodoAST *id = hijo_n(n, 0);
    const Simbolo *s = id ? ts_buscar_local(
        e->tabla, id->valor, e->resolver(id, e->contexto)) : NULL;
    if (!s || s->categoria != SIM_VARIABLE) return 0;
    const NodoAST *ini = hijo_n(n, n->tipo == AST_DECL_LISTA ? 2u : 3u);
    if (!ini) return 0;
    int errores = 0;
    long tam1 = 0, tam2 = 0;
    int verificar1 = dimension_constante(s->dimensiones[0], &tam1);
    int verificar2 = s->forma == FORMA_MATRIZ &&
                    dimension_constante(s->dimensiones[1], &tam2);
    if (n->tipo == AST_DECL_LISTA && ini->tipo == AST_LISTA) {
        long elementos = 0;
        for (const NodoAST *v = ini->hijo; v; v = v->siguiente) {
            elementos++;
            TipoDato t = inferir_tipo(v, e);
            if (t != TIPO_DESCONOCIDO && t != TIPO_ENTERO) {
                fprintf(stderr, "Error semantico en linea %d: lista '%s' requiere enteros.\n",
                        v->linea, id->valor);
                errores++;
            }
        }
        if (verificar1 && elementos != tam1) {
            fprintf(stderr, "Error semantico en linea %d: longitud incorrecta de lista '%s'.\n",
                    id->linea, id->valor);
            errores++;
        }
    }
    if (n->tipo == AST_DECL_MATRIZ && ini->tipo == AST_MATRIZ) {
        long filas = 0;
        for (const NodoAST *fila = ini->hijo; fila; fila = fila->siguiente) {
            filas++;
            long columnas = 0;
            for (const NodoAST *v = fila->hijo; v; v = v->siguiente) {
                columnas++;
                TipoDato t = inferir_tipo(v, e);
                if (t != TIPO_DESCONOCIDO && t != TIPO_ENTERO) {
                    fprintf(stderr, "Error semantico en linea %d: matriz '%s' requiere enteros.\n",
                            v->linea, id->valor);
                    errores++;
                }
            }
            if (verificar2 && columnas != tam2) {
                fprintf(stderr, "Error semantico en linea %d: cantidad de columnas incorrecta en '%s'.\n",
                        id->linea, id->valor);
                errores++;
            }
        }
        if (verificar1 && filas != tam1) {
            fprintf(stderr, "Error semantico en linea %d: cantidad de filas incorrecta en '%s'.\n",
                    id->linea, id->valor);
            errores++;
        }
    }
    return errores;
}


/* FEIST_E_INTEGRACION_2026 */
/* Intrinsecas FEIST: llamadas que no necesitan simbolo de funcion.
   Reservadas para sentencias; los argumentos especiales son inmediatos ISA. */
static int es_intrinseca_feist(const char *s)
{
    return s != NULL &&
        (!strcmp(s, "feist_auth") || !strcmp(s, "feist_kstore") ||
         !strcmp(s, "feist_fround") || !strcmp(s, "feist_logout"));
}

static int literal_feist(const NodoAST *n, long minimo, long maximo)
{
    char *fin = NULL;
    long x;
    if (n == NULL || n->tipo != AST_NUMERO || n->valor == NULL)
        return 0;
    x = strtol(n->valor, &fin, 10);
    return fin != NULL && *fin == '\0' && x >= minimo && x <= maximo;
}

static int validar_intrinseca_feist(const NodoAST *n, const EstadoSemantico *e)
{
    const NodoAST *args[7] = {NULL};
    const NodoAST *a;
    const char *nombre = n->valor;
    int recibidos = 0;
    int esperados = !strcmp(nombre, "feist_auth") ? 2 :
                    !strcmp(nombre, "feist_kstore") ? 3 :
                    !strcmp(nombre, "feist_fround") ? 7 : 0;
    int errores = 0;
    int i;

    for (a = n->hijo; a; a = a->siguiente) {
        if (recibidos < 7) args[recibidos] = a;
        recibidos++;
    }
    if (recibidos != esperados) {
        fprintf(stderr, "Error semantico en linea %d: '%s' espera %d argumentos, recibio %d.\n",
                n->linea, nombre, esperados, recibidos);
        return 1;
    }
    if (!strcmp(nombre, "feist_kstore")) {
        if (!literal_feist(args[0], 0, 3) || !literal_feist(args[1], 0, 3)) {
            fprintf(stderr, "Error semantico en linea %d: llave/ronda de KSTORE deben ser literales 0..3.\n", n->linea);
            errores++;
        }
    } else if (!strcmp(nombre, "feist_fround")) {
        if (!literal_feist(args[4], 0, 3) ||
            !literal_feist(args[5], 0, 3) ||
            !literal_feist(args[6], 0, 1)) {
            fprintf(stderr, "Error semantico en linea %d: FROUND requiere llave 0..3, ronda 0..3 y modo 0..1 literales.\n", n->linea);
            errores++;
        }
        for (i = 0; i < 2; ++i) {
            const Simbolo *s;
            if (!args[i] || args[i]->tipo != AST_VARIABLE ||
                !(s = buscar_en_orden(args[i], e, NULL)) ||
                (s->categoria != SIM_VARIABLE && s->categoria != SIM_PARAMETRO) ||
                s->forma != FORMA_ESCALAR || s->tipo != TIPO_ENTERO) {
                fprintf(stderr, "Error semantico en linea %d: destino %d de FROUND debe ser variable entera escalar declarada.\n",
                        n->linea, i + 1);
                errores++;
            }
        }
        if (args[0] && args[1] && args[0]->valor && args[1]->valor &&
            !strcmp(args[0]->valor, args[1]->valor)) {
            fprintf(stderr, "Error semantico en linea %d: FROUND requiere destinos L y R distintos.\n", n->linea);
            errores++;
        }
    }

    /* Solo las fuentes de datos, no los inmediatos ni destinos de FROUND. */
    if (!strcmp(nombre, "feist_auth")) {
        for (i = 0; i < 2; ++i)
            if (inferir_tipo(args[i], e) != TIPO_DESCONOCIDO &&
                inferir_tipo(args[i], e) != TIPO_ENTERO) {
                fprintf(stderr, "Error semantico en linea %d: AUTH requiere enteros.\n", n->linea);
                errores++;
            }
    } else if (!strcmp(nombre, "feist_kstore")) {
        if (inferir_tipo(args[2], e) != TIPO_DESCONOCIDO &&
            inferir_tipo(args[2], e) != TIPO_ENTERO) {
            fprintf(stderr, "Error semantico en linea %d: KSTORE requiere subllave entera.\n", n->linea);
            errores++;
        }
    } else if (!strcmp(nombre, "feist_fround")) {
        for (i = 2; i < 4; ++i)
            if (inferir_tipo(args[i], e) != TIPO_DESCONOCIDO &&
                inferir_tipo(args[i], e) != TIPO_ENTERO) {
                fprintf(stderr, "Error semantico en linea %d: FROUND requiere mitades enteras.\n", n->linea);
                errores++;
            }
    }
    return errores;
}

static int validar_llamada(const NodoAST *n, const EstadoSemantico *e)
{
    if (!n->valor) return 1;
    if (es_intrinseca_feist(n->valor))
        return validar_intrinseca_feist(n, e);
    const Simbolo *fn = buscar_funcion(e, n->valor);
    if (!fn) {
        fprintf(stderr, "Error semantico en linea %d: funcion '%s' no declarada.\n",
                n->linea, n->valor);
        return 1;
    }
    if (fn->categoria != SIM_FUNCION) {
        fprintf(stderr, "Error semantico en linea %d: '%s' no es funcion.\n",
                n->linea, n->valor);
        return 1;
    }
    int recibidos = 0;
    for (const NodoAST *a = n->hijo; a; a = a->siguiente) recibidos++;
    if (recibidos != fn->cantidad_parametros) {
        fprintf(stderr,
                "Error semantico en linea %d: funcion '%s' espera %d argumentos, pero recibio %d.\n",
                n->linea, n->valor, fn->cantidad_parametros, recibidos);
        return 1;
    }
    int errores = 0, pos = 1;
    const ParametroFirma *p = fn->parametros;
    const NodoAST *a = n->hijo;
    while (p && a) {
        const Simbolo *esperado = p->simbolo;
        if (esperado) {
            TipoDato actual = inferir_tipo(a, e);
            FormaSimbolo forma = FORMA_ESCALAR;
            if (a->tipo == AST_VARIABLE) {
                const Simbolo *s = buscar_en_orden(a, e, NULL);
                if (s) forma = s->forma;
            }
            if (esperado->forma != forma ||
                (esperado->forma == FORMA_ESCALAR && actual != TIPO_DESCONOCIDO &&
                 actual != esperado->tipo) ||
                (esperado->forma != FORMA_ESCALAR && a->tipo == AST_VARIABLE &&
                 buscar_en_orden(a, e, NULL) &&
                 buscar_en_orden(a, e, NULL)->tipo != esperado->tipo)) {
                fprintf(stderr,
                        "Error semantico en linea %d: argumento %d de '%s' tiene un tipo o forma incompatible.\n",
                        n->linea, pos, n->valor);
                errores++;
            }
        }
        p = p->siguiente;
        a = a->siguiente;
        pos++;
    }
    return errores;
}

static int validar_retorno(const NodoAST *n, EstadoSemantico *e)
{
    if (!e->en_funcion) {
        fprintf(stderr, "Error semantico en linea %d: devuelva fuera de funcion.\n",
                n->linea);
        return 1;
    }
    e->retornos_encontrados++;
    if (e->retorno_actual == TIPO_VACIO) {
        fprintf(stderr,
                "Error semantico en linea %d: funcion '%s' es vacio y no puede devolver valor.\n",
                n->linea, e->funcion_actual);
        return 1;
    }
    return validar_tipo_esperado(n, n->hijo, e->retorno_actual, e, "en devuelva");
}

/* Procesamos hermanos uno por uno para conservar el orden del programa. */
static int recorrer_nodo(const NodoAST *n, EstadoSemantico *e);
static int recorrer_hermanos(const NodoAST *n, EstadoSemantico *e)
{
    int errores = 0;
    for (; n; n = n->siguiente) errores += recorrer_nodo(n, e);
    return errores;
}

static int recorrer_funcion(const NodoAST *n, EstadoSemantico *e)
{
    const Simbolo *fn = NULL;
    const NodoAST *cuerpo;
    const char *nombre;
    TipoDato tipo;
    if (n->tipo == AST_PRINCIPAL) {
        nombre = "principal";
        cuerpo = n->hijo;
        tipo = TIPO_VACIO;
    } else {
        const NodoAST *id = hijo_n(n, 0);
        nombre = id && id->valor ? id->valor : "?";
        cuerpo = hijo_n(n, 2);
        fn = buscar_funcion(e, nombre);
        tipo = fn && fn->categoria == SIM_FUNCION ? fn->tipo : TIPO_DESCONOCIDO;
        if (tipo == TIPO_DESCONOCIDO && n->valor) {
            if (!strcmp(n->valor, "entero")) tipo = TIPO_ENTERO;
            else if (!strcmp(n->valor, "vof")) tipo = TIPO_VOF;
            else if (!strcmp(n->valor, "vacio")) tipo = TIPO_VACIO;
        }
    }
    /* Firma anterior se conserva al regresar, aunque funciones anidadas
       no forman parte de la GLC actual. */
    const char *anterior_nombre = e->funcion_actual;
    TipoDato anterior_tipo = e->retorno_actual;
    int anterior_funcion = e->en_funcion;
    unsigned anterior_bucle = e->profundidad_bucle;
    unsigned anterior_retornos = e->retornos_encontrados;
    e->funcion_actual = nombre;
    e->retorno_actual = tipo;
    e->en_funcion = 1;
    e->profundidad_bucle = 0;
    e->retornos_encontrados = 0;

    /* Los parametros estan registrados en su ambito por el parser.
       Verificar dimensiones de la firma es una tarea adicional. */
    int errores = 0;
    if (n->tipo == AST_FUNCION)
        errores += recorrer_nodo(hijo_n(n, 1), e); /* Firma y dimensiones. */
    errores += recorrer_nodo(cuerpo, e);
    if (n->tipo == AST_FUNCION && tipo != TIPO_VACIO &&
        tipo != TIPO_DESCONOCIDO && !e->retornos_encontrados) {
        fprintf(stderr,
                "Error semantico en linea %d: funcion '%s' no tiene devuelva.\n",
                n->linea, nombre);
        errores++;
    }
    e->funcion_actual = anterior_nombre;
    e->retorno_actual = anterior_tipo;
    e->en_funcion = anterior_funcion;
    e->profundidad_bucle = anterior_bucle;
    e->retornos_encontrados = anterior_retornos;
    return errores;
}

static int recorrer_nodo(const NodoAST *n, EstadoSemantico *e)
{
    if (!n) return 0;
    int errores = 0;
    if (n->tipo == AST_FUNCION || n->tipo == AST_PRINCIPAL)
        return recorrer_funcion(n, e);
    if (n->tipo == AST_PARAMETRO) {
        const NodoAST *id = n->hijo;
        for (const NodoAST *dim = id ? id->siguiente : NULL;
             dim; dim = dim->siguiente) {
            TipoDato tipo = inferir_tipo(dim, e);
            if (tipo != TIPO_DESCONOCIDO && tipo != TIPO_ENTERO) {
                fprintf(stderr, "Error semantico en linea %d: dimension de parametro debe ser entero.\n",
                        dim->linea);
                errores++;
            }
        }
        return errores + recorrer_hermanos(id ? id->siguiente : NULL, e);
    }

    if (n->tipo == AST_DECLARACION || n->tipo == AST_DECL_LISTA ||
        n->tipo == AST_DECL_MATRIZ) {
        /* La inicializacion/dimensiones se analizan ANTES de registrar
           la nueva variable (impide autorreferencias prematuras). */
        if (n->tipo == AST_DECLARACION)
            errores += validar_declaracion_escalar(n, e);
        else {
            errores += validar_dimensiones(n, e);
            errores += validar_literal_coleccion(n, e);
        }
        const NodoAST *id = n->hijo;
        errores += recorrer_hermanos(id ? id->siguiente : NULL, e);
        registrar_declaracion(n, e);
        return errores;
    }

    switch (n->tipo) {
    case AST_VARIABLE:
        return validar_uso(n, e);
    case AST_ASIGNACION:
        errores += validar_destino_y_asignacion(n, e);
        break;
    case AST_OPERACION:
        errores += validar_operacion(n, e);
        break;
    case AST_ACCESO_LISTA:
    case AST_ACCESO_MATRIZ:
        errores += validar_acceso(n, e);
        break;
    case AST_LLAMADA:
        errores += validar_llamada(n, e);
        break;
    case AST_RETURN:
        errores += validar_retorno(n, e);
        break;
    case AST_BREAK:
        if (!e->profundidad_bucle) {
            fprintf(stderr, "Error semantico en linea %d: rompa fuera de un ciclo.\n",
                    n->linea);
            errores++;
        }
        break;
    case AST_IF:
    case AST_WHILE:
    case AST_FOR: {
        const NodoAST *cond = hijo_n(n, n->tipo == AST_FOR ? 1u : 0u);
        TipoDato tipo = inferir_tipo(cond, e);
        /* La condicion de para se revisara tras su inicializacion. */
        if (n->tipo != AST_FOR && tipo != TIPO_DESCONOCIDO && tipo != TIPO_VOF) {
            fprintf(stderr, "Error semantico en linea %d: condicion debe ser vof.\n",
                    n->linea);
            errores++;
        }
        break;
    }
    default:
        break;
    }

    if (n->tipo == AST_FOR) {
        /* AST_FOR: inicio -> condicion -> incremento -> cuerpo.
           Inicio debe procesarse antes de condicion e incremento. */
        const NodoAST *inicio = hijo_n(n, 0);
        const NodoAST *cond = hijo_n(n, 1);
        const NodoAST *incr = hijo_n(n, 2);
        const NodoAST *cuerpo = hijo_n(n, 3);
        errores += recorrer_nodo(inicio, e);
        TipoDato t = inferir_tipo(cond, e);
        if (t != TIPO_DESCONOCIDO && t != TIPO_VOF) {
            fprintf(stderr, "Error semantico en linea %d: condicion de para debe ser vof.\n",
                    n->linea);
            errores++;
        }
        errores += recorrer_nodo(cond, e);
        errores += recorrer_nodo(incr, e);
        e->profundidad_bucle++;
        errores += recorrer_nodo(cuerpo, e);
        e->profundidad_bucle--;
        return errores;
    }
    if (n->tipo == AST_WHILE) {
        errores += recorrer_nodo(hijo_n(n, 0), e);
        e->profundidad_bucle++;
        errores += recorrer_nodo(hijo_n(n, 1), e);
        e->profundidad_bucle--;
        return errores;
    }
    if (n->tipo == AST_IF) {
        /* No recorrer los hermanos del nodo de condicion como si fueran
           una lista; cada hijo representa una rama de la estructura. */
        errores += recorrer_nodo(hijo_n(n, 0), e);
        errores += recorrer_nodo(hijo_n(n, 1), e);
        errores += recorrer_nodo(hijo_n(n, 2), e);
        return errores;
    }
    return errores + recorrer_hermanos(n->hijo, e);
}

int semantico_analizar(const NodoAST *raiz, const TablaSimbolos *tabla,
                      SemanticoResolverAmbito resolver, void *contexto)
{
    if (!raiz || !tabla || !resolver) {
        fprintf(stderr, "Error: analisis semantico no inicializado.\n");
        return 1;
    }
    EstadoSemantico e = {0};
    e.tabla = tabla;
    e.resolver = resolver;
    e.contexto = contexto;
    e.cantidad = (size_t)tabla->siguiente_id_simbolo;
    e.declarados = calloc(e.cantidad ? e.cantidad : 1u, 1u);
    if (!e.declarados) {
        fprintf(stderr, "Error: sin memoria para analisis semantico.\n");
        return 1;
    }
    int errores = recorrer_hermanos(raiz, &e);
    free(e.declarados);
    return errores;
}
