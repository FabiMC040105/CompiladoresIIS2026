
#include "dead_code_elimination.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
Revisamos el tipo de operacion de una instruccion
Solamente consideramos las operaciones que calculan
o copian valores y guardan el resultado en una variable
No incluimos llamadas, saltos, retornos ni escrituras
en memoria porque pueden afectar el comportamiento
del programa
*/
int es_operacion_eliminable(const IRInstruction *instruccion)
{
    // Revisamos que la instruccion exista
    if (instruccion == NULL) {
        return 0;
    }

    switch (instruccion->op) {

        // Una copia guarda un valor en una variable
        case IR_COPY:
            return 1;

        // Estas operaciones calculan un resultado
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_NEG:
            return 1;

        // Conservamos las demas operaciones
        default:
            return 0;
    }
}

/*
Recorremos las instrucciones del programa para identificar
las asignaciones cuyos resultados no se necesitan despues
Usamos el OUT que calculamos en liveness
Si una instruccion define una variable y esa variable
no aparece en su OUT, puede ser codigo muerto
Esta funcion solamente muestra las candidatas
Todavia no elimina instrucciones del IR
*/
int buscar_codigo_muerto(
    const IRProgram *programa,
    const DatosInstruccion *resultados,
    EstadisticasDCE *estadisticas)
{
    if (programa == NULL || resultados == NULL ||
        estadisticas == NULL) {
        return 0;
    }

    // Empezamos las estadisticas desde cero
    estadisticas->instrucciones_originales = programa->count;
    estadisticas->posibles_eliminaciones = 0;

    printf("\n===== ANALISIS DE CODIGO MUERTO =====\n");

    // Revisamos todas las instrucciones del programa
    for (size_t i = 0; i < programa->count; i++) {

        const IRInstruction *instruccion =
            &programa->instructions[i];

        // Primero comprobamos si es una operacion eliminable
        if (!es_operacion_eliminable(instruccion)) {
            continue;
        }

        //Obtenemos la variable donde se guarda el resultado
        const char *variable = instruccion->result;

        if (variable == NULL) {
            continue;
        }

        /*
        Revisamos si la variable aparece en OUT
        Si no aparece, significa que el valor producido
        no se necesita despues de esta instruccion
        */
        if (!contiene_variable(&resultados[i].out, variable)) { //resultados[i].out es el conjunto de variables vivas después de la instrucción i

            printf(
                "Instruccion %zu: posible codigo muerto (%s)\n",
                i,
                variable
            );

            estadisticas->posibles_eliminaciones++;
        }
    }

    printf("\nInstrucciones originales: %zu\n",
           estadisticas->instrucciones_originales);

    printf("Posibles eliminaciones: %zu\n",
           estadisticas->posibles_eliminaciones);

    return 1;
}


/*
Libera los textos que pertenecen a una instruccion
El IR reserva memoria para cada cadena cuando agrega
una instruccion. Por eso debemos liberar esos campos
antes de quitarla del programa
*/
static void liberar_instruccion(IRInstruction *instruccion)
{
    if (instruccion == NULL) {
        return;
    }

    // Liberamos los cuatro campos de texto
    free(instruccion->arg1);
    free(instruccion->arg2);
    free(instruccion->result);
    free(instruccion->extra);

    // Dejamos los campos en cero para no conservar punteros viejos
    memset(instruccion, 0, sizeof(*instruccion));
}


/*
Elimina las instrucciones que marcamos como innecesarias
El arreglo eliminar tiene una posicion por instruccion:
- 1 significa que queremos eliminarla
- 0 significa que debemos conservarla
Las instrucciones conservadas se acomodan al inicio
del mismo arreglo, manteniendo su orden original
*/
static size_t quitar_instrucciones(
    IRProgram *programa,
    const unsigned char *eliminar)
{
    size_t posicion_nueva = 0;
    size_t cantidad_original = programa->count;

    // Recorremos todas las instrucciones originales
    for (size_t i = 0; i < cantidad_original; i++) {

        if (eliminar[i]) {

            // Liberamos la instruccion que ya no necesitamos
            liberar_instruccion(&programa->instructions[i]);

        } else {

            /*
            Si conservamos la instruccion, la colocamos
            en la siguiente posicion disponible
            Si ya esta en su posicion correcta,
            no necesitamos moverla
            */
            if (posicion_nueva != i) {

                programa->instructions[posicion_nueva] =
                    programa->instructions[i];

                /*
                Dejamos vacia la posicion anterior porque
                ahora sus cadenas pertenecen a la nueva posicion
                */
                memset(
                    &programa->instructions[i],
                    0,
                    sizeof(IRInstruction)
                );
            }

            posicion_nueva++;
        }
    }

    // Actualizamos la cantidad de instrucciones del programa
    programa->count = posicion_nueva;

    // Retornamos cuantas instrucciones eliminamos
    return cantidad_original - posicion_nueva;
}

/*
Revisamos si la variable tiene una declaracion local
dentro de la funcion donde aparece la asignacion
Esto nos permite distinguir una variable local de una
global sin modificar el AST ni la tabla de simbolos
Si no encontramos una declaracion, no suponemos
que la variable sea local
*/
static int tiene_declaracion_local(
    const IRProgram *programa,
    size_t posicion_asignacion)
{
    const char *nombre =
        programa->instructions[posicion_asignacion].result;

    size_t inicio_funcion = 0;
    int dentro_funcion = 0;

    // Buscamos el inicio de la funcion que contiene la asignacion
    for (size_t i = 0; i <= posicion_asignacion; i++) {

        if (programa->instructions[i].op == IR_FUNC_BEGIN) {
            inicio_funcion = i;
            dentro_funcion = 1;
        }

        if (programa->instructions[i].op == IR_FUNC_END) {
            dentro_funcion = 0;
        }
    }

    // Si la asignacion esta fuera de una funcion, no es local
    if (!dentro_funcion || nombre == NULL) {
        return 0;
    }

    // Revisamos las declaraciones anteriores a la asignacion
    for (size_t i = inicio_funcion + 1;
         i < posicion_asignacion;
         i++) {

        const IRInstruction *instruccion =
            &programa->instructions[i];

        // Una declaracion local o de parametro identifica
        // una variable que pertenece a esta funcion
        if (instruccion->op == IR_DECL ||
            instruccion->op == IR_PARAM_DECL) {

            if (instruccion->result != NULL &&
                strcmp(instruccion->result, nombre) == 0) {

                return 1;
            }
        }
    }

    return 0;
}

/*
Revisamos si una variable se utiliza en otra funcion
Esto es importante porque el OUT de una instruccion
no siempre incluye los usos que ocurren en otras funciones
Por seguridad, conservamos una asignacion cuando encontramos
que su variable se lee desde un contexto diferente
*/
static int se_usa_en_otra_funcion(
    const IRProgram *programa,
    size_t posicion_asignacion)
{
    const char *variable =
        programa->instructions[posicion_asignacion].result;

    // Guardamos el numero de funcion al que pertenece la asignacion.
    // El numero 0 representa instrucciones fuera de las funciones
    size_t funcion_asignacion = 0;
    size_t numero_funcion = 0;

    // Primero averiguamos en que funcion esta la asignacion
    for (size_t i = 0; i <= posicion_asignacion; i++) {

        if (programa->instructions[i].op == IR_FUNC_BEGIN) {
            numero_funcion++;
        }

        if (programa->instructions[i].op == IR_FUNC_END) {
            funcion_asignacion = 0;
        } else if (i == posicion_asignacion) {
            funcion_asignacion = numero_funcion;
        }
    }

    // Volvemos a recorrer el programa buscando lecturas
    numero_funcion = 0;
    size_t funcion_actual = 0;

    for (size_t i = 0; i < programa->count; i++) {

        const IRInstruction *instruccion =
            &programa->instructions[i];

        if (instruccion->op == IR_FUNC_BEGIN) {
            numero_funcion++;
            funcion_actual = numero_funcion;
        }

        if (instruccion->op == IR_FUNC_END) {
            funcion_actual = 0;
        }

        // Las lecturas de la misma funcion ya las revisa liveness
        if (funcion_actual == funcion_asignacion) {
            continue;
        }

        ConjuntoVariables use = {0};
        ConjuntoVariables def = {0};

        // Reutilizamos la funcion que ya calcula USE y DEF
        int correcto = obtener_use_def_instruccion(
            instruccion, &use, &def
        );

        int encontrada = correcto &&
            contiene_variable(&use, variable);

        liberar_conjunto(&use);
        liberar_conjunto(&def);

        // Encontramos una lectura en otra funcion
        if (encontrada) {
            return 1;
        }
    }

    return 0;
}


/* ------------ Eliminacion de codigo muerto ------------ */

/*
Ejecuta la eliminacion de codigo muerto
En cada pasada construimos el CFG, calculamos liveness
y buscamos asignaciones cuyos resultados no se utilizan
Si eliminamos alguna instruccion, repetimos el proceso
porque otras asignaciones pueden quedar innecesarias
*/
int eliminar_codigo_muerto(
    IRProgram *programa,
    ResultadoDCE *resultado)
{
    if (programa == NULL || resultado == NULL) {
        return 0;
    }

    // Inicializamos las estadisticas
    memset(resultado, 0, sizeof(*resultado));
    resultado->originales = programa->count;
    resultado->restantes = programa->count;

    // Repetimos mientras encontremos instrucciones eliminables
    while (programa->count > 0) {

        BasicBlockCFG *cfg = NULL;
        AnalisisBloques analisis = {0};
        DatosInstruccion *datos = NULL;
        unsigned char *eliminar = NULL;

        size_t cantidad = programa->count;
        size_t eliminadas_pasada = 0;
        int correcto = 1;

        // Construimos los bloques a partir del IR actual
        cfg = bb_build(programa);

        if (cfg == NULL || !bb_build_cfg(cfg)) {
            correcto = 0;
            goto limpiar;
        }

        // Calculamos USE y DEF por bloque
        if (!analizar_use_def_bloques(cfg, &analisis)) {
            correcto = 0;
            goto limpiar;
        }

        // Calculamos IN y OUT por bloque
        if (!calcular_in_out(cfg, &analisis)) {
            correcto = 0;
            goto limpiar;
        }

        /*
        Reservamos un espacio para IN y OUT de cada
        instruccion y otro para marcar las eliminaciones
        */
        datos = calloc(cantidad, sizeof(*datos));
        eliminar = calloc(cantidad, sizeof(*eliminar));

        if (datos == NULL || eliminar == NULL) {
            correcto = 0;
            goto limpiar;
        }

        // Reutilizamos el analisis por instruccion
        if (!calcular_in_out_instrucciones(
                cfg, &analisis, datos)) {
            correcto = 0;
            goto limpiar;
        }

        /*
        Revisamos las instrucciones para identificar
        cuales producen valores que ya no se necesitan
        */
        for (size_t i = 0; i < cantidad; i++) {

            const IRInstruction *instruccion =
                &programa->instructions[i];

            // No eliminamos operaciones con efectos secundarios
            if (!es_operacion_eliminable(instruccion)) {
                continue;
            }

            // Necesitamos conocer la variable definida
            if (instruccion->result == NULL) {
                continue;
            }

            // Primero revisamos si el valor se necesita mas adelante
            // dentro del flujo de control que analizamos con liveness
            int esta_viva = contiene_variable(
                &datos[i].out,
                instruccion->result
            );

            // Revisamos si la variable tiene una declaracion local.
            // Si es local, una funcion diferente no puede utilizar
            // esa misma variable solamente porque tenga igual nombre
            int es_local = tiene_declaracion_local(programa, i);

            // Cuando no podemos demostrar que es local,
            // conservamos la comprobacion de usos entre funciones
            int usada_en_otra_funcion = 0;

            if (!es_local) {
                usada_en_otra_funcion = se_usa_en_otra_funcion(
                    programa, i
                );
            }

            // Eliminamos solamente cuando el resultado no esta vivo
            // y tampoco encontramos un uso que debamos proteger
            if (!esta_viva && !usada_en_otra_funcion) {
                eliminar[i] = 1;
            }
        }

        /*
        Ya terminamos de utilizar liveness

        Liberamos sus estructuras ANTES de modificar el IR,
        porque sus conjuntos contienen referencias a las
        cadenas originales de las instrucciones
        */
limpiar:
        if (datos != NULL) {
            liberar_analisis_instrucciones(datos, cantidad);
        }

        free(datos);
        liberar_analisis_bloques(&analisis);
        bb_free(cfg);

        if (!correcto) {
            free(eliminar);
            return 0;
        }

        // Eliminamos las instrucciones que marcamos
        eliminadas_pasada = quitar_instrucciones(
            programa, eliminar
        );

        free(eliminar);

        resultado->pasadas++;
        resultado->eliminadas += eliminadas_pasada;
        resultado->restantes = programa->count;

        // Si no eliminamos nada, ya terminamos
        if (eliminadas_pasada == 0) {
            break;
        }
    }

    return 1;
}


/*
Muestra los resultados de la optimizacion DCE

Calculamos el porcentaje de instrucciones eliminadas
para comparar el tamaño del IR antes y despues
*/
void mostrar_estadisticas_dce(const ResultadoDCE *resultado)
{
    if (resultado == NULL) {
        return;
    }

    double porcentaje = 0.0;

    // Evitamos dividir entre cero cuando el programa esta vacio
    if (resultado->originales > 0) {
        porcentaje =
            100.0 * resultado->eliminadas / resultado->originales;
    }

    printf("\n===== ESTADISTICAS DCE =====\n");
    printf("Instrucciones originales: %zu\n", resultado->originales);
    printf("Instrucciones eliminadas: %zu\n", resultado->eliminadas);
    printf("Instrucciones restantes: %zu\n", resultado->restantes);
    printf("Pasadas realizadas: %zu\n", resultado->pasadas);
    printf("Porcentaje eliminado: %.2f%%\n", porcentaje);
}
