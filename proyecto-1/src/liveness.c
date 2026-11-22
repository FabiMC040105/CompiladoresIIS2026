#include "liveness.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
Revisamos si un texto puede ser el nombre de una variable porque en IR todo está gurdado como texto
Aceptamos nombres normales como a, total o contador_1, y temporales como %t0
Las etiquetas como @L0 no son variables, porque representan destinos de saltos
*/
static int es_variable(const char *texto)
{
    size_t i = 0;
    // Si el operando no existe o esta vacio, no es una variable
    if (!texto || !texto[0]) return 0;

    // Los temporales del compilador tienen nombres como %t0 o %t15
    if (texto[0] == '%') {
        if (texto[1] != 't' || !isdigit((unsigned char)texto[2])) return 0;
        i = 2;
        while (isdigit((unsigned char)texto[i])) i++;
        return texto[i] == '\0';
    }

    // Para las variables normales permitimos letras y guion bajo al inicio
    if (!isalpha((unsigned char)texto[0]) && texto[0] != '_') return 0;
    for (i = 1; texto[i]; i++) {
        if (!isalnum((unsigned char)texto[i]) && texto[i] != '_') return 0;
    }
    return 1;
}

/*
Revisamos si el operando es un valor constante, por ejemplo 10
Estos valores no se agregan a USE porque no son nombres de variables
La funcion strtod nos ayuda a reconocer numeros escritos como texto
*/
static int es_literal(const char *texto)
{
    char *fin;
    if (!texto || !texto[0]) return 0;
    if (texto[0] == '"' || texto[0] == '\'') return 1;
    if (strcmp(texto, "verdadero") == 0 || strcmp(texto, "falso") == 0 ||
        strcmp(texto, "true") == 0 || strcmp(texto, "false") == 0) return 1;
    // strtod permite reconocer numeros sin agregarlos como variables
    (void)strtod(texto, &fin);
    return fin != texto && *fin == '\0';
}

/*
Preparamos un conjunto vacio para comenzar a guardar variables
*/
void iniciar_conjunto(ConjuntoVariables *conjunto)
{
    if (conjunto == NULL) return;

    // Empezamos sin nombres guardados y sin espacio reservado
    conjunto->nombres = NULL;
    conjunto->cantidad = 0;
    conjunto->capacidad = 0;
}

/*
Cuando terminamos de usar un conjunto, liberamos el arreglo de nombres
Los textos de las variables pertenecen al programa IR, por eso no los liberamos aqui
*/
void liberar_conjunto(ConjuntoVariables *conjunto)
{
    if (!conjunto) return;
    free(conjunto->nombres);
    iniciar_conjunto(conjunto);
}

/*
Buscamos si una variable está dentro de un conjunto
Si el conjunto tiene {a, b} y buscamos b, retorna 1
Si buscamos x, retorna 0. strcmp compara los textos de los nombres
*/
int contiene_variable(const ConjuntoVariables *conjunto, const char *nombre)
{
    if (!conjunto || !nombre) return 0;
    for (size_t i = 0; i < conjunto->cantidad; i++) {
        if (strcmp(conjunto->nombres[i], nombre) == 0) return 1;
    }
    return 0;
}

/*
Guardamos una variable dentro del conjunto, pero sin repetirla.
Si el arreglo esta lleno, reservamos mas espacio con realloc
Retorna 1 si se pudo agregar (o si ya existia) y 0 si hubo un error
*/
int agregar_variable(ConjuntoVariables *conjunto, const char *nombre)
{
    const char **nuevo;
    size_t capacidad;
    if (!conjunto || !nombre) return 0;
    if (contiene_variable(conjunto, nombre)) return 1;
    if (conjunto->cantidad == conjunto->capacidad) {
        // Si es la primera vez reservamos 8 espacios; luego duplicamos
        if (conjunto->capacidad == 0) {
            capacidad = 8;
        } else {
            capacidad = conjunto->capacidad * 2;
        }
        if (capacidad < conjunto->capacidad || capacidad > SIZE_MAX / sizeof(*nuevo)) return 0;
        nuevo = realloc(conjunto->nombres, capacidad * sizeof(*nuevo));
        if (!nuevo) return 0;
        conjunto->nombres = nuevo;
        conjunto->capacidad = capacidad;
    }
    conjunto->nombres[conjunto->cantidad] = nombre;
    conjunto->cantidad++;
    return 1;
}

/*
Mostramos en pantalla los nombres que guardamos en el conjunto
Por ejemplo, si tenemos a, b y %t0, imprime {a, b, %t0}
Esto solo muestra informacion
*/
void mostrar_conjunto(const ConjuntoVariables *conjunto, FILE *out)
{
    if (!conjunto || !out) return;
    fprintf(out, "{");
    for (size_t i = 0; i < conjunto->cantidad; i++) {
        if (i > 0) fprintf(out, ", ");
        fprintf(out, "%s", conjunto->nombres[i]);
    }
    fprintf(out, "}");
}

/* ---------- Identificando USE y DEF por instrucción ---------- */

/*
Revisamos un operando que la instruccion necesita leer
Si es un numero o texto fijo, no agregamos nada
Si es una variable, la agregamos a USE
Si el formato es desconocido, devolvemos 0 para avisar del error
*/
static int agregar_si_se_lee(ConjuntoVariables *use, const char *valor)
{
    if (valor == NULL || valor[0] == '\0' || es_literal(valor)) return 1;
    if (!es_variable(valor)) return 0;
    return agregar_variable(use, valor);
}

/*
Guardamos en DEF el nombre que recibe un resultado
*/
static int agregar_si_se_define(ConjuntoVariables *def, const char *valor)
{
    if (!es_variable(valor)) return 0;
    return agregar_variable(def, valor);
}

/*
Revisamos UNA instruccion para saber que variables lee y cuales define
Cada tipo de instruccion tiene una regla distinta en el switch
*/
int obtener_use_def_instruccion(const IRInstruction *inst,
                                   ConjuntoVariables *use,
                                   ConjuntoVariables *def)
{
    if (inst == NULL || use == NULL || def == NULL) return 0;
    // Cada caso indica que operandos se leen y donde se escribe el resultado
    switch (inst->op) {
        // Una copia o negacion lee arg1 y guarda el resultado
        case IR_COPY:
        case IR_NEG:
            return agregar_si_se_lee(use, inst->arg1) && agregar_si_se_define(def, inst->result);

        // Operaciones con dos operandos: ambos se leen y result se define
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
        case IR_MOD: case IR_POW:
        case IR_EQ: case IR_NE: case IR_LT: case IR_LE:
        case IR_GT: case IR_GE:
            return agregar_si_se_lee(use, inst->arg1) && agregar_si_se_lee(use, inst->arg2) &&
                   agregar_si_se_define(def, inst->result);

        // Los saltos condicionales leen la condicion pero no definen variables
        case IR_IF_FALSE: case IR_IF_TRUE:
            // La condicion se lee; el destino del salto es una etiqueta
            return agregar_si_se_lee(use, inst->arg1);

        // Los parametros enviados y el retorno necesitan agregar_si_se_lee su valor
        case IR_PARAM: case IR_RETURN:
            return agregar_si_se_lee(use, inst->arg1);

        case IR_CALL:
            // arg1 es nombre de funcion; arg2 es cantidad de argumentos
            // Los argumentos ya se leyeron mediante IR_PARAM
            // arg2 indica cuantas entradas recibe la funcion, no es una variable
            if (inst->result == NULL) return 1;
            return agregar_si_se_define(def, inst->result);

        // Leer una posicion de un arreglo usa el arreglo y el indice
        case IR_ARRAY_LOAD:
            // result = arg1[arg2]: usamos arreglo e indice
            return agregar_si_se_lee(use, inst->arg1) && agregar_si_se_lee(use, inst->arg2) &&
                   agregar_si_se_define(def, inst->result);

        // Para la matriz necesitamos el nombre y los dos indices
        case IR_MATRIX_LOAD:
            // result = arg1[arg2][extra].
            return agregar_si_se_lee(use, inst->arg1) && agregar_si_se_lee(use, inst->arg2) &&
                   agregar_si_se_lee(use, inst->extra) && agregar_si_se_define(def, inst->result);

        // Escribir en memoria no es lo mismo que agregar_si_se_define una variable escalar
        case IR_ARRAY_STORE:
            // result[arg1] = arg2. No definimos una variable escalar
            return agregar_si_se_lee(use, inst->result) && agregar_si_se_lee(use, inst->arg1) &&
                   agregar_si_se_lee(use, inst->arg2);

        case IR_MATRIX_STORE:
            // result[arg1][arg2] = extra
            return agregar_si_se_lee(use, inst->result) && agregar_si_se_lee(use, inst->arg1) &&
                   agregar_si_se_lee(use, inst->arg2) && agregar_si_se_lee(use, inst->extra);

        // Las declaraciones de arreglos pueden usar tamanos calculados
        case IR_ARRAY_DECL:
            return agregar_si_se_lee(use, inst->arg1) && agregar_si_se_define(def, inst->result);

        // La matriz puede necesitar dos dimensiones
        case IR_MATRIX_DECL:
            return agregar_si_se_lee(use, inst->arg1) && agregar_si_se_lee(use, inst->arg2) &&
                   agregar_si_se_define(def, inst->result);

        // Un parametro declarado queda disponible dentro de la funcion
        case IR_PARAM_DECL:
            // arg1 describe el tipo; arg2 y extra son dimensiones
            return agregar_si_se_lee(use, inst->arg2) && agregar_si_se_lee(use, inst->extra) &&
                   agregar_si_se_define(def, inst->result);

        case IR_DECL:
            // La declaracion sola no asigna un valor utilizable
            // La inicializacion, si existe, se emite aparte como IR_COPY
            return inst->result && es_variable(inst->result);

        case IR_LABEL: case IR_GOTO:
        case IR_FUNC_BEGIN: case IR_FUNC_END:
            // Etiquetas y nombres de funciones no son variables
            return 1;
    }
    return 0; // No ignoramos operaciones que no conocemos
}

/*
Liberamos la memoria que utilizamos para guardar los conjuntos
de cada bloque. Debemos liberar los cuatro conjuntos: USE, DEF, IN, OUT
Al final liberamos el arreglo donde guardamos todos los bloques y dejamos el analisis vacio
*/
void liberar_analisis_bloques(AnalisisBloques *analisis)
{
    // Revisamos que el analisis exista
    if (analisis == NULL) {
        return;
    }

    // Recorremos todos los bloques que analizamos
    for (size_t i = 0; i < analisis->cantidad_bloques; i++) {

        // Liberamos los conjuntos de este bloque
        liberar_conjunto(&analisis->bloques[i].use);
        liberar_conjunto(&analisis->bloques[i].def);
        liberar_conjunto(&analisis->bloques[i].in);
        liberar_conjunto(&analisis->bloques[i].out);
    }

    // Liberamos el arreglo que contenia todos los bloques
    free(analisis->bloques);

    // Dejamos la estructura vacia para no conservar datos anteriores
    analisis->bloques = NULL;
    analisis->cantidad_bloques = 0;
}

/* ------ Identificando USE y DEF por bloque ------ */

/*
Calculamos USE y DEF de todos los bloques del CFG
Primero calculamos USE y DEF de cada instruccion por separado
Despues agregamos al USE del bloque solamente las variables leidas
antes de que el mismo bloque les asigne un valor
En DEF guardamos las variables que reciben valores dentro del bloque
*/
int analizar_use_def_bloques(const BasicBlockCFG *cfg, AnalisisBloques *out)
{
    if (!cfg || !cfg->program || !out) return 0;
    // El analisis debe comenzar vacio
    memset(out, 0, sizeof(*out));
    out->bloques = calloc(cfg->block_count ? cfg->block_count : 1,
                         sizeof(*out->bloques));
    if (!out->bloques) return 0;
    out->cantidad_bloques = cfg->block_count;

    for (size_t b = 0; b < cfg->block_count; b++) {
        const BasicBlock *bloque = &cfg->blocks[b];
        DatosBloque *destino = &out->bloques[b];
        if (bloque->start > bloque->end || bloque->end >= cfg->program->count)
            goto error;

        // Revisamos las instrucciones del bloque en su orden original
        for (size_t i = bloque->start; i <= bloque->end; i++) {
            ConjuntoVariables usos = {0}, defs = {0};
            const IRInstruction *inst = &cfg->program->instructions[i];
            int ok = obtener_use_def_instruccion(inst, &usos, &defs);
            if (ok) {
                // Solo entra a USE del bloque lo leido antes de definirse aqui
                for (size_t j = 0; j < usos.cantidad; j++) {
                    const char *nombre = usos.nombres[j];
                    if (!contiene_variable(&destino->def, nombre) &&
                        !agregar_variable(&destino->use, nombre)) ok = 0;
                }
                // Guardamos todas las variables definidas en el bloque
                for (size_t j = 0; j < defs.cantidad; j++) {
                    if (!agregar_variable(&destino->def, defs.nombres[j])) ok = 0;
                }
            }
            liberar_conjunto(&usos);
            liberar_conjunto(&defs);
            if (!ok) {
                fprintf(stderr, "Liveness: no se pudo analizar instruccion %zu (op=%d)\n",
                        i, (int)inst->op);
                goto error;
            }
        }
    }
    return 1;
error:
    liberar_analisis_bloques(out);
    return 0;
}

/*
Mostramos USE y DEF de cada instruccion del programa
Esta funcion nos ayuda a comprobar los resultados, no elimina codigo
*/
void mostrar_use_def_instrucciones(const IRProgram *program)
{
    if (!program) return;
    printf("\n===== USE Y DEF POR INSTRUCCION =====\n");
    for (size_t i = 0; i < program->count; i++) {
        ConjuntoVariables use = {0}, def = {0};
        int ok = obtener_use_def_instruccion(&program->instructions[i], &use, &def);
        printf("Instruccion %zu: ", i);
        if (ok) {
            printf("USE = "); mostrar_conjunto(&use, stdout);
            printf("  DEF = "); mostrar_conjunto(&def, stdout);
            printf("\n");
        } else {
            printf("ERROR: operacion u operando no reconocido\n");
        }
        liberar_conjunto(&use);
        liberar_conjunto(&def);
    }
}

/*
Mostramos USE y DEF de cada bloque basico
Primero calculamos los conjuntos, luego los imprimimos y al final
liberamos la memoria que utilizamos para guardarlos
*/
void mostrar_use_def_bloques(const BasicBlockCFG *cfg)
{
    AnalisisBloques analisis = {0};
    if (!analizar_use_def_bloques(cfg, &analisis)) {
        fprintf(stderr, "No se pudo calcular USE y DEF por bloque.\n");
        return;
    }
    printf("\n===== USE Y DEF POR BLOQUE =====\n");
    for (size_t b = 0; b < analisis.cantidad_bloques; b++) {
        printf("Bloque B%zu\nUSE: ", cfg->blocks[b].id);
        mostrar_conjunto(&analisis.bloques[b].use, stdout);
        printf("\nDEF: ");
        mostrar_conjunto(&analisis.bloques[b].def, stdout);
        printf("\n");
    }
    liberar_analisis_bloques(&analisis);
}

/*
Estas dos funciones mantienen los nombres anteriores para no tengamos
que cambiar la prueba inicial que ya funcionaba
*/
void liveness_print_uses_defs(const IRProgram *programa){
    mostrar_use_def_instrucciones(programa);
}

void liveness_print_block_uses_defs(const BasicBlockCFG *cfg){
    mostrar_use_def_bloques(cfg);
}

/* ---------------- Conjuntos IN y OUT ----------------*/

/*
Une dos conjuntos de variables
Usamos agregar_variable porque esa funcion evita los repetidos
*/
static int unir_conjuntos(
    ConjuntoVariables *destino,
    const ConjuntoVariables *origen)
{
    // Revisamos que ambos conjuntos existan
    if (destino == NULL || origen == NULL) {
        return 0;
    }

    // Recorremos todas las variables del conjunto de origen
    for (size_t i = 0; i < origen->cantidad; i++) {

        // Agregamos la variable al destino si todavia no existe
        if (!agregar_variable(destino, origen->nombres[i])) {
            return 0;
        }
    }

    return 1;
}

/*
Agrega al destino las variables del primer conjunto que no estan
en el segundo conjunto
Esta operacion nos sirve para calcular OUT menos DEF
*/
static int agregar_diferencia(
    ConjuntoVariables *destino,
    const ConjuntoVariables *primero,
    const ConjuntoVariables *segundo)
{
    if (destino == NULL || primero == NULL || segundo == NULL) {
        return 0;
    }

    // Revisamos cada variable del primer conjunto
    for (size_t i = 0; i < primero->cantidad; i++) {

        const char *nombre = primero->nombres[i];

        // Solo agregamos las variables que no estan en el segundo
        if (!contiene_variable(segundo, nombre)) {

            if (!agregar_variable(destino, nombre)) {
                return 0;
            }
        }
    }

    return 1;
}

/*
Compara dos conjuntos para saber si tienen las mismas variables
El orden no importa, porque estamos trabajando con conjuntos
Retorna 1 si son iguales y 0 si son diferentes
*/
static int conjuntos_iguales(
    const ConjuntoVariables *primero,
    const ConjuntoVariables *segundo)
{
    if (primero == NULL || segundo == NULL) {
        return 0;
    }

    // Si tienen cantidades diferentes, no pueden ser iguales
    if (primero->cantidad != segundo->cantidad) {
        return 0;
    }

    // Comprobamos que todas las variables del primero esten en el segundo
    for (size_t i = 0; i < primero->cantidad; i++) {

        if (!contiene_variable(segundo, primero->nombres[i])) {
            return 0;
        }
    }

    return 1;
}

/*
Calculamos las variables vivas al entrar y salir de cada bloque

OUT contiene las variables que necesitan los bloques sucesores
IN contiene las variables que necesitamos dentro del bloque,
mas las que se necesitan despues y no se definen aqui

Repetimos el calculo porque los bloques pueden depender entre si
Terminamos cuando IN y OUT ya no cambian
*/
int calcular_in_out(
    const BasicBlockCFG *cfg,
    AnalisisBloques *analisis)
{
    if (cfg == NULL || analisis == NULL) {
        return 0;
    }

    // USE y DEF deben haberse calculado antes de llamar esta funcion
    if (cfg->block_count != analisis->cantidad_bloques) {
        return 0;
    }

    if (cfg->block_count > 0 && (cfg->blocks == NULL || analisis->bloques == NULL)) {
        return 0;
    }

    if (cfg->edge_count > 0 && cfg->edges == NULL) {
        return 0;
    }

    /*
    Verificamos las conexiones antes de comenzar
    En nuestro CFG, from y to indican posiciones del arreglo de bloques
    */
    for (size_t i = 0; i < cfg->edge_count; i++) {
        if (cfg->edges[i].from >= cfg->block_count ||
            cfg->edges[i].to >= cfg->block_count) {
            return 0;
        }
    }

    // Empezamos con IN y OUT vacios
    for (size_t b = 0; b < analisis->cantidad_bloques; b++) {
        liberar_conjunto(&analisis->bloques[b].in);
        liberar_conjunto(&analisis->bloques[b].out);
    }

    int hubo_cambios;

    do {
        // Suponemos que ya no hay cambios hasta encontrar alguno
        hubo_cambios = 0;

        /*
        Recorremos los bloques desde el ultimo hasta el primero
        Esto ayuda porque liveness analiza las necesidades
        de las instrucciones posteriores hacia las anteriores
        */
        for (size_t b = cfg->block_count; b > 0; b--) {

            size_t actual = b - 1;
            DatosBloque *bloque = &analisis->bloques[actual];

            // Preparamos los nuevos conjuntos de este bloque.
            ConjuntoVariables nuevo_in = {0};
            ConjuntoVariables nuevo_out = {0};

            /*
            PRIMERA PARTE: CALCULAR OUT

            Buscamos todas las conexiones que salen de este bloque.
            Por cada sucesor, agregamos su IN al nuevo OUT
            */
            for (size_t e = 0; e < cfg->edge_count; e++) {

                CFGEdge conexion = cfg->edges[e];

                if (conexion.from == actual) {

                    size_t sucesor = conexion.to;

                    if (!unir_conjuntos(
                            &nuevo_out,
                            &analisis->bloques[sucesor].in)) {

                        liberar_conjunto(&nuevo_in);
                        liberar_conjunto(&nuevo_out);
                        return 0;
                    }
                }
            }

            /*
            SEGUNDA PARTE: CALCULAR IN

            Usamos la ecuacion: IN = USE union (OUT - DEF)

            Primero agregamos USE
            Despues agregamos las variables de OUT que no estan en DEF
            */
            if (!unir_conjuntos(&nuevo_in, &bloque->use) ||
                !agregar_diferencia(
                    &nuevo_in,
                    &nuevo_out,
                    &bloque->def)) {

                liberar_conjunto(&nuevo_in);
                liberar_conjunto(&nuevo_out);
                return 0;
            }

            /*
            TERCERA PARTE: REVISAR SI HUBO CAMBIOS

            Si IN u OUT son diferentes de los anteriores,
            significa que debemos continuar con otra iteracion
            */
            if (!conjuntos_iguales(&bloque->in, &nuevo_in) ||
                !conjuntos_iguales(&bloque->out, &nuevo_out)) {

                hubo_cambios = 1;
            }

            // Liberamos los resultados anteriores
            liberar_conjunto(&bloque->in);
            liberar_conjunto(&bloque->out);

            // Guardamos los conjuntos que acabamos de calcular
            bloque->in = nuevo_in;
            bloque->out = nuevo_out;
        }

    } while (hubo_cambios);

    return 1;
}

/*
Muestra los conjuntos IN y OUT de cada bloque
No modifica las instrucciones del programa
*/
void mostrar_in_out_bloques(
    const BasicBlockCFG *cfg,
    const AnalisisBloques *analisis)
{
    if (cfg == NULL || analisis == NULL) {
        return;
    }

    if (cfg->block_count != analisis->cantidad_bloques) {
        return;
    }

    printf("\n===== ANALISIS IN Y OUT =====\n");

    for (size_t b = 0; b < analisis->cantidad_bloques; b++) {

        printf("\nBloque B%zu\n", cfg->blocks[b].id);

        printf("USE: ");
        mostrar_conjunto(&analisis->bloques[b].use, stdout);

        printf("\nDEF: ");
        mostrar_conjunto(&analisis->bloques[b].def, stdout);

        printf("\nIN: ");
        mostrar_conjunto(&analisis->bloques[b].in, stdout);

        printf("\nOUT: ");
        mostrar_conjunto(&analisis->bloques[b].out, stdout);

        printf("\n");
    }
}


/* ------------ IN y OUT por instruccion ------------ */

/*
Liberamos los conjuntos que utilizamos para guardar
las variables vivas de cada instruccion
No liberamos el arreglo resultados porque ese arreglo
lo crea y administra la funcion que llama al analisis
*/
void liberar_analisis_instrucciones(
    DatosInstruccion *resultados,
    size_t cantidad)
{
    if (resultados == NULL) {
        return;
    }

    // Recorremos cada instruccion para liberar sus conjuntos
    for (size_t i = 0; i < cantidad; i++) {
        liberar_conjunto(&resultados[i].in);
        liberar_conjunto(&resultados[i].out);
    }
}

/*
Calculamos las variables vivas antes y despues
de cada instruccion del programa

Ya tenemos IN y OUT de cada bloque, entonces
los utilizamos para analizar sus instrucciones

Recorremos cada bloque desde su ultima instruccion
hasta la primera porque necesitamos conocer primero
que variables se utilizan mas adelante
*/
int calcular_in_out_instrucciones(
    const BasicBlockCFG *cfg,
    const AnalisisBloques *analisis,
    DatosInstruccion *resultados)
{
    if (cfg == NULL || analisis == NULL || resultados == NULL) {
        return 0;
    }

    if (cfg->program == NULL ||
        cfg->block_count != analisis->cantidad_bloques) {
        return 0;
    }

    /*
    Revisamos cada bloque del programa:
    El IN y OUT de los bloques ya debe estar calculado
    antes de ejecutar esta funcion
    */
    for (size_t b = 0; b < cfg->block_count; b++) {

        const BasicBlock *bloque = &cfg->blocks[b];

        /*
        La ultima instruccion del bloque tiene como OUT
        el mismo OUT que calculamos para todo el bloque
        */
        ConjuntoVariables vivas = {0};

        if (!unir_conjuntos(&vivas, &analisis->bloques[b].out)) {
            liberar_conjunto(&vivas);
            return 0;
        }

        /*
        Recorremos las instrucciones de atras hacia adelante
        Usamos i > start para evitar problemas al restar
        cuando la primera instruccion tiene indice cero
        */
        for (size_t i = bloque->end + 1; i > bloque->start;) {

            i--;

            // Obtenemos la instruccion que estamos analizando
            const IRInstruction *instruccion =
                &cfg->program->instructions[i];

            // Guardamos los USE y DEF de esta instruccion
            ConjuntoVariables use = {0};
            ConjuntoVariables def = {0};

            // Reutilizamos nuestra funcion anterior
            if (!obtener_use_def_instruccion(
                    instruccion, &use, &def)) {

                liberar_conjunto(&use);
                liberar_conjunto(&def);
                liberar_conjunto(&vivas);
                return 0;
            }

            /*
            OUT de esta instruccion contiene las variables
            que se necesitan despues de ejecutarla
            Como recorremos hacia atras, esas variables
            ya estan guardadas en el conjunto vivas
            */
            if (!unir_conjuntos(&resultados[i].out, &vivas)) {
                liberar_conjunto(&use);
                liberar_conjunto(&def);
                liberar_conjunto(&vivas);
                return 0;
            }

            /*
            Calculamos IN usando: IN = USE union (OUT - DEF)
            Primero agregamos las variables que esta
            instruccion necesita leer
            */
            if (!unir_conjuntos(&resultados[i].in, &use)) {
                liberar_conjunto(&use);
                liberar_conjunto(&def);
                liberar_conjunto(&vivas);
                return 0;
            }

            /*
            Despues agregamos las variables que siguen
            vivas al salir, excepto las que esta misma
            instruccion define
            */
            if (!agregar_diferencia(
                    &resultados[i].in,
                    &resultados[i].out,
                    &def)) {

                liberar_conjunto(&use);
                liberar_conjunto(&def);
                liberar_conjunto(&vivas);
                return 0;
            }

            /*
            El IN que acabamos de calcular sera el OUT
            de la instruccion anterior
            Por eso actualizamos el conjunto vivas
            */
            liberar_conjunto(&vivas);

            if (!unir_conjuntos(&vivas, &resultados[i].in)) {
                liberar_conjunto(&use);
                liberar_conjunto(&def);
                liberar_conjunto(&vivas);
                return 0;
            }

            // Ya terminamos de utilizar USE y DEF temporales
            liberar_conjunto(&use);
            liberar_conjunto(&def);
        }

        // Terminamos de analizar este bloque
        liberar_conjunto(&vivas);
    }

    return 1;
}

/*
Mostramos IN y OUT de cada instruccion para revisar
que el analisis funcione como esperamos
Esta funcion solamente imprime los resultados
No modifica ni elimina instrucciones del programa
*/
void mostrar_in_out_instrucciones(
    const IRProgram *programa,
    const DatosInstruccion *resultados)
{
    if (programa == NULL || resultados == NULL) {
        return;
    }

    printf("\n===== IN Y OUT POR INSTRUCCION =====\n");

    for (size_t i = 0; i < programa->count; i++) {

        printf("\nInstruccion %zu\n", i);

        printf("IN: ");
        mostrar_conjunto(&resultados[i].in, stdout);

        printf("\nOUT: ");
        mostrar_conjunto(&resultados[i].out, stdout);

        printf("\n");
    }
}
