# Ensamblador intermedio generado por cdff
# Cada instruccion emitida ocupa un bundle de 16 bytes
principal:
# declaracion a
ALU: ADDI x4, x0, 17
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
# declaracion b
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
# declaracion q
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
# division por software: resta repetida
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x8, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x4, x0, div_izq_positivo_0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_izq_positivo_0:
BRU: BGE x5, x0, div_der_positivo_1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x5, x0, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_der_positivo_1:
BRU: BNE x5, x0, div_divisor_valido_2
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# division entre cero: cociente=0, resto=dividendo
BRU: JMP div_fin_bucle_5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_divisor_valido_2:
div_bucle_3:
BRU: BGE x4, x5, div_restar_4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_fin_bucle_5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_restar_4:
ALU: SUB x4, x4, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x6, x6, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_bucle_3
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_bucle_5:
BRU: BNE x7, x8, div_fin_signo_cociente_6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x6, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_cociente_6:
BRU: BNE x9, x8, div_fin_signo_resto_7
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_resto_7:
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
# declaracion r
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
# modulo por software: resta repetida
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x8, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x4, x0, div_izq_positivo_8
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_izq_positivo_8:
BRU: BGE x5, x0, div_der_positivo_9
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x5, x0, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_der_positivo_9:
BRU: BNE x5, x0, div_divisor_valido_10
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# division entre cero: cociente=0, resto=dividendo
BRU: JMP div_fin_bucle_13
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_divisor_valido_10:
div_bucle_11:
BRU: BGE x4, x5, div_restar_12
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_fin_bucle_13
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_restar_12:
ALU: SUB x4, x4, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x6, x6, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_bucle_11
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_bucle_13:
BRU: BNE x7, x8, div_fin_signo_cociente_14
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x6, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_cociente_14:
BRU: BNE x9, x8, div_fin_signo_resto_15
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_resto_15:
ALU: ADDI x13, x0, 12
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# declaracion qn
ALU: ADDI x4, x0, 17
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
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
# division por software: resta repetida
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x8, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x4, x0, div_izq_positivo_16
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_izq_positivo_16:
BRU: BGE x5, x0, div_der_positivo_17
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x5, x0, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_der_positivo_17:
BRU: BNE x5, x0, div_divisor_valido_18
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# division entre cero: cociente=0, resto=dividendo
BRU: JMP div_fin_bucle_21
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_divisor_valido_18:
div_bucle_19:
BRU: BGE x4, x5, div_restar_20
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_fin_bucle_21
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_restar_20:
ALU: SUB x4, x4, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x6, x6, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_bucle_19
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_bucle_21:
BRU: BNE x7, x8, div_fin_signo_cociente_22
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x6, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_cociente_22:
BRU: BNE x9, x8, div_fin_signo_resto_23
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_resto_23:
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
# declaracion rn
ALU: ADDI x4, x0, 17
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
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
# modulo por software: resta repetida
ALU: ADDI x6, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x8, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 0
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x4, x0, div_izq_positivo_24
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x9, x0, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_izq_positivo_24:
BRU: BGE x5, x0, div_der_positivo_25
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x5, x0, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x7, x7, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_der_positivo_25:
BRU: BNE x5, x0, div_divisor_valido_26
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
# division entre cero: cociente=0, resto=dividendo
BRU: JMP div_fin_bucle_29
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_divisor_valido_26:
div_bucle_27:
BRU: BGE x4, x5, div_restar_28
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_fin_bucle_29
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_restar_28:
ALU: SUB x4, x4, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x6, x6, 1
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
BRU: JMP div_bucle_27
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_bucle_29:
BRU: BNE x7, x8, div_fin_signo_cociente_30
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x6, x0, x6
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_cociente_30:
BRU: BNE x9, x8, div_fin_signo_resto_31
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
div_fin_signo_resto_31:
ALU: ADDI x13, x0, 20
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
fin_programa:
BRU: JMP fin_programa
