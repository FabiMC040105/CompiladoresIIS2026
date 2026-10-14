ALU: ADDI x14, x0, 0
ALU: NOP
ALU: NOP
ALU: SLLI x14, x14, 8
ALU: NOP
ALU: NOP
ALU: ADDI x14, x14, 1
ALU: NOP
ALU: NOP
ALU: SLLI x14, x14, 8
ALU: NOP
ALU: NOP
ALU: ADDI x14, x14, 0
ALU: NOP
ALU: NOP
ALU: SLLI x14, x14, 8
ALU: NOP
ALU: NOP
ALU: ADDI x14, x14, 0
ALU: NOP
ALU: NOP
ALU: ADD x15, x14, x0
BRU: JMP 2160
func_buscar:
ALU: ADDI x2, x0, 0
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x15, 0(x14)
ALU: ADD x15, x14, x0
ALU: ADDI x14, x14, -8
ALU: ADDI x4, x0, 0
ALU: NOP
ALU: NOP
LSU: STORE x4, -4(x15)
ALU: ADDI x4, x0, 0
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15)
inicio_while_0:
ALU: ADDI x5, x0, 2 | LSU: LOAD x4, -4(x15)
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x4, x5, 1600
ALU: ADDI x4, x0, 0
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15)
inicio_while_2:
ALU: ADDI x5, x0, 3 | LSU: LOAD x4, -8(x15)
ALU: NOP
ALU: NOP
ALU: NOP
BRU: BGE x4, x5, 1312
LSU: LOAD x4, -4(x15)
ALU: ADDI x6, x0, 3 | LSU: LOAD x5, -8(x15)
ALU: NOP
ALU: ADDI x7, x0, 0
ALU: ADDI x8, x0, 0
ALU: ADDI x9, x0, 1 | BRU: BGE x4, x0, 64
ALU: NOP
ALU: SUB x4, x0, x4
ALU: ADDI x8, x8, 1
mul_izq_positivo_5:
BRU: BGE x6, x0, 48
ALU: SUB x6, x0, x6
ALU: ADDI x8, x8, 1
mul_der_positivo_6:
mul_bucle_7:
ALU: NOP
BRU: BEQ x6, x0, 144
ALU: AND x10, x6, x9
ALU: NOP
ALU: NOP
BRU: BEQ x10, x0, 32
ALU: ADD x7, x7, x4
mul_sin_suma_8:
ALU: SLLI x4, x4, 1
ALU: SRLI x6, x6, 1
BRU: JMP -144
mul_fin_bucle_9:
BRU: BNE x8, x9, 32
ALU: SUB x7, x0, x7
mul_fin_signo_10:
ALU: NOP
ALU: NOP
ALU: ADD x7, x7, x5
ALU: NOP
ALU: NOP
ALU: SLLI x7, x7, 2
LSU: LOAD x13, 8(x15)
ALU: NOP
ALU: NOP
ALU: ADD x13, x13, x7
ALU: NOP
ALU: NOP
LSU: LOAD x4, 0(x13)
LSU: LOAD x5, 12(x15)
ALU: NOP
ALU: NOP
BRU: BNE x4, x5, 512
ALU: ADDI x5, x0, 3 | LSU: LOAD x4, -4(x15)
ALU: NOP
ALU: ADDI x6, x0, 0
ALU: ADDI x7, x0, 0
ALU: ADDI x8, x0, 1 | BRU: BGE x4, x0, 64
ALU: NOP
ALU: SUB x4, x0, x4
ALU: ADDI x7, x7, 1
mul_izq_positivo_11:
BRU: BGE x5, x0, 48
ALU: SUB x5, x0, x5
ALU: ADDI x7, x7, 1
mul_der_positivo_12:
mul_bucle_13:
ALU: NOP
BRU: BEQ x5, x0, 144
ALU: AND x9, x5, x8
ALU: NOP
ALU: NOP
BRU: BEQ x9, x0, 32
ALU: ADD x6, x6, x4
mul_sin_suma_14:
ALU: SLLI x4, x4, 1
ALU: SRLI x5, x5, 1
BRU: JMP -144
mul_fin_bucle_15:
BRU: BNE x7, x8, 32
ALU: SUB x6, x0, x6
mul_fin_signo_16:
LSU: LOAD x4, -8(x15)
ALU: NOP
ALU: NOP
ALU: ADD x6, x6, x4
ALU: NOP
ALU: NOP
ALU: ADD x2, x0, x6
BRU: JMP 432
fin_if_4:
ALU: ADDI x5, x0, 1 | LSU: LOAD x4, -8(x15)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x4, x4, x5
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15) | BRU: JMP -1344
ALU: NOP
fin_while_3:
ALU: ADDI x5, x0, 1 | LSU: LOAD x4, -4(x15)
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADD x4, x4, x5
ALU: NOP
ALU: NOP
LSU: STORE x4, -4(x15) | BRU: JMP -1632
ALU: NOP
fin_while_1:
ALU: ADDI x4, x0, 1
ALU: NOP
ALU: NOP
ALU: SUB x4, x0, x4
ALU: NOP
ALU: NOP
ALU: ADD x2, x0, x4
BRU: JMP 16
ret_buscar:
ALU: ADD x14, x15, x0
LSU: LOAD x3, 0(x15)
LSU: LOAD x13, 4(x15)
ALU: ADDI x14, x15, 8
ALU: ADD x15, x3, x0
ALU: ADD x1, x13, x0
ALU: NOP
principal:
ALU: ADD x15, x14, x0
ALU: ADDI x13, x14, -40
ALU: ADDI x3, x0, 0
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 128
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
ALU: NOP
ALU: NOP
BRU: BGE x13, x3, 32
BRU: JMP 3600
pila_ok_17:
ALU: ADDI x14, x14, -40
ALU: ADDI x4, x0, 4
ALU: ADDI x13, x15, -24
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: ADDI x4, x0, 8
ALU: ADDI x13, x15, -24
ALU: NOP
ALU: NOP
ALU: ADDI x13, x13, 4
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: ADDI x4, x0, 15
ALU: ADDI x13, x15, -24
ALU: NOP
ALU: NOP
ALU: ADDI x13, x13, 8
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: ADDI x4, x0, 16
ALU: ADDI x13, x15, -24
ALU: NOP
ALU: NOP
ALU: ADDI x13, x13, 12
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: ADDI x4, x0, 23
ALU: ADDI x13, x15, -24
ALU: NOP
ALU: NOP
ALU: ADDI x13, x13, 16
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: ADDI x4, x0, 42
ALU: ADDI x13, x15, -24
ALU: NOP
ALU: NOP
ALU: ADDI x13, x13, 20
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x13)
ALU: ADDI x4, x0, 23
ALU: NOP
ALU: NOP
LSU: STORE x4, -28(x15)
ALU: ADDI x13, x14, -60
ALU: ADDI x3, x0, 0
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 128
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
ALU: NOP
ALU: NOP
BRU: BGE x13, x3, 32
BRU: JMP 2416
pila_ok_18:
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x5, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x6, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x7, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x8, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x9, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x10, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x11, 0(x14)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x12, 0(x14)
LSU: LOAD x4, -28(x15)
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x14)
ALU: ADDI x13, x15, -24
ALU: ADDI x14, x14, -4
ALU: NOP
ALU: NOP
LSU: STORE x13, 0(x14)
ALU: ADDI x14, x14, -4
ALU: ADDI x3, x1, 80
ALU: NOP
ALU: NOP
LSU: STORE x3, 0(x14) | BRU: JMP -4528
ALU: NOP
ret_call_19:
ALU: ADDI x14, x14, 8
ALU: NOP
ALU: NOP
LSU: LOAD x12, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x11, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x10, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x9, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x8, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x7, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x6, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x5, 0(x14)
ALU: ADDI x14, x14, 4
ALU: NOP
ALU: NOP
LSU: LOAD x4, 0(x14)
ALU: ADDI x14, x14, 4
ALU: ADD x4, x0, x2
ALU: NOP
ALU: NOP
LSU: STORE x4, -32(x15)
ALU: ADDI x5, x0, 3 | LSU: LOAD x4, -32(x15)
ALU: NOP
ALU: ADDI x6, x0, 0
ALU: ADDI x7, x0, 0
ALU: ADDI x8, x0, 1
ALU: ADDI x9, x0, 0 | BRU: BGE x4, x0, 80
ALU: NOP
ALU: SUB x4, x0, x4
ALU: ADDI x7, x7, 1
ALU: ADDI x9, x0, 1
div_izq_positivo_20:
BRU: BGE x5, x0, 48
ALU: SUB x5, x0, x5
ALU: ADDI x7, x7, 1
div_der_positivo_21:
ALU: NOP
BRU: BNE x5, x0, 32
BRU: JMP 96
div_divisor_valido_22:
div_bucle_23:
BRU: BGE x4, x5, 32
BRU: JMP 64
div_restar_24:
ALU: SUB x4, x4, x5
ALU: ADDI x6, x6, 1
BRU: JMP -64
div_fin_bucle_25:
BRU: BNE x7, x8, 32
ALU: SUB x6, x0, x6
div_fin_signo_cociente_26:
BRU: BNE x9, x8, 32
ALU: SUB x4, x0, x4
div_fin_signo_resto_27:
LSU: STORE x6, -36(x15)
ALU: ADDI x5, x0, 3 | LSU: LOAD x4, -32(x15)
ALU: NOP
ALU: ADDI x6, x0, 0
ALU: ADDI x7, x0, 0
ALU: ADDI x8, x0, 1
ALU: ADDI x9, x0, 0 | BRU: BGE x4, x0, 80
ALU: NOP
ALU: SUB x4, x0, x4
ALU: ADDI x7, x7, 1
ALU: ADDI x9, x0, 1
div_izq_positivo_28:
BRU: BGE x5, x0, 48
ALU: SUB x5, x0, x5
ALU: ADDI x7, x7, 1
div_der_positivo_29:
ALU: NOP
BRU: BNE x5, x0, 32
BRU: JMP 96
div_divisor_valido_30:
div_bucle_31:
BRU: BGE x4, x5, 32
BRU: JMP 64
div_restar_32:
ALU: SUB x4, x4, x5
ALU: ADDI x6, x6, 1
BRU: JMP -64
div_fin_bucle_33:
BRU: BNE x7, x8, 32
ALU: SUB x6, x0, x6
div_fin_signo_cociente_34:
BRU: BNE x9, x8, 32
ALU: SUB x4, x0, x4
div_fin_signo_resto_35:
ALU: NOP
ALU: NOP
LSU: STORE x4, -40(x15)
ALU: ADD x14, x15, x0
BRU: JMP 16
fin_programa:
BRU: JMP 0
error_pila:
BRU: JMP 0
