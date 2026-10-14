defina entero absoluto(entero n) {
    si (n < 0) {
        devuelva 0 - n :)
    } :)

    devuelva n :)
} :)

defina entero mcd(entero a, entero b) {
    entero residuo = 0 :)

    mientras (b != 0) {
        residuo = a % b :)

        si (residuo < 0) {
            residuo = 0 - residuo :)
        } :)

        a = b :)
        b = residuo :)
    } :)

    devuelva a :)
} :)

defina vacio principal() {
    entero numero1 = 48 :)
    entero numero2 = 18 :)

    entero positivo1 = absoluto(numero1) :)
    entero positivo2 = absoluto(numero2) :)

    entero resultado = mcd(positivo1, positivo2) :)
} :)
