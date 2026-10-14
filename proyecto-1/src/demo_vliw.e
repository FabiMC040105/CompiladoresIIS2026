defina vacio principal() {
    entero authid = 1 :)
    entero authpw = 2 :)
    entero llave = 9 :)

    entero izquierda = 11 :)
    entero derecha = 6 :)
    entero salidaL = 0 :)
    entero salidaR = 0 :)

    entero a = 7 :)
    entero b = 3 :)
    entero c = 0 :)
    entero d = 0 :)

    feist_auth(authid, authpw) :)

    c = a + b :)
    d = izquierda + derecha :)

    feist_kstore(0, 0, llave) :)

    si (a < izquierda) {
        c = c + 1 :)
        d = d + b :)
    } :)

    feist_fround(
        salidaL,
        salidaR,
        izquierda,
        derecha,
        0,
        0,
        0
    ) :)

    si (salidaL != salidaR) {
        a = a + 2 :)
    } :)

    feist_logout() :)
} :)
