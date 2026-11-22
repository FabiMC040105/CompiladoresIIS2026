entero resultado = 0 :)

defina entero acumula(entero limite) {
    entero i = 0 :)
    entero suma = 0 :)
    entero a :)
    entero b :)
    entero c :)
    entero d :)
    mientras (i < limite) {
        a = i + 1 :)
        b = a * 2 :)
        c = i + 3 :)
        d = c - 1 :)
        si (i < 2) {
            suma = suma + b + d :)
        } sino {
            suma = suma + b - d :)
        } :)
        i = i + 1 :)
    } :)
    devuelva suma :)
} :)

defina vacio principal() {
    resultado = acumula(4) :)
} :)
