defina entero calcular_promedio_ventas(entero lista ventas[5], entero n) {
    entero total = 0 :)
    entero contador = 0 :)
    entero i = 0 :)

    mientras (i < n) {
        total = total + ventas[i] :)
        contador = contador + 1 :)
        i = i + 1 :)
    } :)

    si (contador == 0) {
        devuelva 0 :)
    } :)

    entero promedio = total / contador :)
    devuelva promedio :)
} :)

defina entero calcular_promedio_temperaturas(entero lista temperaturas[5], entero n) {
    entero suma = 0 :)
    entero cantidad = 0 :)
    entero j = 0 :)

    mientras (j < n) {
        suma = suma + temperaturas[j] :)
        cantidad = cantidad + 1 :)
        j = j + 1 :)
    } :)

    si (cantidad == 0) {
        devuelva 0 :)
    } :)

    entero media = suma / cantidad :)
    devuelva media :)
} :)

defina entero calcular_promedio_gastos(entero lista gastos[5], entero n) {
    entero total = 0 :)
    entero contador = 0 :)
    entero i = 0 :)

    mientras (i < n) {
        total = total + gastos[i] :)
        contador = contador + 1 :)
        i = i + 1 :)
    } :)

    si (contador == 0) {
        devuelva 0 :)
    } :)

    entero promedio = total / contador :)
    devuelva promedio :)
} :)

defina entero calcular_mediana_edades(entero lista edades[5], entero n) {
    entero lista datos[5] :)
    entero i = 0 :)
    entero j :)
    entero aux :)

    mientras (i < n) {
        datos[i] = edades[i] :)
        i = i + 1 :)
    } :)

    i = 0 :)

    mientras (i < n) {
        j = i + 1 :)

        mientras (j < n) {
            si (datos[i] > datos[j]) {
                aux = datos[i] :)
                datos[i] = datos[j] :)
                datos[j] = aux :)
            } :)

            j = j + 1 :)
        } :)

        i = i + 1 :)
    } :)

    si (n == 0) {
        devuelva 0 :)
    } :)

    entero medio = n / 2 :)

    si (n % 2 == 0) {
        devuelva (datos[medio - 1] + datos[medio]) / 2 :)
    } :)

    devuelva datos[medio] :)
} :)
