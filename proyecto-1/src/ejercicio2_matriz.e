defina entero buscar(entero matriz datos[2][3], entero objetivo) {
    entero i = 0 :)
    entero j = 0 :)

    mientras (i < 2) {
        j = 0 :)

        mientras (j < 3) {
            si (datos[i][j] == objetivo) {
                devuelva i * 3 + j :)
            } :)

            j = j + 1 :)
        } :)

        i = i + 1 :)
    } :)

    devuelva -1 :)
} :)

defina vacio principal() {
    entero matriz datos[2][3] = [[4, 8, 15], [16, 23, 42]] :)
    entero objetivo = 23 :)

    entero posicion = buscar(datos, objetivo) :)
    entero fila = posicion / 3 :)
    entero columna = posicion % 3 :)
} :)
