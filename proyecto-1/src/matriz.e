^ Prueba de multiplicacion de dos matrices 10x10.
^ Resultado y banderas de verificacion globales para revisar RAM.

entero matriz a[10][10] = [
    [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    [11, 12, 13, 14, 15, 16, 17, 18, 19, 20],
    [21, 22, 23, 24, 25, 26, 27, 28, 29, 30],
    [31, 32, 33, 34, 35, 36, 37, 38, 39, 40],
    [41, 42, 43, 44, 45, 46, 47, 48, 49, 50],
    [51, 52, 53, 54, 55, 56, 57, 58, 59, 60],
    [61, 62, 63, 64, 65, 66, 67, 68, 69, 70],
    [71, 72, 73, 74, 75, 76, 77, 78, 79, 80],
    [81, 82, 83, 84, 85, 86, 87, 88, 89, 90],
    [91, 92, 93, 94, 95, 96, 97, 98, 99, 100]
] :)

entero matriz b[10][10] = [
    [1, 2, 0, 0, 0, 0, 0, 0, 0, 0],
    [0, 1, 2, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 1, 2, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 1, 2, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 1, 2, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 1, 2, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 1, 2, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 1, 2, 0],
    [0, 0, 0, 0, 0, 0, 0, 0, 1, 2],
    [2, 0, 0, 0, 0, 0, 0, 0, 0, 1]
] :)

entero matriz resultado[10][10] :)

^ Valores esperados de a por b.
entero matriz esperado[10][10] = [
    [21, 4, 7, 10, 13, 16, 19, 22, 25, 28],
    [51, 34, 37, 40, 43, 46, 49, 52, 55, 58],
    [81, 64, 67, 70, 73, 76, 79, 82, 85, 88],
    [111, 94, 97, 100, 103, 106, 109, 112, 115, 118],
    [141, 124, 127, 130, 133, 136, 139, 142, 145, 148],
    [171, 154, 157, 160, 163, 166, 169, 172, 175, 178],
    [201, 184, 187, 190, 193, 196, 199, 202, 205, 208],
    [231, 214, 217, 220, 223, 226, 229, 232, 235, 238],
    [261, 244, 247, 250, 253, 256, 259, 262, 265, 268],
    [291, 274, 277, 280, 283, 286, 289, 292, 295, 298]
] :)

entero errores = 0 :)
entero prueba_correcta = 0 :)

defina entero acumular_producto(entero fila, entero columna, entero k, entero suma) {
    si (k >= 10) {
        devuelva suma :)
    } :)
    devuelva acumular_producto(fila, columna, k + 1,
        suma + a[fila][k] * b[k][columna]) :)
} :)

defina entero calcular_fila(entero fila, entero columna) {
    si (columna >= 10) {
        devuelva 0 :)
    } :)
    resultado[fila][columna] = acumular_producto(fila, columna, 0, 0) :)
    devuelva calcular_fila(fila, columna + 1) :)
} :)

defina entero calcular_matriz(entero fila) {
    si (fila >= 10) {
        devuelva 0 :)
    } :)
    calcular_fila(fila, 0) :)
    devuelva calcular_matriz(fila + 1) :)
} :)

defina entero revisar_fila(entero fila, entero columna, entero fallos) {
    si (columna >= 10) {
        devuelva fallos :)
    } :)
    entero distinto = 0 :)
    si (resultado[fila][columna] != esperado[fila][columna]) {
        distinto = 1 :)
    } :)
    devuelva revisar_fila(fila, columna + 1, fallos + distinto) :)
} :)

defina entero revisar_matriz(entero fila, entero fallos) {
    si (fila >= 10) {
        devuelva fallos :)
    } :)
    entero acumulado = revisar_fila(fila, 0, fallos) :)
    devuelva revisar_matriz(fila + 1, acumulado) :)
} :)

defina vacio principal() {
    calcular_matriz(0) :)
    errores = revisar_matriz(0, 0) :)
    si (errores == 0) {
        prueba_correcta = 1 :)
    } :)
} :)
