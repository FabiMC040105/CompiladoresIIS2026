defina entero suma(entero a, entero b) {
    devuelva a + b :)
} :)

defina vacio principal() {
    entero lista datos[3] = [5, 8, 10] :)
    entero matriz m[2][2] = [[1, 2], [3, 4]] :)
    entero x = suma(datos[0], suma(2, 3)) :)
    entero i = 0 :)

    si (x > 0 $$ x < 20) {
        m[0][1] = x :)
    } sino {
        m[0][1] = 0 :)
    } :)

    para (i = 0, i < 3, i = i + 1) {
        si (i == 1 -- i == 2) {
            rompa :)
        } :)
        datos[i] = x :)
    } :)
} :)
