# Ensamblador intermedio generado por cdff
# Cada instruccion emitida ocupa un bundle de 16 bytes
principal:
# declaracion base
ALU: ADDI x4, x0, 2
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x13, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# declaracion exp
ALU: ADDI x4, x0, 5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x13, x0, 4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# declaracion pot
ALU: ADDI x13, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: LOAD x4, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x13, x0, 4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: LOAD x5, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# potencia por software: multiplicacion repetida
ALU: ADDI x6, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x5, x0, pow_exponente_valido_0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# exponente negativo no soportado: resultado=0
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP pow_fin_2
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
pow_exponente_valido_0:
pow_bucle_1:
BRU: BEQ x5, x0, pow_fin_2
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x7, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x8, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# multiplicacion por software: shift-and-add
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x11, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x7, x0, mul_izq_positivo_3
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x7, x0, x7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x10, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_izq_positivo_3:
BRU: BGE x8, x0, mul_der_positivo_4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x8, x0, x8
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x10, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_der_positivo_4:
mul_bucle_5:
BRU: BEQ x8, x0, mul_fin_bucle_7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: AND x12, x8, x11
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BEQ x12, x0, mul_sin_suma_6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x9, x9, x7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_sin_suma_6:
ALU: SLLI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SRLI x8, x8, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP mul_bucle_5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_fin_bucle_7:
BRU: BNE x10, x11, mul_fin_signo_8
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x9, x0, x9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_fin_signo_8:
ALU: ADD x6, x0, x9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x5, x5, -1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP pow_bucle_1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
pow_fin_2:
ALU: ADDI x13, x0, 8
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x6, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# declaracion cero
ALU: ADDI x13, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: LOAD x4, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x5, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# potencia por software: multiplicacion repetida
ALU: ADDI x6, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x5, x0, pow_exponente_valido_9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# exponente negativo no soportado: resultado=0
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP pow_fin_11
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
pow_exponente_valido_9:
pow_bucle_10:
BRU: BEQ x5, x0, pow_fin_11
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x7, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x8, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# multiplicacion por software: shift-and-add
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x11, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x7, x0, mul_izq_positivo_12
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x7, x0, x7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x10, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_izq_positivo_12:
BRU: BGE x8, x0, mul_der_positivo_13
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x8, x0, x8
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x10, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_der_positivo_13:
mul_bucle_14:
BRU: BEQ x8, x0, mul_fin_bucle_16
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: AND x12, x8, x11
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BEQ x12, x0, mul_sin_suma_15
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x9, x9, x7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_sin_suma_15:
ALU: SLLI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SRLI x8, x8, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP mul_bucle_14
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_fin_bucle_16:
BRU: BNE x10, x11, mul_fin_signo_17
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x9, x0, x9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_fin_signo_17:
ALU: ADD x6, x0, x9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x5, x5, -1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP pow_bucle_10
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
pow_fin_11:
ALU: ADDI x13, x0, 12
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x6, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# declaracion neg
ALU: ADDI x4, x0, 2
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x5, x0, 3
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# potencia por software: multiplicacion repetida
ALU: ADDI x6, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x5, x0, pow_exponente_valido_18
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# exponente negativo no soportado: resultado=0
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP pow_fin_20
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
pow_exponente_valido_18:
pow_bucle_19:
BRU: BEQ x5, x0, pow_fin_20
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x7, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x8, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# multiplicacion por software: shift-and-add
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x11, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x7, x0, mul_izq_positivo_21
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x7, x0, x7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x10, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_izq_positivo_21:
BRU: BGE x8, x0, mul_der_positivo_22
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x8, x0, x8
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x10, x10, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_der_positivo_22:
mul_bucle_23:
BRU: BEQ x8, x0, mul_fin_bucle_25
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: AND x12, x8, x11
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BEQ x12, x0, mul_sin_suma_24
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x9, x9, x7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_sin_suma_24:
ALU: SLLI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SRLI x8, x8, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP mul_bucle_23
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_fin_bucle_25:
BRU: BNE x10, x11, mul_fin_signo_26
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x9, x0, x9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
mul_fin_signo_26:
ALU: ADD x6, x0, x9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x5, x5, -1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP pow_bucle_19
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
pow_fin_20:
ALU: ADDI x13, x0, 16
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x6, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
fin_programa:
BRU: JMP fin_programa
