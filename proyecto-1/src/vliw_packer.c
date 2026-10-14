/*
        Empaquetador de instrucciones VLIW del compilador.
    Recibe el ensamblador con etiquetas ya resueltas, analiza las operaciones
    de cada bundle y agrupa instrucciones de diferentes unidades funcionales
    cuando las restricciones de dependencias y saltos lo permiten.
    Se mantiene la cantidad original de bundles para conservar sus direcciones.
*/
#include "vliw_packer.h"
#include "scheduler.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
        Limites de almacenamiento para las lineas de ensamblador, instrucciones
    individuales y nombres de etiquetas que procesa el empaquetador.
*/
#define VLIW_LINEA_MAX 2048u
#define VLIW_SLOT_MAX  512u
#define VLIW_LABEL_MAX 256u

/*
        Identifica los cuatro slots disponibles en la arquitectura VLIW.
    Cada indice se utiliza para acceder a la operacion de una unidad funcional
    dentro de un bundle.
*/
typedef enum {
    SLOT_ALU = 0,
    SLOT_LSU = 1,
    SLOT_BRU = 2,
    SLOT_FEIST = 3,
    SLOT_CANTIDAD = 4
} SlotVLIW;

typedef struct {
/*
        Informacion de una instruccion perteneciente a un slot.
    presente indica que el slot fue escrito en el ASM, nop identifica una
    instruccion vacia e info guarda los registros leidos y modificados.
*/
    int presente;
    int nop;
    char texto[VLIW_SLOT_MAX];       /* instruccion sin "SLOT:" */
    SchedulerInfo info;
} OperacionVLIW;

typedef struct {
/*
        Un bundle contiene un espacio para cada una de las cuatro unidades
    funcionales, aunque no todas necesitan tener una operacion activa.
*/
    OperacionVLIW op[SLOT_CANTIDAD];
} BundleVLIW;

typedef struct {
/*
        Relaciona el nombre de una etiqueta con el indice del bundle al que apunta.
*/
    char nombre[VLIW_LABEL_MAX];
    size_t bundle;
} EtiquetaVLIW;

typedef struct {
/*
        Almacena dinamicamente los bundles del programa y las etiquetas
    encontradas durante la lectura del ensamblador.
*/
    BundleVLIW *bundles;
    size_t cantidad;
    size_t capacidad;
    EtiquetaVLIW *etiquetas;
    size_t cantidad_etiquetas;
    size_t capacidad_etiquetas;
} ProgramaVLIW;

typedef struct {
/*
        Contadores de las combinaciones producidas por el empaquetamiento.
    Se utilizan para mostrar cuantos bundles se aprovecharon en cada fase.
*/
    size_t alu_lsu;
    size_t alu_bru;
    size_t lsu_bru;
    size_t feist_mix;
    size_t cuatro_slots;
} EstadisticasVLIW;

/*
    Elimina los espacios al inicio y al final de una cadena.
        Devuelve un puntero al primer caracter valido de la misma cadena.
*/
static char *trim(char *s)
{
    char *fin;
    while (isspace((unsigned char)*s)) s++;
    fin = s + strlen(s);
    while (fin > s && isspace((unsigned char)fin[-1])) *--fin = '\0';
    return s;
}

/*
    Elimina el contenido que aparece despues de # o //.
        Permite analizar las instrucciones sin interpretar los comentarios del ASM.
*/
static void quitar_comentario(char *s)
{
    char *a = strchr(s, '#');
    char *b = strstr(s, "//");
    char *c = NULL;
    if (a != NULL && b != NULL) c = a < b ? a : b;
    else c = a != NULL ? a : b;
    if (c != NULL) *c = '\0';
}

/*
    Comprueba si una linea corresponde a una etiqueta valida.
        Requiere un nombre formado por letras, numeros o guion bajo, terminado en dos puntos.
*/
static int es_nombre_etiqueta(const char *s)
{
    size_t i, n = strlen(s);
    if (n < 2 || s[n - 1] != ':') return 0;
    if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) return 0;
    for (i = 1; i + 1 < n; i++)
        if (!(isalnum((unsigned char)s[i]) || s[i] == '_')) return 0;
    return 1;
}

/*
    Devuelve el nombre textual de la unidad funcional indicada.
        Se utiliza cuando se escribe el ensamblador empaquetado.
*/
static const char *nombre_slot(SlotVLIW s)
{
    switch (s) {
        case SLOT_ALU: return "ALU";
        case SLOT_LSU: return "LSU";
        case SLOT_BRU: return "BRU";
        case SLOT_FEIST: return "FEIST";
        default: return "?";
    }
}

/*
    Convierte el nombre de un slot del ASM a su valor interno.
        Reconoce ALU, LSU, BRU y FEIST, y rechaza nombres desconocidos.
*/
static int parsear_slot(const char *s, SlotVLIW *slot)
{
    if (strcmp(s, "ALU") == 0) *slot = SLOT_ALU;
    else if (strcmp(s, "LSU") == 0) *slot = SLOT_LSU;
    else if (strcmp(s, "BRU") == 0) *slot = SLOT_BRU;
    else if (strcmp(s, "FEIST") == 0) *slot = SLOT_FEIST;
    else return 0;
    return 1;
}

/*
    Convierte una cadena a mayusculas para reconocer los nombres
        de las unidades funcionales independientemente de como esten escritos.
*/
static void mayusculas(char *s)
{
    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

/*
    Agrega un bundle al arreglo del programa.
        Si no existe capacidad suficiente, aumenta el espacio reservado mediante realloc.
*/
static int agregar_bundle(ProgramaVLIW *p, const BundleVLIW *b)
{
    BundleVLIW *nuevo;
    size_t cap;
/*
        El arreglo inicia con una capacidad pequena y se duplica cuando se llena.
    Se comprueba que el nuevo tamano no desborde la memoria reservada.
*/
    if (p->cantidad == p->capacidad) {
        cap = p->capacidad == 0 ? 64u : p->capacidad * 2u;
        if (cap < p->capacidad || cap > SIZE_MAX / sizeof(*nuevo)) return 0;
        nuevo = realloc(p->bundles, cap * sizeof(*nuevo));
        if (nuevo == NULL) return 0;
        p->bundles = nuevo;
        p->capacidad = cap;
    }
    p->bundles[p->cantidad++] = *b;
    return 1;
}

/*
    Almacena una etiqueta junto con la posicion del siguiente bundle.
        Amplia el arreglo de etiquetas cuando se alcanza su capacidad actual.
*/
static int agregar_etiqueta(ProgramaVLIW *p, const char *nombre, size_t bundle)
{
    EtiquetaVLIW *nuevo;
    size_t cap, n = strlen(nombre);
    if (n >= VLIW_LABEL_MAX) return 0;
    if (p->cantidad_etiquetas == p->capacidad_etiquetas) {
        cap = p->capacidad_etiquetas == 0 ? 16u : p->capacidad_etiquetas * 2u;
        if (cap < p->capacidad_etiquetas || cap > SIZE_MAX / sizeof(*nuevo)) return 0;
        nuevo = realloc(p->etiquetas, cap * sizeof(*nuevo));
        if (nuevo == NULL) return 0;
        p->etiquetas = nuevo;
        p->capacidad_etiquetas = cap;
    }
    memcpy(p->etiquetas[p->cantidad_etiquetas].nombre, nombre, n + 1u);
    p->etiquetas[p->cantidad_etiquetas].bundle = bundle;
    p->cantidad_etiquetas++;
    return 1;
}

/*
    Libera los arreglos de bundles y etiquetas utilizados durante
        el empaquetamiento, y reinicia la estructura del programa.
*/
static void liberar_programa(ProgramaVLIW *p)
{
    free(p->bundles);
    free(p->etiquetas);
    memset(p, 0, sizeof(*p));
}

/*
    Analiza una instruccion con formato SLOT: OPERACION.
        Identifica la unidad funcional, impide repetir un slot dentro del bundle
        y consulta al scheduler para obtener las dependencias de registros.
*/
static int parsear_operacion(char *pieza, BundleVLIW *bundle,
                             size_t numero_linea, FILE *err)
{
    char *dos = strchr(pieza, ':');
    char slot_txt[16];
    char *slot_inicio, *instr;
    size_t n;
    SlotVLIW slot;
    OperacionVLIW *op;

    if (dos == NULL) {
        fprintf(err, "Empaquetador VLIW, linea %zu: falta ':' en '%s'\n",
                numero_linea, pieza);
        return 0;
    }
    *dos = '\0';
    slot_inicio = trim(pieza);
    n = strlen(slot_inicio);
    if (n == 0 || n >= sizeof(slot_txt)) return 0;
    memcpy(slot_txt, slot_inicio, n + 1u);
    mayusculas(slot_txt);
    if (!parsear_slot(slot_txt, &slot)) {
        fprintf(err, "Empaquetador VLIW, linea %zu: slot invalido '%s'\n",
                numero_linea, slot_txt);
        return 0;
    }
    op = &bundle->op[slot];
    if (op->presente) {
        fprintf(err, "Empaquetador VLIW, linea %zu: slot %s repetido\n",
                numero_linea, slot_txt);
        return 0;
    }
    instr = trim(dos + 1);
    if (*instr == '\0' || strlen(instr) >= sizeof(op->texto)) {
        fprintf(err, "Empaquetador VLIW, linea %zu: instruccion invalida\n",
                numero_linea);
        return 0;
    }
/*
        El scheduler identifica que registros lee y escribe la instruccion.
    Sin esta informacion no se puede comprobar la seguridad de la fusion.
*/
    if (!scheduler_analizar(slot_txt, instr, &op->info)) {
        fprintf(err, "Empaquetador VLIW, linea %zu: instruccion no reconocida: %s: %s\n",
                numero_linea, slot_txt, instr);
        return 0;
    }
    op->presente = 1;
    op->nop = strcmp(instr, "NOP") == 0;
    memcpy(op->texto, instr, strlen(instr) + 1u);
    return 1;
}

/*
    Lee el ensamblador de entrada y construye los bundles en memoria.
        Ignora comentarios, registra etiquetas y permite varias instrucciones
        por linea cuando estan separadas por el caracter |.
*/
static int leer_programa(const char *ruta, ProgramaVLIW *p, FILE *err)
{
    FILE *f = fopen(ruta, "r");
    char linea[VLIW_LINEA_MAX];
    size_t numero = 0;
    if (f == NULL) {
        fprintf(err, "Empaquetador VLIW: no se pudo abrir %s\n", ruta);
        return 0;
    }
    while (fgets(linea, sizeof(linea), f) != NULL) {
        char *s, *cursor;
        BundleVLIW b;
        numero++;
        if (strchr(linea, '\n') == NULL && !feof(f)) {
            fprintf(err, "Empaquetador VLIW, linea %zu: linea demasiado larga\n", numero);
            fclose(f);
            return 0;
        }
        quitar_comentario(linea);
        s = trim(linea);
        if (*s == '\0') continue;
/*
        Las etiquetas apuntan a la cantidad de bundles procesados hasta ahora.
    No ocupan un bundle ni incrementan el contador de instrucciones.
*/
        if (es_nombre_etiqueta(s)) {
            size_t n = strlen(s);
            s[n - 1] = '\0';
            if (!agregar_etiqueta(p, s, p->cantidad)) {
                fclose(f);
                return 0;
            }
            continue;
        }
        memset(&b, 0, sizeof(b));
        cursor = s;
/*
        Separa cada segmento delimitado por | y lo coloca en el slot
    correspondiente del mismo bundle.
*/
        while (cursor != NULL) {
            char *sep = strchr(cursor, '|');
            char *pieza;
            if (sep != NULL) *sep = '\0';
            pieza = trim(cursor);
            if (*pieza == '\0' || !parsear_operacion(pieza, &b, numero, err)) {
                fclose(f);
                return 0;
            }
            cursor = sep != NULL ? sep + 1 : NULL;
        }
        if (!agregar_bundle(p, &b)) {
            fclose(f);
            return 0;
        }
    }
    if (ferror(f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

/*
    Indica si el slot contiene una instruccion activa.
        Los slots ausentes y las instrucciones NOP no se cuentan como operaciones.
*/
static int actual(const OperacionVLIW *op)
{
    return op->presente && !op->nop;
}

/*
    Cuenta cuantas unidades funcionales tienen instrucciones activas
        dentro de un mismo bundle.
*/
static int cantidad_actuales(const BundleVLIW *b)
{
    int n = 0, s;
    for (s = 0; s < SLOT_CANTIDAD; s++) if (actual(&b->op[s])) n++;
    return n;
}

/*
    Reune los registros que leen todas las operaciones activas del bundle.
        Cada posicion del arreglo representa uno de los registros de la ISA.
*/
static void lecturas_bundle(const BundleVLIW *b, unsigned char out[16])
{
    int s, r;
    memset(out, 0, 16);
    for (s = 0; s < SLOT_CANTIDAD; s++) if (actual(&b->op[s]))
        for (r = 2; r < 16; r++) if (b->op[s].info.lee[r]) out[r] = 1;
}

/*
    Reune los registros escritos por las instrucciones activas del bundle.
        Esta informacion se utiliza para detectar dependencias antes de fusionar.
*/
static void escrituras_bundle(const BundleVLIW *b, unsigned char out[16])
{
    int s, r;
    memset(out, 0, 16);
    for (s = 0; s < SLOT_CANTIDAD; s++) if (actual(&b->op[s]))
        for (r = 2; r < 16; r++) if (b->op[s].info.escribe[r]) out[r] = 1;
}

/*
    Verifica si dos conjuntos de registros tienen algun elemento en comun.
        Se utiliza para detectar posibles dependencias RAW, WAR o WAW.
*/
static int interseccion(const unsigned char a[16], const unsigned char b[16])
{
    int r;
    for (r = 2; r < 16; r++) if (a[r] && b[r]) return 1;
    return 0;
}

/*
    Detecta si una operacion distinta de BRU modifica el registro x1.
        Estas redirecciones del PC necesitan tratamiento especial para no
        mover operaciones que podrian ejecutarse fuera de su camino correcto.
*/
static int redireccion_pc_no_bru(const BundleVLIW *b)
{
    int s;
    for (s = 0; s < SLOT_CANTIDAD; s++)
        if (s != SLOT_BRU && actual(&b->op[s]) && b->op[s].info.redirecciona_pc)
            return 1;
    return 0;
}

/*
    Convierte un texto a numero y comprueba que no queden caracteres invalidos.
        Se utiliza al leer los offsets numericos de los saltos.
*/
static int parse_long_completo(const char *s, long *v)
{
    char *fin;
    errno = 0;
    *v = strtol(s, &fin, 0);
    while (isspace((unsigned char)*fin)) fin++;
    return errno != ERANGE && *s != '\0' && *fin == '\0';
}

/*
    Extrae la informacion de una instruccion de salto BRU.
        Obtiene su offset, los registros usados en la condicion y el bundle destino,
        verificando que el destino este dentro del programa y alineado a 16 bytes.
*/
static int info_branch(const OperacionVLIW *op, size_t pos, size_t total,
                       long *offset, size_t *destino,
                       unsigned char lee[16], char *nombre, size_t n_nombre)
{
    char mn[16];
    const char *t = op->texto;
    int r1, r2;
    long off;
    memset(lee, 0, 16);
    if (!actual(op) || sscanf(t, "%15s", mn) != 1) return 0;
    if (strcmp(mn, "JMP") == 0) {
        const char *p = t + 3;
        while (isspace((unsigned char)*p)) p++;
        if (!parse_long_completo(p, &off)) return 0;
    } else if (strcmp(mn, "BGE") == 0 || strcmp(mn, "BLE") == 0 ||
               strcmp(mn, "BEQ") == 0 || strcmp(mn, "BNE") == 0) {
        const char *coma = strrchr(t, ',');
        if (coma == NULL || sscanf(t, "%*s x%d, x%d", &r1, &r2) != 2) return 0;
        if (!parse_long_completo(trim((char *)coma + 1), &off)) return 0;
        if (r1 >= 2 && r1 < 16) lee[r1] = 1;
        if (r2 >= 2 && r2 < 16) lee[r2] = 1;
    } else return 0;
    {
/*
        El PC de cada bundle avanza 16 bytes. La direccion destino se obtiene
    sumando el desplazamiento del salto a la direccion de su bundle origen.
*/
        int64_t d = (int64_t)pos * 16 + (int64_t)off;
        if (d < 0 || d % 16 != 0 || d / 16 >= (int64_t)total) return 0;
        *destino = (size_t)(d / 16);
    }
    *offset = off;
    snprintf(nombre, n_nombre, "%s", mn);
    return 1;
}

/*
    Comprueba si una operacion es ADDI con origen x0 y destino temporal x4-x12.
        Esta forma se utiliza como candidato para las fusiones conservadoras
        con instrucciones LSU y BRU.
*/
static int es_addi_constante_temporal(const OperacionVLIW *op,
                                      int *rd)
{
    long imm;
    int d, rs;
    char extra;
    if (!actual(op)) return 0;
    if (sscanf(op->texto, "ADDI x%d, x%d, %ld %c", &d, &rs, &imm, &extra) < 3)
        return 0;
    if (strncmp(op->texto, "ADDI ", 5) != 0 || rs != 0 || d < 4 || d > 12)
        return 0;
    *rd = d;
    return 1;
}

/*
    Reconoce una instruccion LOAD y obtiene el registro destino y
        el registro base empleado para calcular su direccion de memoria.
*/
static int info_load(const OperacionVLIW *op, int *rd, int *base)
{
    long off;
    if (!actual(op) || strncmp(op->texto, "LOAD ", 5) != 0) return 0;
    return sscanf(op->texto, "LOAD x%d, %ld(x%d)", rd, &off, base) == 3;
}

/*
    Verifica si una operacion activa corresponde a un STORE valido.
        Se utiliza para identificar candidatos al empaquetamiento con BRU.
*/
static int es_store(const OperacionVLIW *op)
{
    int r, b;
    long off;
    return actual(op) && strncmp(op->texto, "STORE ", 6) == 0 &&
           sscanf(op->texto, "STORE x%d, %ld(x%d)", &r, &off, &b) == 3;
}

/*
    Comprueba si una instruccion FEIST utiliza x1 como registro fuente.
        Esto evita mover operaciones que dependen del valor especial del PC.
*/
static int feist_usa_x1(const OperacionVLIW *op)
{
    char mn[16];
    int a, b, c, d;
    if (!actual(op) || sscanf(op->texto, "%15s", mn) != 1) return 0;
    if (strcmp(mn, "AUTH") == 0)
        return sscanf(op->texto, "%*s x%d, x%d", &a, &b) == 2 && (a == 1 || b == 1);
    if (strcmp(mn, "KSTORE") == 0)
        return sscanf(op->texto, "%*s %*d, %*d, x%d", &a) == 1 && a == 1;
    if (strcmp(mn, "FROUND") == 0)
        return sscanf(op->texto, "%*s x%d, x%d, x%d, x%d", &a, &b, &c, &d) == 4 &&
               (c == 1 || d == 1);
    return 0;
}

/*
    Identifica las instrucciones FEIST que modifican el estado de la boveda.
        Este dato permite aplicar restricciones adicionales al reordenamiento.
*/
static int feist_cambia_boveda(const OperacionVLIW *op)
{
    return actual(op) &&
           (strncmp(op->texto, "AUTH", 4) == 0 ||
            strncmp(op->texto, "KSTORE", 6) == 0 ||
            strncmp(op->texto, "LOGOUT", 6) == 0);
}

/*
    Marca los bundles que pueden ser destinos de saltos o etiquetas.
        Estos puntos se protegen para no mover instrucciones de una forma
        que altere lo que ejecuta el procesador al llegar a una direccion.
*/
static void marcar_destinos(const ProgramaVLIW *p, unsigned char *targets)
{
    size_t i, e;
    memset(targets, 0, p->cantidad);
    for (e = 0; e < p->cantidad_etiquetas; e++)
        if (p->etiquetas[e].bundle < p->cantidad)
            targets[p->etiquetas[e].bundle] = 1;
/*
        Ademas de las etiquetas escritas en el ASM, se protegen los destinos
    calculados a partir de los saltos BRU con offsets numericos.
*/
    for (i = 0; i < p->cantidad; i++) {
        long off;
        size_t dest;
        unsigned char lee[16];
        char mn[16];
        if (info_branch(&p->bundles[i].op[SLOT_BRU], i, p->cantidad,
                        &off, &dest, lee, mn, sizeof(mn)))
            targets[dest] = 1;
    }
}

/*
    Traslada una operacion completa entre slots y deja vacio el origen.
        Se conservan tanto su texto como las dependencias calculadas.
*/
static void mover_op(OperacionVLIW *destino, OperacionVLIW *origen)
{
    *destino = *origen;
    memset(origen, 0, sizeof(*origen));
}

/*
    Reemplaza las operaciones de un bundle por un NOP de ALU.
        El bundle continua existiendo y conserva su posicion en el programa.
*/
static void poner_nop(BundleVLIW *b)
{
    SchedulerInfo info;
    int s;
    for (s = 0; s < SLOT_CANTIDAD; s++) memset(&b->op[s], 0, sizeof(b->op[s]));
    b->op[SLOT_ALU].presente = 1;
    b->op[SLOT_ALU].nop = 1;
    strcpy(b->op[SLOT_ALU].texto, "NOP");
    scheduler_analizar("ALU", "NOP", &info);
    b->op[SLOT_ALU].info = info;
}

/*
    Quita los NOP explicitos si el bundle ya contiene instrucciones activas.
        De esta forma solo se escriben las operaciones reales de los slots ocupados.
*/
static void limpiar_nops_si_hay_operacion(BundleVLIW *b)
{
    int s;
    if (cantidad_actuales(b) == 0) return;
    for (s = 0; s < SLOT_CANTIDAD; s++)
        if (b->op[s].presente && b->op[s].nop)
            memset(&b->op[s], 0, sizeof(b->op[s]));
}

/*
    Primera fase de empaquetamiento: combina LSU y ALU.
        Busca un LOAD seguido de un ADDI de constante a un registro temporal
        independiente. Mueve el ADDI al bundle del LOAD y deja un NOP en el siguiente.
*/
static void fase_alu_lsu(ProgramaVLIW *p, const unsigned char *targets,
                         EstadisticasVLIW *st)
{
    size_t i;
    for (i = 0; i + 1 < p->cantidad; i++) {
        BundleVLIW *a = &p->bundles[i];
        BundleVLIW *b = &p->bundles[i + 1];
        int rd_load, base, rd_addi;
        if (cantidad_actuales(a) != 1 || !actual(&a->op[SLOT_LSU]) ||
            !info_load(&a->op[SLOT_LSU], &rd_load, &base)) continue;
        if (cantidad_actuales(b) != 1 || !actual(&b->op[SLOT_ALU]) ||
            !es_addi_constante_temporal(&b->op[SLOT_ALU], &rd_addi)) continue;
/*
        La ALU no puede escribir el registro cargado por LSU ni su registro base.
    Tampoco se mueve un ADDI si su bundle original es destino de un salto.
*/
        if (rd_addi == rd_load || rd_addi == base || targets[i + 1]) continue;
        limpiar_nops_si_hay_operacion(a);
        mover_op(&a->op[SLOT_ALU], &b->op[SLOT_ALU]);
        poner_nop(b);
        st->alu_lsu++;
    }
}

/*
    Actualiza el offset textual de una instruccion de salto.
        Conserva su operacion y operandos originales, sustituyendo el desplazamiento.
*/
static int ajustar_branch(OperacionVLIW *op, long nuevo)
{
    char mn[16];
    int a, b;
    if (sscanf(op->texto, "%15s", mn) != 1) return 0;
    if (strcmp(mn, "JMP") == 0) {
        snprintf(op->texto, sizeof(op->texto), "JMP %ld", nuevo);
        return 1;
    }
    if (sscanf(op->texto, "%*s x%d, x%d", &a, &b) != 2) return 0;
    snprintf(op->texto, sizeof(op->texto), "%s x%d, x%d, %ld", mn, a, b, nuevo);
    return 1;
}

/*
    Segunda fase de empaquetamiento: combina BRU con ALU o LSU.
        Considera un ADDI independiente o un STORE previo a un salto,
        comprueba dependencias y modifica el offset al mover el salto un bundle atras.
*/
static void fase_bru(ProgramaVLIW *p, const unsigned char *targets,
                     EstadisticasVLIW *st)
{
    size_t i;
    for (i = 0; i + 1 < p->cantidad; i++) {
        BundleVLIW *a = &p->bundles[i];
        BundleVLIW *b = &p->bundles[i + 1];
        OperacionVLIW *branch = &b->op[SLOT_BRU];
        int fuente = -1, rd_dummy;
        long off, nuevo;
        size_t dest;
        unsigned char lee[16], escribe[16];
        char mn[16];
        int bits;
        size_t j;

        if (cantidad_actuales(a) != 1 || cantidad_actuales(b) != 1 || !actual(branch))
            continue;
        if (actual(&a->op[SLOT_ALU]) && es_addi_constante_temporal(&a->op[SLOT_ALU], &rd_dummy))
            fuente = SLOT_ALU;
        else if (actual(&a->op[SLOT_LSU]) && es_store(&a->op[SLOT_LSU]))
            fuente = SLOT_LSU;
        else continue;
        if (!info_branch(branch, i + 1, p->cantidad, &off, &dest, lee, mn, sizeof(mn)))
            continue;
/*
        Evita fusionar cuando alguno de los bundles participa como destino
    de un salto, ya que esto podria modificar el flujo de control.
*/
        if (targets[i] || targets[i + 1]) continue;
        escrituras_bundle(a, escribe);
        if (interseccion(lee, escribe)) continue;
/*
        Revisa las escrituras de los bundles anteriores para evitar que
    la condicion del salto lea un valor que aun no este disponible.
*/
        for (j = i >= 2 ? i - 2 : 0; j < i; j++) {
            escrituras_bundle(&p->bundles[j], escribe);
            if (interseccion(lee, escribe)) break;
        }
        if (j < i) continue;
/*
        Al adelantar el salto un bundle, su PC retrocede 16 bytes.
    Se suman 16 al offset para mantener exactamente la misma direccion destino.
*/
        nuevo = off + 16;
        bits = strcmp(mn, "JMP") == 0 ? 20 : 12;
        if (nuevo < -(1L << (bits - 1)) || nuevo > (1L << (bits - 1)) - 1) continue;
        if ((int64_t)i * 16 + nuevo != (int64_t)(i + 1) * 16 + off) continue;
        if (!ajustar_branch(branch, nuevo)) continue;
        limpiar_nops_si_hay_operacion(a);
        mover_op(&a->op[SLOT_BRU], branch);
        poner_nop(b);
        if (fuente == SLOT_ALU) st->alu_bru++;
        else st->lsu_bru++;
    }
}

/*
    Tercera fase de empaquetamiento: agrupa FEIST con otras unidades.
        Intenta mover una instruccion FEIST al siguiente bundle y revisa
        conflictos de registros, redirecciones del PC y cambios de la boveda.
*/
static void fase_feist(ProgramaVLIW *p, const unsigned char *targets,
                       EstadisticasVLIW *st)
{
    size_t i, j;
    for (i = 0; i + 1 < p->cantidad; i++) {
        BundleVLIW *a = &p->bundles[i];
        BundleVLIW *b = &p->bundles[i + 1];
        OperacionVLIW *f = &a->op[SLOT_FEIST];
        unsigned char fr[16], fw[16], br[16], bw[16], tmp[16];
        int peligro = 0;
        if (cantidad_actuales(a) != 1 || !actual(f)) continue;
        if (actual(&b->op[SLOT_FEIST]) || cantidad_actuales(b) == 0) continue;
        if (targets[i] || targets[i + 1] || feist_usa_x1(f)) continue;
        memcpy(fr, f->info.lee, 16);
        memcpy(fw, f->info.escribe, 16);
        lecturas_bundle(b, br);
        escrituras_bundle(b, bw);
/*
        Comprueba conflictos entre FEIST y las otras unidades:
    escrituras sobre el mismo destino, lecturas de valores escritos por FEIST
    o escrituras que modificarian los valores leidos por FEIST.
*/
        if (interseccion(fw, bw) || interseccion(fw, br) || interseccion(fr, bw)) continue;
        if (redireccion_pc_no_bru(b)) continue;
/*
        Antes de adelantar FEIST un ciclo, revisa si aun existen dependencias
    con escrituras de bundles anteriores.
*/
        for (j = i >= 3 ? i - 3 : 0; j < i; j++) {
            escrituras_bundle(&p->bundles[j], tmp);
            if (interseccion(fr, tmp)) { peligro = 1; break; }
        }
        if (peligro) continue;
/*
        Se revisan tambien las operaciones posteriores para impedir que
    una lectura utilice el resultado de FEIST antes de estar disponible.
*/
        for (j = i + 2; j < p->cantidad && j < i + 4; j++) {
            lecturas_bundle(&p->bundles[j], tmp);
            if (interseccion(fw, tmp)) { peligro = 1; break; }
        }
        if (peligro) continue;
/*
        Las instrucciones que alteran la boveda deben mantener una separacion
    segura respecto a otras operaciones FEIST posteriores.
*/
        if (feist_cambia_boveda(f)) {
            for (j = i + 2; j < p->cantidad && j < i + 4; j++)
                if (actual(&p->bundles[j].op[SLOT_FEIST])) { peligro = 1; break; }
        }
        if (peligro) continue;
        limpiar_nops_si_hay_operacion(b);
        mover_op(&b->op[SLOT_FEIST], f);
        poner_nop(a);
        st->feist_mix++;
        if (cantidad_actuales(b) == 4) st->cuatro_slots++;
    }
}

/*
    Escribe el programa resultante como ensamblador.
        Conserva las etiquetas por posicion, separa varias operaciones de
        un bundle mediante | y escribe ALU: NOP si el bundle quedo vacio.
*/
static int escribir_programa(const char *ruta, const ProgramaVLIW *p, FILE *err)
{
    FILE *f = fopen(ruta, "w");
    size_t i, e;
    int s, primero;
    if (f == NULL) {
        fprintf(err, "Empaquetador VLIW: no se pudo escribir %s\n", ruta);
        return 0;
    }
/*
        Recorre todas las posiciones, incluida la posterior al ultimo bundle,
    para conservar incluso las etiquetas definidas al final del programa.
*/
    for (i = 0; i <= p->cantidad; i++) {
        for (e = 0; e < p->cantidad_etiquetas; e++)
            if (p->etiquetas[e].bundle == i)
                fprintf(f, "%s:\n", p->etiquetas[e].nombre);
        if (i == p->cantidad) break;
        primero = 1;
        if (cantidad_actuales(&p->bundles[i]) == 0) {
            fprintf(f, "ALU: NOP\n");
            continue;
        }
        for (s = 0; s < SLOT_CANTIDAD; s++) {
            const OperacionVLIW *op = &p->bundles[i].op[s];
            if (!actual(op)) continue;
            if (!primero) fputs(" | ", f);
            fprintf(f, "%s: %s", nombre_slot((SlotVLIW)s), op->texto);
            primero = 0;
        }
        fputc('\n', f);
    }
    if (fclose(f) != 0) return 0;
    return 1;
}

/*
    Funcion principal del empaquetador VLIW.
        Lee el ASM con offsets ya resueltos, identifica los destinos de salto,
        ejecuta las tres fases de fusion y escribe el nuevo archivo ensamblador.
        Devuelve si el proceso termino correctamente y libera sus recursos.
*/
int vliw_empaquetar_archivo(const char *entrada,
                            const char *salida,
                            FILE *reporte)
{
    ProgramaVLIW p = {0};
    EstadisticasVLIW st = {0};
    unsigned char *targets = NULL;
    FILE *rep = reporte != NULL ? reporte : stdout;
    int ok = 0;

    if (entrada == NULL || salida == NULL || strcmp(entrada, salida) == 0) {
        fprintf(stderr, "Empaquetador VLIW: entrada y salida deben ser distintas\n");
        return 0;
    }
    remove(salida);
    if (!leer_programa(entrada, &p, stderr)) goto fin;
    if (p.cantidad == 0) {
        fprintf(stderr, "Empaquetador VLIW: el programa no contiene bundles\n");
        goto fin;
    }
/*
        Reserva una marca por bundle para reconocer las posiciones a las
    que puede llegar una transferencia de control.
*/
    targets = calloc(p.cantidad, 1u);
    if (targets == NULL) goto fin;
    marcar_destinos(&p, targets);

/*
        Se aplican las fusiones en orden: primero ALU+LSU, luego las
    combinaciones con BRU y finalmente las instrucciones FEIST.
    Los bundles vaciados se sustituyen por NOP y no se reduce el total.
*/
    fase_alu_lsu(&p, targets, &st);
    fase_bru(&p, targets, &st);
    fase_feist(&p, targets, &st);

    if (!escribir_programa(salida, &p, stderr)) goto fin;
    ok = 1;

/*
        Si se solicito un reporte, imprime el total de bundles conservados
    y la cantidad de fusiones realizadas en cada fase.
*/
    if (reporte != NULL) {
        fprintf(rep, "Empaquetado VLIW: %zu bundles conservados\n", p.cantidad);
        fprintf(rep, "  ALU + LSU: %zu\n", st.alu_lsu);
        fprintf(rep, "  ALU + BRU: %zu\n", st.alu_bru);
        fprintf(rep, "  LSU + BRU: %zu\n", st.lsu_bru);
        fprintf(rep, "  FEIST + otras unidades: %zu\n", st.feist_mix);
        fprintf(rep, "  Bundles con 4 slots activos: %zu\n", st.cuatro_slots);
    }

/*
        Punto comun de limpieza para las salidas exitosas y los errores.
    Si falla el empaquetamiento, elimina el archivo de salida incompleto.
*/
fin:
    if (!ok) remove(salida);
    free(targets);
    liberar_programa(&p);
    return ok;
}
