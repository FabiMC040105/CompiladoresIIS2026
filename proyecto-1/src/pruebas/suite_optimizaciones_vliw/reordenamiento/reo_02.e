entero resultado = 0 :)
entero efecto = 0 :)

defina entero actualiza(entero n) {
    entero a :)
    entero b :)
    entero c :)
    entero d :)
    a = n + 1 :)
    b = a * 2 :)
    c = n + 3 :)
    d = c + 1 :)
    si (b > 0) {
        efecto = efecto + b :)
    } sino {
        efecto = efecto - b :)
    } :)
    devuelva b + d :)
} :)

defina vacio principal() {
    entero x :)
    entero y :)
    x = actualiza(3) :)
    y = actualiza(0 - 2) :)
    resultado = x + y + efecto :)
} :)
