#include "grafo_interferencia.h"

#include <stdlib.h>
#include <string.h>

static int es_temporal(const char *nombre)
{
    return nombre != NULL && nombre[0] == '%' && nombre[1] == 't';
}

static char *duplicar_cadena(const char *s)
{
    size_t n;
    char *copia;

    if (s == NULL)
        return NULL;

    n = strlen(s) + 1;
    copia = (char *)malloc(n);
    if (copia != NULL)
        memcpy(copia, s, n);
    return copia;
}

void gi_init(GrafoInterferencia *grafo)
{
    if (grafo != NULL)
        *grafo = (GrafoInterferencia){0};
}

void gi_free(GrafoInterferencia *grafo)
{
    size_t i;

    if (grafo == NULL)
        return;

    for (i = 0; i < grafo->cantidad; i++)
        free(grafo->nodos[i].nombre);

    free(grafo->nodos);
    free(grafo->ady);
    *grafo = (GrafoInterferencia){0};
}

static int buscar_nodo(const GrafoInterferencia *grafo, const char *nombre)
{
    size_t i;

    for (i = 0; i < grafo->cantidad; i++) {
        if (strcmp(grafo->nodos[i].nombre, nombre) == 0)
            return (int)i;
    }
    return -1;
}

static int reconstruir_matriz(GrafoInterferencia *grafo,
                              size_t cantidad_anterior,
                              size_t cantidad_nueva)
{
    unsigned char *nueva;
    size_t i, j;

    nueva = (unsigned char *)calloc(cantidad_nueva * cantidad_nueva,
                                    sizeof(unsigned char));
    if (nueva == NULL)
        return 0;

    for (i = 0; i < cantidad_anterior; i++) {
        for (j = 0; j < cantidad_anterior; j++) {
            nueva[i * cantidad_nueva + j] =
                grafo->ady[i * cantidad_anterior + j];
        }
    }

    free(grafo->ady);
    grafo->ady = nueva;
    return 1;
}

static int obtener_o_agregar_nodo(GrafoInterferencia *grafo,
                                   const char *nombre)
{
    int existente;
    size_t anterior, nueva_capacidad;
    NodoInterferencia *nuevos;
    char *copia;

    if (!es_temporal(nombre))
        return -1;

    existente = buscar_nodo(grafo, nombre);
    if (existente >= 0)
        return existente;

    if (grafo->cantidad == grafo->capacidad) {
        nueva_capacidad = grafo->capacidad == 0 ? 8 : grafo->capacidad * 2;
        nuevos = (NodoInterferencia *)realloc(
            grafo->nodos, nueva_capacidad * sizeof(NodoInterferencia));
        if (nuevos == NULL)
            return -2;
        grafo->nodos = nuevos;
        grafo->capacidad = nueva_capacidad;
    }

    copia = duplicar_cadena(nombre);
    if (copia == NULL)
        return -2;

    anterior = grafo->cantidad;
    grafo->nodos[anterior].nombre = copia;
    grafo->nodos[anterior].color = -1;
    grafo->nodos[anterior].spill = 0;
    grafo->cantidad++;

    if (!reconstruir_matriz(grafo, anterior, grafo->cantidad)) {
        free(grafo->nodos[anterior].nombre);
        grafo->cantidad--;
        return -2;
    }

    return (int)anterior;
}

static void agregar_arista(GrafoInterferencia *grafo, size_t a, size_t b)
{
    size_t n = grafo->cantidad;

    if (a == b)
        return;

    grafo->ady[a * n + b] = 1;
    grafo->ady[b * n + a] = 1;
}

static int agregar_conjunto(GrafoInterferencia *grafo,
                            const ConjuntoVariables *conjunto)
{
    size_t i, j;
    int a, b;

    for (i = 0; i < conjunto->cantidad; i++) {
        a = obtener_o_agregar_nodo(grafo, conjunto->nombres[i]);
        if (a == -2)
            return 0;
    }

    /* Variables simultaneamente vivas interfieren entre si. */
    for (i = 0; i < conjunto->cantidad; i++) {
        if (!es_temporal(conjunto->nombres[i]))
            continue;

        a = buscar_nodo(grafo, conjunto->nombres[i]);
        if (a < 0)
            continue;

        for (j = i + 1; j < conjunto->cantidad; j++) {
            if (!es_temporal(conjunto->nombres[j]))
                continue;

            b = buscar_nodo(grafo, conjunto->nombres[j]);
            if (b >= 0)
                agregar_arista(grafo, (size_t)a, (size_t)b);
        }
    }

    return 1;
}

int gi_construir(const IRProgram *programa,
                 const DatosInstruccion *liveness,
                 GrafoInterferencia *grafo)
{
    size_t i;

    if (programa == NULL || liveness == NULL || grafo == NULL)
        return 0;

    gi_free(grafo);
    gi_init(grafo);

    for (i = 0; i < programa->count; i++) {
        if (!agregar_conjunto(grafo, &liveness[i].in) ||
            !agregar_conjunto(grafo, &liveness[i].out)) {
            gi_free(grafo);
            return 0;
        }
    }

    return 1;
}

static int grado_activo(const GrafoInterferencia *grafo,
                        size_t nodo,
                        const unsigned char *activo)
{
    size_t j, n = grafo->cantidad;
    int grado = 0;

    for (j = 0; j < n; j++) {
        if (activo[j] && grafo->ady[nodo * n + j])
            grado++;
    }
    return grado;
}

int gi_colorear(GrafoInterferencia *grafo, int cantidad_colores)
{
    size_t n, restantes, cima = 0, i, j;
    size_t *pila;
    unsigned char *activo;
    unsigned char *candidato_spill;

    if (grafo == NULL || cantidad_colores <= 0)
        return 0;

    n = grafo->cantidad;
    if (n == 0)
        return 1;

    pila = (size_t *)malloc(n * sizeof(size_t));
    activo = (unsigned char *)malloc(n * sizeof(unsigned char));
    candidato_spill = (unsigned char *)calloc(n, sizeof(unsigned char));
    if (pila == NULL || activo == NULL || candidato_spill == NULL) {
        free(pila);
        free(activo);
        free(candidato_spill);
        return 0;
    }

    for (i = 0; i < n; i++) {
        activo[i] = 1;
        grafo->nodos[i].color = -1;
        grafo->nodos[i].spill = 0;
    }

    restantes = n;

    /* SIMPLIFY: quitar primero nodos con grado < K. Si no existe ninguno,
       elegir como candidato de spill el de mayor grado. */
    while (restantes > 0) {
        int elegido = -1;
        int mejor_grado = -1;

        for (i = 0; i < n; i++) {
            int grado;
            if (!activo[i])
                continue;

            grado = grado_activo(grafo, i, activo);
            if (grado < cantidad_colores) {
                elegido = (int)i;
                break;
            }
        }

        if (elegido < 0) {
            for (i = 0; i < n; i++) {
                int grado;
                if (!activo[i])
                    continue;

                grado = grado_activo(grafo, i, activo);
                if (grado > mejor_grado) {
                    mejor_grado = grado;
                    elegido = (int)i;
                }
            }
            candidato_spill[elegido] = 1;
        }

        activo[elegido] = 0;
        pila[cima++] = (size_t)elegido;
        restantes--;
    }

    /* SELECT / COLOR: reinsertar en orden inverso y escoger el primer color
       que no utilice ninguno de sus vecinos ya coloreados. */
    while (cima > 0) {
        size_t nodo = pila[--cima];
        unsigned char *usado = (unsigned char *)calloc(
            (size_t)cantidad_colores, sizeof(unsigned char));
        int color = -1;

        if (usado == NULL) {
            free(pila);
            free(activo);
            free(candidato_spill);
            return 0;
        }

        for (j = 0; j < n; j++) {
            int vecino_color;
            if (!grafo->ady[nodo * n + j])
                continue;

            vecino_color = grafo->nodos[j].color;
            if (vecino_color >= 0 && vecino_color < cantidad_colores)
                usado[vecino_color] = 1;
        }

        for (j = 0; j < (size_t)cantidad_colores; j++) {
            if (!usado[j]) {
                color = (int)j;
                break;
            }
        }

        free(usado);

        if (color >= 0) {
            grafo->nodos[nodo].color = color;
            grafo->nodos[nodo].spill = 0;
        } else {
            grafo->nodos[nodo].color = -1;
            grafo->nodos[nodo].spill = 1;
        }

        (void)candidato_spill; /* candidato puede resultar coloreable al reconstruir */
    }

    free(pila);
    free(activo);
    free(candidato_spill);
    return 1;
}

size_t gi_contar_spills(const GrafoInterferencia *grafo)
{
    size_t i, total = 0;

    if (grafo == NULL)
        return 0;

    for (i = 0; i < grafo->cantidad; i++) {
        if (grafo->nodos[i].spill)
            total++;
    }
    return total;
}

void gi_imprimir(const GrafoInterferencia *grafo,
                 int registro_base,
                 FILE *salida)
{
    size_t i;

    if (grafo == NULL || salida == NULL)
        return;

    fprintf(salida, "\n===== COLOREO DEL GRAFO DE INTERFERENCIA =====\n");
    for (i = 0; i < grafo->cantidad; i++) {
        if (grafo->nodos[i].spill) {
            fprintf(salida, "%s -> SPILL\n", grafo->nodos[i].nombre);
        } else {
            fprintf(salida, "%s -> color %d -> x%d\n",
                    grafo->nodos[i].nombre,
                    grafo->nodos[i].color,
                    registro_base + grafo->nodos[i].color);
        }
    }
}

void gi_imprimir_grafo(const GrafoInterferencia *grafo,
                       FILE *salida)
{
    size_t i;
    size_t j;
    int hay_aristas = 0;

    if (grafo == NULL || salida == NULL)
        return;

    fprintf(salida,
            "\n===== GRAFO DE INTERFERENCIA =====\n");

    fprintf(salida,
            "Nodos (%zu):\n",
            grafo->cantidad);

    for (i = 0; i < grafo->cantidad; i++)
        fprintf(salida,
                "  [%zu] %s\n",
                i,
                grafo->nodos[i].nombre);

    fprintf(salida, "Aristas:\n");

    for (i = 0; i < grafo->cantidad; i++) {
        for (j = i + 1; j < grafo->cantidad; j++) {

            if (!grafo->ady[
                    i * grafo->cantidad + j
                ])
                continue;

            fprintf(salida,
                    "  %s -- %s\n",
                    grafo->nodos[i].nombre,
                    grafo->nodos[j].nombre);

            hay_aristas = 1;
        }
    }

    if (!hay_aristas)
        fprintf(salida, "  (sin aristas)\n");
}

int gi_exportar_dot(const GrafoInterferencia *grafo,
                    int registro_base,
                    const char *ruta)
{
    FILE *f;
    size_t i;
    size_t j;

    if (grafo == NULL || ruta == NULL)
        return 0;

    f = fopen(ruta, "w");

    if (f == NULL)
        return 0;

    fprintf(f, "graph Interferencia {\n");
    fprintf(f, "  node [shape=circle];\n");

    for (i = 0; i < grafo->cantidad; i++) {

        const NodoInterferencia *nodo =
            &grafo->nodos[i];

        if (nodo->spill) {

            fprintf(f,
                    "  \"%s\" [label=\"%s\\nSPILL\"];\n",
                    nodo->nombre,
                    nodo->nombre);

        } else if (nodo->color >= 0) {

            fprintf(f,
                    "  \"%s\" [label=\"%s\\ncolor %d -> x%d\"];\n",
                    nodo->nombre,
                    nodo->nombre,
                    nodo->color,
                    registro_base + nodo->color);

        } else {

            fprintf(f,
                    "  \"%s\";\n",
                    nodo->nombre);
        }
    }

    for (i = 0; i < grafo->cantidad; i++) {
        for (j = i + 1; j < grafo->cantidad; j++) {

            if (grafo->ady[
                    i * grafo->cantidad + j
                ]) {

                fprintf(f,
                        "  \"%s\" -- \"%s\";\n",
                        grafo->nodos[i].nombre,
                        grafo->nodos[j].nombre);
            }
        }
    }

    fprintf(f, "}\n");

    fclose(f);

    return 1;
}
