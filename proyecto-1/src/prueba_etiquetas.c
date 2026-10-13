#include "etiquetas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int cantidad_pruebas = 0;
static unsigned int cantidad_fallos = 0;

#define COMPROBAR(condicion, mensaje)                                           \
    do {                                                                        \
        cantidad_pruebas++;                                                      \
        if (!(condicion)) {                                                      \
            cantidad_fallos++;                                                   \
            fprintf(stderr, "  [FALLO] %s (linea %d)\n", mensaje, __LINE__);    \
        }                                                                       \
    } while (0)

static TablaEtiquetas *crear_tabla(void)
{
    TablaEtiquetas *t = etq_crear();
    if (t == NULL) {
        fprintf(stderr, "Sin memoria para crear tabla\n");
        exit(EXIT_FAILURE);
    }
    return t;
}

static void prueba_mientras(void)
{
    TablaEtiquetas *t = crear_tabla();
    const ReferenciaSalto *salida, *retorno;
    puts("\n=== PRUEBA 1: mientras, salto adelante y atras ===");

    /* 0 INICIO: evaluar condicion; 1 BEQ ... FIN; 2 cuerpo;
       3 JMP INICIO; 4 FIN: continuacion */
    COMPROBAR(etq_agregar_salto(t, 1, ETQ_BEQ, "FIN") == ETQ_OK,
              "referencia hacia adelante antes de definir etiqueta");
    COMPROBAR(etq_agregar_salto(t, 3, ETQ_JMP, "INICIO") == ETQ_OK,
              "referencia hacia atras");
    COMPROBAR(etq_definir(t, "INICIO", 0) == ETQ_OK, "definir INICIO");
    COMPROBAR(etq_definir(t, "FIN", 4) == ETQ_OK, "definir FIN");
    etq_imprimir(t, stdout);
    COMPROBAR(etq_resolver(t, 5) == ETQ_OK, "resolver ambos saltos");
    salida = etq_buscar_salto(t, 1);
    retorno = etq_buscar_salto(t, 3);
    COMPROBAR(salida != NULL && salida->resuelta && salida->offset_bytes == 48,
              "BEQ desde 0x10 hasta 0x40 es +48");
    COMPROBAR(retorno != NULL && retorno->resuelta && retorno->offset_bytes == -48,
              "JMP desde 0x30 hasta 0x00 es -48");
    etq_imprimir(t, stdout);
    etq_destruir(t);
}

static void prueba_anidamientos(void)
{
    TablaEtiquetas *t = crear_tabla();
    char inicio[64], fin_while[64], fin_if[64];
    puts("\n=== PRUEBA 2: etiquetas de estructuras anidadas ===");
    COMPROBAR(etq_generar_nombre(t, "WHILE", inicio, sizeof inicio) == ETQ_OK,
              "generar nombre unico while");
    COMPROBAR(etq_generar_nombre(t, "FIN_WHILE", fin_while,
                                 sizeof fin_while) == ETQ_OK,
              "generar nombre unico fin while");
    COMPROBAR(etq_generar_nombre(t, "FIN_IF", fin_if, sizeof fin_if) == ETQ_OK,
              "generar nombre unico if");
    COMPROBAR(strcmp(inicio, fin_while) != 0 &&
              strcmp(inicio, fin_if) != 0 &&
              strcmp(fin_while, fin_if) != 0, "nombres diferentes");

    COMPROBAR(etq_definir(t, inicio, 0) == ETQ_OK, "inicio ciclo");
    COMPROBAR(etq_definir(t, fin_if, 4) == ETQ_OK, "fin if anidado");
    COMPROBAR(etq_definir(t, fin_while, 6) == ETQ_OK, "fin while");
    COMPROBAR(etq_agregar_salto(t, 0, ETQ_BEQ, fin_while) == ETQ_OK,
              "salida del while");
    COMPROBAR(etq_agregar_salto(t, 2, ETQ_BNE, fin_if) == ETQ_OK,
              "salida del if");
    COMPROBAR(etq_agregar_salto(t, 5, ETQ_JMP, inicio) == ETQ_OK,
              "retorno al while");
    COMPROBAR(etq_resolver(t, 7) == ETQ_OK, "resolver saltos anidados");
    COMPROBAR(etq_buscar_salto(t, 0)->offset_bytes == 96, "salida while +96");
    COMPROBAR(etq_buscar_salto(t, 2)->offset_bytes == 32, "salida if +32");
    COMPROBAR(etq_buscar_salto(t, 5)->offset_bytes == -80, "retorno while -80");
    etq_imprimir(t, stdout);
    etq_destruir(t);
}

static void prueba_errores(void)
{
    TablaEtiquetas *t;
    const ReferenciaSalto *ref;
    char demasiado_corto[4];
    puts("\n=== PRUEBA 3: errores controlados ===");

    t = crear_tabla();
    COMPROBAR(etq_definir(t, "FIN", 1) == ETQ_OK, "primera definicion");
    COMPROBAR(etq_definir(t, "FIN", 2) == ETQ_ERROR_ETIQUETA_DUPLICADA,
              "no permitir etiqueta duplicada");
    COMPROBAR(etq_agregar_salto(t, 0, ETQ_JMP, "FIN") == ETQ_OK,
              "primer salto del bundle");
    COMPROBAR(etq_agregar_salto(t, 0, ETQ_BEQ, "FIN") ==
                  ETQ_ERROR_REFERENCIA_DUPLICADA,
              "unico BRU por bundle");
    COMPROBAR(etq_resolver(t, 2) == ETQ_OK, "resolver salto valido");
    COMPROBAR(etq_agregar_salto(t, 1, ETQ_JMP, "NO_EXISTE") == ETQ_OK,
              "registrar referencia sin destino");
    COMPROBAR(etq_resolver(t, 2) == ETQ_ERROR_ETIQUETA_NO_DEFINIDA,
              "detectar etiqueta ausente");
    ref = etq_buscar_salto(t, 0);
    COMPROBAR(ref != NULL && !ref->resuelta,
              "en error se invalidan offsets antiguos");
    COMPROBAR(etq_generar_nombre(t, "WHILE", demasiado_corto,
                                  sizeof demasiado_corto) == ETQ_ERROR_ARGUMENTO,
              "detectar buffer pequeno");
    etq_destruir(t);

    t = crear_tabla();
    COMPROBAR(etq_definir(t, "LEJOS", 128) == ETQ_OK, "definir destino +2048");
    COMPROBAR(etq_agregar_salto(t, 0, ETQ_BEQ, "LEJOS") == ETQ_OK,
              "salto +2048");
    COMPROBAR(etq_resolver(t, 129) == ETQ_ERROR_OFFSET_FUERA_RANGO,
              "BEQ +2048 fuera de rango firmado 12 bits");
    etq_destruir(t);

    t = crear_tabla();
    COMPROBAR(etq_definir(t, "LEJOS", 0) == ETQ_OK, "definir destino retroceso");
    COMPROBAR(etq_agregar_salto(t, 128, ETQ_BNE, "LEJOS") == ETQ_OK,
              "BNE -2048");
    COMPROBAR(etq_resolver(t, 129) == ETQ_OK, "BNE -2048 permitido");
    COMPROBAR(etq_buscar_salto(t, 128)->offset_bytes == -2048,
              "valor extremo negativo");
    etq_destruir(t);

    t = crear_tabla();
    COMPROBAR(etq_definir(t, "DEST", ETQ_MAX_BUNDLES) ==
                  ETQ_ERROR_BUNDLE_INVALIDO,
              "etiqueta fuera de memoria de instrucciones");
    COMPROBAR(etq_agregar_salto(t, ETQ_MAX_BUNDLES, ETQ_JMP, "DEST") ==
                  ETQ_ERROR_BUNDLE_INVALIDO,
              "salto fuera de memoria de instrucciones");
    COMPROBAR(etq_definir(t, "DEST", 9) == ETQ_OK, "destino en bundle 9");
    COMPROBAR(etq_resolver(t, 9) == ETQ_ERROR_BUNDLE_INVALIDO,
              "destino fuera del codigo generado");
    etq_destruir(t);
}

int main(void)
{
    prueba_mientras();
    prueba_anidamientos();
    prueba_errores();
    printf("\nRESULTADO: %u comprobaciones, %u fallos.\n",
           cantidad_pruebas, cantidad_fallos);
    if (cantidad_fallos != 0u) return EXIT_FAILURE;
    puts("Todas las pruebas del modulo de etiquetas pasaron.");
    return EXIT_SUCCESS;
}
