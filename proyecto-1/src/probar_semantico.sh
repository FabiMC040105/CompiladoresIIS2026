#!/usr/bin/env bash
# Pruebas de integración del análisis semántico. Ejecutar desde proyecto-1/src:
# bash probar_semantico.sh
set -uo pipefail
cd "$(dirname "$0")" || exit 1

if ! command -v make >/dev/null 2>&1; then
  echo 'Falta make. Ejecuta este script en WSL Ubuntu.' >&2
  exit 1
fi
if ! make > /tmp/semantico_compilacion_$$.log 2>&1; then
  echo 'ERROR: no compila. Salida:'
  cat /tmp/semantico_compilacion_$$.log
  rm -f /tmp/semantico_compilacion_$$.log
  exit 1
fi
rm -f /tmp/semantico_compilacion_$$.log
if [[ ! -x ./cdff ]]; then
  echo 'No existe ./cdff despues de make.' >&2
  exit 1
fi
TEMP_DIR="$(mktemp -d)" || exit 1
trap 'rm -rf "$TEMP_DIR"' EXIT
PASAN=0
FALLAN=0

probar() {
  local nombre="$1" expectativa="$2" patron="$3" programa="$4"
  local salida="$TEMP_DIR/salida.txt" estado=0
  printf '%s\n' "$programa" > "$TEMP_DIR/programa.e"
  ./cdff -v "$TEMP_DIR/programa.e" >"$salida" 2>&1 || estado=$?
  local correcto=0
  if [[ "$expectativa" == "VALIDO" ]]; then
    if [[ $estado -eq 0 ]] && grep -Fq 'Programa sintacticamente correcto' "$salida" &&
       ! grep -q 'Error semantico\|Error sintactico\|Error interno' "$salida"; then correcto=1; fi
  else
    if [[ $estado -ne 0 ]] && grep -Fq "$patron" "$salida" &&
       grep -Fq 'Se encontraron' "$salida"; then correcto=1; fi
  fi
  if [[ $correcto -eq 1 ]]; then
    printf 'OK    %s\n' "$nombre"
    ((PASAN+=1))
  else
    printf 'FALLO %s (exit=%s; esperado=%s; patron=%s)\n' "$nombre" "$estado" "$expectativa" "$patron"
    grep -E 'Error|error|Programa sintacticamente|Se encontraron' "$salida" | tail -n 9 || true
    ((FALLAN+=1))
  fi
}

# Variables y declaraciones
probar '01 declaracion valida' VALIDO '' $'defina vacio principal() {\n entero x = 10 :)\n x = 5 :)\n}:)'
probar '02 variable inexistente' ERROR 'no declarada' $'defina vacio principal() {\n entero x = 10 :)\n y = x :)\n}:)'
probar '03 uso antes de declarar (misma linea)' ERROR 'antes de declararse' 'defina vacio principal() { x = 5 :) entero x = 10 :) }:)'
probar '04 declaracion previa (misma linea)' VALIDO '' 'defina vacio principal() { entero x = 10 :) x = 5 :) }:)'
probar '05 tipo erroneo asignacion' ERROR 'tipo incompatible' $'defina vacio principal() {\n entero x = 10 :)\n x = verdadero :)\n}:)'
probar '06 tipo erroneo declaracion' ERROR 'tipo incompatible' $'defina vacio principal() {\n entero x = verdadero :)\n}:)'
probar '07 variable booleana en suma' ERROR 'operandos incompatibles' $'defina vacio principal() {\n vof activo = verdadero :)\n entero x = activo + 5 :)\n}:)'
probar '08 suma anidada incorrecta' ERROR 'operandos incompatibles' $'defina vacio principal() {\n vof activo = verdadero :)\n entero x = (2 + 3) + activo :)\n}:)'
probar '09 comparacion no asignable a entero' ERROR 'tipo incompatible' $'defina vacio principal() {\n entero edad = 21 :)\n entero mayor = edad >= 18 :)\n}:)'
probar '10 comparacion asignable a vof' VALIDO '' $'defina vacio principal() {\n entero edad = 21 :)\n vof mayor = edad >= 18 :)\n}:)'
probar '11 AND con entero' ERROR 'operandos incompatibles' $'defina vacio principal() {\n vof x = verdadero $$ 5 :)\n}:)'
probar '12 AND con vof' VALIDO '' $'defina vacio principal() {\n vof x = verdadero $$ falso :)\n}:)'

# Ambitos, bucles, condiciones
probar '13 variable global visible' VALIDO '' $'entero total = 2 :)\ndefina vacio principal() {\n entero x = total :)\n}:)'
probar '14 variable local no visible fuera de si' ERROR 'no declarada' $'defina vacio principal() {\n entero x = 10 :)\n si (x > 0) {\n entero y = 5 :)\n x = y :)\n } :)\n x = y :)\n}:)'
probar '15 variable local visible dentro de si' VALIDO '' $'defina vacio principal() {\n entero x = 10 :)\n si (x > 0) {\n entero y = 5 :)\n x = y :)\n } :)\n}:)'
probar '16 rompa fuera de ciclo' ERROR 'rompa fuera de un ciclo' $'defina vacio principal() {\n rompa :)\n}:)'
probar '17 rompa dentro de mientras' VALIDO '' $'defina vacio principal() {\n entero i = 0 :)\n mientras (i < 2) {\n rompa :)\n } :)\n}:)'
probar '18 para normal' VALIDO '' $'defina vacio principal() {\n para(entero i = 0, i < 2, i = i + 1) {\n entero x = i :)\n } :)\n}:)'
probar '19 condicion si con entero' ERROR 'condicion debe ser vof' $'defina vacio principal() {\n si (5) {\n entero x = 1 :)\n } :)\n}:)'

# Funciones, firma y retornos
FUN=$'defina entero sumar(entero a, entero b) {\n devuelva a + b :)\n}:)\n'
probar '20 cantidad de argumentos incorrecta' ERROR 'espera 2 argumentos' "${FUN}"$'defina vacio principal() {\n entero x = sumar(5) :)\n}:)'
probar '21 cantidad correcta' VALIDO '' "${FUN}"$'defina vacio principal() {\n entero x = sumar(5, 3) :)\n}:)'
probar '22 tipo de argumento incorrecto' ERROR 'argumento 1' "${FUN}"$'defina vacio principal() {\n entero x = sumar(verdadero, 3) :)\n}:)'
probar '23 funcion inexistente' ERROR 'no declarada' $'defina vacio principal() {\n entero x = noExiste(1) :)\n}:)'
probar '24 retorno incompatible' ERROR 'tipo incompatible en devuelva' $'defina entero edad() {\n devuelva verdadero :)\n}:)\ndefina vacio principal() {\n}:)'
probar '25 retorno compatible' VALIDO '' $'defina entero edad() {\n devuelva 21 :)\n}:)\ndefina vacio principal() {\n entero x = edad() :)\n}:)'
probar '26 retorno en funcion vacio' ERROR 'es vacio y no puede devolver' $'defina vacio mostrar() {\n devuelva 5 :)\n}:)\ndefina vacio principal() {\n}:)'
probar '27 funcion sin retorno' ERROR 'no tiene devuelva' $'defina entero edad() {\n entero x = 21 :)\n}:)\ndefina vacio principal() {\n}:)'

# Listas, matrices e indices
probar '28 lista valida' VALIDO '' $'entero lista numeros[3] = [1,2,3] :)\ndefina vacio principal() {\n entero x = numeros[1] :)\n}:)'
probar '29 longitud incorrecta de lista' ERROR 'longitud incorrecta' $'entero lista numeros[3] = [1,2] :)\ndefina vacio principal() {\n}:)'
probar '30 indice booleano' ERROR 'indice debe ser entero' $'entero lista numeros[3] = [1,2,3] :)\ndefina vacio principal() {\n entero x = numeros[verdadero] :)\n}:)'
probar '31 matriz valida' VALIDO '' $'entero matriz tabla[2][2] = [[1,2],[3,4]] :)\ndefina vacio principal() {\n entero x = tabla[1][0] :)\n}:)'
probar '32 matriz columnas incorrectas' ERROR 'cantidad de columnas incorrecta' $'entero matriz tabla[2][2] = [[1],[2,3]] :)\ndefina vacio principal() {\n}:)'
probar '33 param lista con escalar' ERROR 'argumento 1' $'defina vacio usar(entero lista datos[3]) {\n}:)\ndefina vacio principal() {\n entero x = 2 :)\n usar(x) :)\n}:)'
probar '34 param lista con lista' VALIDO '' $'defina vacio usar(entero lista datos[3]) {\n}:)\ndefina vacio principal() {\n entero lista x[3] = [1,2,3] :)\n usar(x) :)\n}:)'

printf '\nResultado: %d aprobadas, %d fallidas.\n' "$PASAN" "$FALLAN"
if ((FALLAN > 0)); then exit 1; fi
