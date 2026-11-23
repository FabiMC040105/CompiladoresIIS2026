^ Prueba de integracion de optimizaciones sobre IR y codegen_ir.
^ Resultado matematico esperado: resultado = 35.

entero resultado = 0 :)

defina vacio principal() {
    entero n = 5 :)
    entero i = 0 :)
    entero suma = 0 :)
    entero a :)
    entero d :)
    entero b :)
    entero c :)
    entero h :)
    entero basura :)

    ^ Cadenas independientes para reordenamiento.
    a = 2 + 3 :)
    d = a + a :)
    b = n + 4 :)
    c = b - 1 :)

    ^ Segunda escritura de a, con su valor anterior aun necesario.
    ^ Patron WAW/WAR para renombramiento.
    a = a + 4 :)
    h = a - 2 :)

    ^ Calculo muerto; no participa en resultado.
    basura = n + 99 :)

    ^ Ciclo simple, candidato a unrolling factor 2.
    mientras (i < n) {
        suma = suma + i :)
        i = i + 1 :)
    } :)

    ^ suma=10, d=10, c=8, h=7 -> 35.
    resultado = suma + d + c + h :)
} :)
