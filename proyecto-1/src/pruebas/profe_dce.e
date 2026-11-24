entero resultado = 0 :)

defina entero multiplicacion_matrices(
    entero matriz a[2][2],
    entero matriz b[2][2]
) {
    entero matriz c[2][2] = [[0, 0], [0, 0]] :)

    entero filas = 2 :)
    entero columnas = 2 :)
    entero comun = 2 :)
    entero total_operaciones = 0 :)

    entero i = 0 :)
    entero j = 0 :)
    entero k = 0 :)
    entero suma :)
    entero temporal_externo :)
    entero basura1 :)
    entero basura2 :)
    entero basura3 :)
    entero estadistica :)
    entero desperdicio :)

    mientras (i < filas) {
        temporal_externo = i * 100 :)
        j = 0 :)

        mientras (j < columnas) {
            basura1 = i + j :)
            suma = 0 :)
            k = 0 :)

            mientras (k < comun) {
                basura2 = k * 50 :)
                suma = suma + a[i][k] * b[k][j] :)
                total_operaciones = total_operaciones + 1 :)
                k = k + 1 :)
            } :)

            c[i][j] = suma :)
            basura3 = suma * 999 :)
            j = j + 1 :)
        } :)

        i = i + 1 :)
    } :)

    estadistica = total_operaciones :)
    desperdicio = estadistica * 2 :)

    devuelva c[0][0] + c[0][1] * 2
        + c[1][0] * 3 + c[1][1] * 4 :)
} :)

defina vacio principal() {
    entero matriz a[2][2] = [[1, 2], [3, 4]] :)
    entero matriz b[2][2] = [[5, 6], [7, 8]] :)

    resultado = multiplicacion_matrices(a, b) :)
} :)
