entero resultado = 0 :)

defina entero procesar_bloques(entero matriz bloques[2][5], entero filas, entero columnas) {
    entero total = 0 :)
    entero bloque = 0 :)
    entero i = 0 :)

    mientras (bloque < filas) {
        i = 0 :)

        mientras (i < columnas) {
            total = total + bloques[bloque][i] * 2 :)
            i = i + 1 :)
        } :)

        bloque = bloque + 1 :)
    } :)

    devuelva total :)
} :)

defina vacio principal() {
    entero matriz bloques[2][5] = [[1, 2, 3, 4, 5], [6, 7, 8, 9, 10]] :)
    resultado = procesar_bloques(bloques, 2, 5) :)
} :)
