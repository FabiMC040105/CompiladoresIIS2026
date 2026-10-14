defina vacio principal() {
    entero authid = -1059188172 :)
    entero authpw = -1412606344 :)
    entero llave0 = 287454020 :)
    entero llave1 = 324508639 :)
    entero llave2 = 610839776 :)
    entero llave3 = 253635900 :)
    entero izquierda = 42 :)
    entero derecha = 99 :)
    entero cifradoizq = 0 :)
    entero cifradoder = 0 :)

    feist_auth(authid, authpw) :)
    feist_kstore(0, 0, llave0) :)
    feist_kstore(0, 1, llave1) :)
    feist_kstore(0, 2, llave2) :)
    feist_kstore(0, 3, llave3) :)

    feist_fround(izquierda, derecha, izquierda, derecha, 0, 0, 0) :)
    feist_fround(izquierda, derecha, izquierda, derecha, 0, 1, 0) :)
    feist_fround(izquierda, derecha, izquierda, derecha, 0, 2, 0) :)
    feist_fround(izquierda, derecha, izquierda, derecha, 0, 3, 0) :)

    cifradoizq = izquierda :)
    cifradoder = derecha :)

    feist_fround(izquierda, derecha, izquierda, derecha, 0, 3, 1) :)
    feist_fround(izquierda, derecha, izquierda, derecha, 0, 2, 1) :)
    feist_fround(izquierda, derecha, izquierda, derecha, 0, 1, 1) :)
    feist_fround(izquierda, derecha, izquierda, derecha, 0, 0, 1) :)

    feist_logout() :)
    entero marca = 7 :)
}:)
