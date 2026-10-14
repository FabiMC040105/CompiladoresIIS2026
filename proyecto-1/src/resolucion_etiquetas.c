#include "resolucion_etiquetas.h"
#include "etiquetas.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINEA_ASM 1024u

/*
    Representa una linea del ensamblador que se mantiene durante la Fase 5.
    Guarda el texto original, el bundle que le corresponde y las posiciones
    que se deben reemplazar si contiene una referencia a una etiqueta.
*/
typedef struct {
    char *texto;
    size_t inicio_referencia;
    size_t fin_referencia;
    size_t bundle;
    int tiene_referencia;
} LineaASM;

/*
    Almacena las lineas leidas del ensamblador en un arreglo dinamico.
    Cantidad y capacidad controlan la memoria reservada, mientras bundles
    lleva el conteo de instrucciones VLIW reconocidas durante el recorrido.
*/
typedef struct {
    LineaASM *lineas;
    size_t cantidad;
    size_t capacidad;
    size_t bundles;
} ProgramaASM;

/*
    Crea una copia del texto recibido reservando memoria para la cadena.
    Permite conservar cada linea original sin depender del buffer de lectura.
    Devuelve la copia o NULL si no se pudo reservar memoria.
*/
static char *copiar_texto(const char *cadena)
{
    size_t n = strlen(cadena) + 1u;
    char *copia = malloc(n);
    if (copia != NULL)
        memcpy(copia, cadena, n);
    return copia;
}

/*
    Libera las cadenas guardadas y el arreglo de lineas del programa.
    Al terminar reinicia la estructura para no conservar referencias a
    memoria que ya fue liberada.
*/
static void liberar_programa(ProgramaASM *p)
{
    size_t i;
    for (i = 0; i < p->cantidad; ++i)
        free(p->lineas[i].texto);
    free(p->lineas);
    memset(p, 0, sizeof(*p));
}

/*
    Agrega una linea al programa, aumentando la capacidad del arreglo
    cuando es necesario. Copia el contenido de la linea y actualiza la
    cantidad almacenada. Retorna 0 si ocurre un error de memoria.
*/
static int agregar_linea(ProgramaASM *p, const char *linea)
{
    LineaASM *nuevo;
    size_t capacidad;
    if (p->cantidad == p->capacidad) {
        capacidad = p->capacidad == 0 ? 64u : p->capacidad * 2u;
        if (capacidad < p->capacidad || capacidad > SIZE_MAX / sizeof(LineaASM))
            return 0;
        nuevo = realloc(p->lineas, capacidad * sizeof(LineaASM));
        if (nuevo == NULL)
            return 0;
        p->lineas = nuevo;
        p->capacidad = capacidad;
    }
    memset(&p->lineas[p->cantidad], 0, sizeof(LineaASM));
    p->lineas[p->cantidad].texto = copiar_texto(linea);
    if (p->lineas[p->cantidad].texto == NULL)
        return 0;
    p->cantidad++;
    return 1;
}

/*
    Avanza el puntero para ignorar los espacios al inicio del texto.
    Devuelve la posicion desde la que comienza el contenido util.
*/
static char *sin_espacios(char *p)
{
    while (isspace((unsigned char)*p)) p++;
    return p;
}

/*
    Verifica que un fragmento de texto pueda utilizarse como nombre de etiqueta.
    Debe comenzar con una letra o guion bajo y continuar con letras,
    numeros o guion bajo. El intervalo termina antes del puntero fin.
*/
static int es_nombre_valido(const char *inicio, const char *fin)
{
    const char *p;
    if (inicio >= fin || !(isalpha((unsigned char)*inicio) || *inicio == '_'))
        return 0;
    for (p = inicio + 1; p < fin; p++)
        if (!(isalnum((unsigned char)*p) || *p == '_')) return 0;
    return 1;
}

/*
    Comprueba si el operando ya contiene un valor numerico en lugar
    de una etiqueta simbolica. Utiliza strtol y verifica que se haya
    consumido todo el fragmento sin errores de rango.
*/
static int es_numero(const char *inicio, const char *fin)
{
    char *termino;
    long valor;
    if (inicio == fin) return 0;
    errno = 0;
    valor = strtol(inicio, &termino, 0);
    (void)valor;
    return errno != ERANGE && termino == fin;
}

/*
    Convierte los resultados de la tabla de etiquetas a mensajes de texto.
    Se utiliza para explicar errores de nombres duplicados, referencias,
    limites de memoria y offsets al procesar el ensamblador.
*/
static const char *nombre_error(ResultadoEtiquetas r)
{
    switch (r) {
        case ETQ_ERROR_ARGUMENTO: return "argumento invalido";
        case ETQ_ERROR_MEMORIA: return "memoria insuficiente";
        case ETQ_ERROR_ETIQUETA_DUPLICADA: return "etiqueta duplicada";
        case ETQ_ERROR_ETIQUETA_NO_DEFINIDA: return "etiqueta no definida";
        case ETQ_ERROR_REFERENCIA_DUPLICADA: return "doble referencia en bundle";
        case ETQ_ERROR_BUNDLE_INVALIDO: return "bundle fuera de memoria o programa";
        case ETQ_ERROR_OFFSET_FUERA_RANGO: return "offset fuera de rango de la ISA";
        default: return "error desconocido";
    }
}

/*
    ANALISIS DE LINEAS Y REFERENCIAS DEL ENSAMBLADOR
    Esta funcion reconoce etiquetas, instrucciones y saltos simbolicos.
    Tambien asigna a cada instruccion su indice de bundle para calcular
    despues las direcciones relativas de salto.
*/
/* Devuelve 1 si la linea tiene un bundle; 0 si es comentario/etiqueta.
   En caso de error, retorna -1. */
static int analizar_linea(ProgramaASM *p, TablaEtiquetas *tabla,
                         LineaASM *linea, size_t numero, FILE *errores)
{
    char *original = linea->texto;
    char *pos = sin_espacios(original);
    size_t largo = strlen(pos);
    char *fin = pos + largo;
    const char *operando = NULL;
    const char *termino = NULL;
    TipoSalto tipo;
    ResultadoEtiquetas r;
    char nombre[MAX_LINEA_ASM];
    int rd, rs;

/*
    Quita los espacios finales para identificar correctamente lineas
    vacias, comentarios y definiciones de etiquetas.
*/
    while (fin > pos && isspace((unsigned char)fin[-1])) fin--;
    if (fin == pos || *pos == '#') return 0;

    /* Cada etiqueta apunta al siguiente bundle en el archivo. */
    if (fin[-1] == ':' &&
        strncmp(pos, "ALU:", 4) != 0 &&
        strncmp(pos, "BRU:", 4) != 0 &&
        strncmp(pos, "LSU:", 4) != 0 &&
        strncmp(pos, "FEIST:", 6) != 0) {
        if (!es_nombre_valido(pos, fin - 1)) {
            fprintf(errores, "Fase 5, linea %zu: etiqueta invalida\n", numero);
            return -1;
        }
        memcpy(nombre, pos, (size_t)(fin - pos - 1));
        nombre[fin - pos - 1] = '\0';
        r = etq_definir(tabla, nombre, p->bundles);
        if (r != ETQ_OK) {
            fprintf(errores, "Fase 5, linea %zu: %s (%s)\n",
                    numero, nombre_error(r), nombre);
            return -1;
        }
        return 0;
    }

/*
    Asocia la instruccion con el bundle actual y analiza su unidad funcional.
    BRU puede contener saltos; en ALU se revisa ADDI cuando utiliza x1
    como registro base para construir una direccion de retorno.
*/
    linea->bundle = p->bundles;
    if (strncmp(pos, "BRU:", 4) == 0) {
        char *instruccion = sin_espacios(pos + 4);
        char op[16];
        char *ultimo;
        if (sscanf(instruccion, "%15s", op) != 1)
            goto formato_invalido;
        if (strcmp(op, "NOP") == 0) goto bundle_valido;
        if (strcmp(op, "JMP") == 0) {
            tipo = ETQ_JMP;
            operando = sin_espacios(instruccion + 3);
        } else {
            if (strcmp(op, "BEQ") == 0) tipo = ETQ_BEQ;
            else if (strcmp(op, "BNE") == 0) tipo = ETQ_BNE;
            else if (strcmp(op, "BGE") == 0) tipo = ETQ_BGE;
            else if (strcmp(op, "BLE") == 0) tipo = ETQ_BLE;
            else goto formato_invalido;
            ultimo = strrchr(instruccion, ',');
            if (ultimo == NULL) goto formato_invalido;
            operando = sin_espacios(ultimo + 1);
        }
    } else if (strncmp(pos, "ALU:", 4) == 0) {
        char *instruccion = sin_espacios(pos + 4);
        char *ultimo;
        if (strncmp(instruccion, "ADDI ", 5) != 0)
            goto bundle_valido;
        ultimo = strrchr(instruccion, ',');
        if (ultimo == NULL) goto formato_invalido;
        if (sscanf(instruccion, "ADDI x%d, x%d,", &rd, &rs) != 2)
            goto formato_invalido;
        if (rs != 1) goto bundle_valido;
        tipo = ETQ_ADDI_PC;
        operando = sin_espacios(ultimo + 1);
    } else if (strncmp(pos, "LSU:", 4) == 0 ||
               strncmp(pos, "FEIST:", 6) == 0) {
        goto bundle_valido;
    } else {
        goto formato_invalido;
    }

/*
    Ubica el operando final y revisa si es una direccion numerica o un
    nombre simbolico. Si es una etiqueta, registra la referencia y guarda
    el tramo de texto que sera sustituido por el offset calculado.
*/
    termino = operando;
    while (termino < fin && !isspace((unsigned char)*termino)) termino++;
    if (operando == termino ||
        (sin_espacios((char *)termino) < fin && *sin_espacios((char *)termino) != '#'))
        goto formato_invalido;
    if (!es_numero(operando, termino)) {
        if (!es_nombre_valido(operando, termino)) goto formato_invalido;
        memcpy(nombre, operando, (size_t)(termino - operando));
        nombre[termino - operando] = '\0';
        r = etq_agregar_salto(tabla, p->bundles, tipo, nombre);
        if (r != ETQ_OK) {
            fprintf(errores, "Fase 5, linea %zu: %s (%s)\n",
                    numero, nombre_error(r), nombre);
            return -1;
        }
        linea->tiene_referencia = 1;
        linea->inicio_referencia = (size_t)(operando - original);
        linea->fin_referencia = (size_t)(termino - original);
    }

/*
    Una instruccion valida ocupa un bundle de 16 bytes. El contador
    se incrementa aun cuando no exista una referencia simbolica.
*/
bundle_valido:
    if (p->bundles >= ETQ_MAX_BUNDLES) {
        fprintf(errores, "Fase 5, linea %zu: se superaron los %u bundles de memoria\n",
                numero, ETQ_MAX_BUNDLES);
        return -1;
    }
    p->bundles++;
    return 1;

/*
    Reporta las lineas que no cumplen el formato de ensamblador
    esperado para esta fase, indicando su numero y contenido.
*/
formato_invalido:
    fprintf(errores, "Fase 5, linea %zu: formato no reconocido: %s", numero, original);
    if (largo && original[strlen(original)-1] != '\n') fputc('\n', errores);
    return -1;
}

/*
    RESOLUCION DE ETIQUETAS - FASE 5
    Lee el ensamblador con etiquetas, registra sus posiciones y resuelve
    los destinos de saltos en offsets relativos medidos en bytes.
    Finalmente escribe otro archivo ASM con las referencias reemplazadas,
    conservando las demas lineas y comentarios del archivo original.
*/
int resolver_etiquetas_asm(const char *entrada, const char *salida, FILE *reporte)
{
    FILE *in = NULL;
    FILE *out = NULL;
    FILE *err = reporte != NULL ? reporte : stderr;
    TablaEtiquetas *tabla = NULL;
    ProgramaASM programa = {0};
    char buffer[MAX_LINEA_ASM];
    size_t n, i;
    int bien = 0;
    int salida_creada = 0;
    ResultadoEtiquetas r;

/*
    Valida las rutas para evitar reemplazar el archivo de entrada
    con el resultado de la resolucion.
*/
    if (entrada == NULL || salida == NULL || strcmp(entrada, salida) == 0) {
        fprintf(err, "Fase 5: entrada y salida deben ser rutas diferentes\n");
        return 0;
    }
    /* Evitar que un resultado viejo parezca valido si esta ejecucion falla. */
    remove(salida);
    in = fopen(entrada, "r");
    if (in == NULL) {
        fprintf(err, "Fase 5: no se pudo abrir %s\n", entrada);
        return 0;
    }
/*
    Crea la tabla temporal donde se almacenan etiquetas definidas
    y referencias a saltos encontradas durante la lectura.
*/
    tabla = etq_crear();
    if (tabla == NULL) goto limpieza;

/*
    Primera pasada: lee las lineas de entrada en orden, conserva el
    contenido original y registra la posicion de cada etiqueta y salto.
    Tambien detecta lineas que superan el tamano maximo permitido.
*/
    while (fgets(buffer, sizeof(buffer), in)) {
        LineaASM *actual;
        n = strlen(buffer);
        if (n == sizeof(buffer) - 1 && buffer[n-1] != '\n' && !feof(in)) {
            fprintf(err, "Fase 5: linea demasiado larga\n");
            goto limpieza;
        }
        if (!agregar_linea(&programa, buffer)) goto limpieza;
        actual = &programa.lineas[programa.cantidad - 1];
        if (analizar_linea(&programa, tabla, actual, programa.cantidad, err) < 0)
            goto limpieza;
    }
    if (ferror(in)) goto limpieza;

/*
    Calcula los offsets de todas las referencias una vez que se conocen
    todos los bundles y sus etiquetas. Cada offset es relativo al bundle
    de origen y debe respetar el rango de la instruccion del ISA.
*/
    r = etq_resolver(tabla, programa.bundles);
/*
    Si la resolucion falla, muestra las referencias problematicas.
    En particular indica destinos inexistentes y saltos cuyo offset
    supera el rango admitido por la arquitectura.
*/
    if (r != ETQ_OK) {
        const ReferenciaSalto *ref;
        fprintf(err, "Fase 5: %s\n", nombre_error(r));
        for (ref = tabla->primera_referencia; ref != NULL; ref = ref->siguiente) {
            const Etiqueta *destino = etq_buscar_etiqueta(tabla, ref->destino);
            if (destino == NULL) {
                fprintf(err, "  Destino inexistente: '%s' desde bundle %zu\n",
                        ref->destino, ref->bundle_origen);
            } else if (r == ETQ_ERROR_OFFSET_FUERA_RANGO) {
                int64_t off = ((int64_t)destino->indice_bundle -
                               (int64_t)ref->bundle_origen) * 16;
                int64_t limite = ref->tipo == ETQ_JMP ? 524288 : 2048;
                if (off < -limite || off >= limite) {
                    fprintf(err, "  Offset %lld fuera de rango: bundle %zu -> %s (bundle %zu)\n",
                            (long long)off, ref->bundle_origen,
                            ref->destino, destino->indice_bundle);
                }
            }
        }
        goto limpieza;
    }

/*
    Solo se crea el archivo de salida cuando todas las etiquetas
    fueron verificadas y sus offsets quedaron resueltos.
*/
    out = fopen(salida, "w");
    if (out != NULL) salida_creada = 1;
    if (out == NULL) {
        fprintf(err, "Fase 5: no se pudo escribir %s\n", salida);
        goto limpieza;
    }

/*
    Segunda pasada: vuelve a escribir las lineas del programa.
    Para una referencia simbolica sustituye solo el nombre de la etiqueta
    por el desplazamiento numerico, sin modificar el resto de la linea.
*/
    for (i = 0; i < programa.cantidad; i++) {
        LineaASM *linea = &programa.lineas[i];
        if (linea->tiene_referencia) {
            const ReferenciaSalto *ref = etq_buscar_salto(tabla, linea->bundle);
            if (ref == NULL || !ref->resuelta) goto limpieza;
            if (fprintf(out, "%.*s%d%s",
                        (int)linea->inicio_referencia, linea->texto,
                        (int)ref->offset_bytes,
                        linea->texto + linea->fin_referencia) < 0)
                goto limpieza;
        } else if (fputs(linea->texto, out) == EOF) {
            goto limpieza;
        }
    }
    if (fclose(out) != 0) { out = NULL; goto limpieza; }
    out = NULL;
    bien = 1;
/*
    Cuando se solicita un reporte, imprime la cantidad de bundles
    resueltos y el contenido de la tabla de etiquetas y referencias.
*/
    if (reporte != NULL) {
        fprintf(reporte, "Fase 5: %zu bundles resueltos correctamente\n",
                programa.bundles);
        etq_imprimir(tabla, reporte);
    }

/*
    Se ejecuta tanto al completar el proceso como al encontrar errores.
    Cierra archivos, elimina una salida incompleta y libera la memoria
    utilizada por el programa y su tabla de etiquetas.
*/
limpieza:
    if (out != NULL) fclose(out);
    if (!bien && salida_creada) remove(salida);
    if (in != NULL) fclose(in);
    etq_destruir(tabla);
    liberar_programa(&programa);
    return bien;
}
