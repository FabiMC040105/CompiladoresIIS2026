entero resultado = 0 :)

defina entero analizar_senales_multiples(
    entero s1, entero s2, entero s3, entero s4,
    entero s5, entero s6, entero s7, entero s8
) {
    entero a = s1 + s2 :)
    entero b = s3 + s4 :)
    entero c = s5 + s6 :)
    entero d = s7 + s8 :)

    entero e = a * b :)
    entero f = c * d :)

    entero g = s1 - s3 :)
    entero h = s2 - s4 :)
    entero i = s5 - s7 :)
    entero j = s6 - s8 :)

    entero k = g + h :)
    entero l = i + j :)

    entero m = a * s5 :)
    entero n = b * s6 :)
    entero o = c * s7 :)
    entero p = d * s8 :)

    entero q = m + n :)
    entero r = o + p :)

    entero t = s1 * s8 :)
    entero u = s2 * s7 :)
    entero w = s3 * s6 :)
    entero x = s4 * s5 :)

    entero res = e + f + k + l + m + n + o + p
        + q + r + t + u + w + x
        + a + b + c + d + g + h + i + j :)

    devuelva res :)
} :)

defina vacio principal() {
    resultado = analizar_senales_multiples(
        1, 2, 3, 4, 5, 6, 7, 8
    ) :)
} :)
