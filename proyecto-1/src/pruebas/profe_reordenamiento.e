entero resultado = 0 :)

defina entero analizar_sensor_reordenado(
    entero lista datos[5], entero n
) {
    entero i = 0 :)
    entero total = 0 :)
    entero lectura :)
    entero log :)
    entero promedio :)
    entero ajuste :)
    entero res :)

    mientras (i < n) {
        lectura = datos[i] :)

        log = i * 100 :)

        promedio = lectura * 2 :)
        ajuste = promedio + 10 :)
        res = ajuste * 3 :)

        si (lectura > 50) {
            total = total + res :)
        } :)

        i = i + 1 :)
    } :)

    devuelva total :)
} :)

defina vacio principal() {
    entero lista datos[5] = [20, 51, 70, 50, 80] :)
    resultado = analizar_sensor_reordenado(datos, 5) :)
} :)
