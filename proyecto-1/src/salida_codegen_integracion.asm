# Ensamblador intermedio generado por cdff
# Cada instruccion emitida ocupa un bundle de 16 bytes
# NOPs calculados por scheduler RAW para IF-ID-RR-EX-WB
# convencion: x2=retorno, x14=SP, x15=FP
# pila: 0x8000..0xFFFF, SP inicial=0x10000
ALU: ADDI x14, x0, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x14, x14, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x14, x14, 1
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x14, x14, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x14, x14, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x14, x14, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x14, x14, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADD x15, x14, x0
BRU: JMP principal
# funcion factorial: x14=SP, x15=FP, retorno en x2
func_factorial:
ALU: ADDI x2, x0, 0
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x15, 0(x14)
ALU: ADD x15, x14, x0
ALU: ADDI x14, x14, -8
# declaracion resultado
ALU: ADDI x4, x0, 1
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -4(x15)
# declaracion i
ALU: ADDI x4, x0, 1
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15)
# condicion si linea 22
LSU: LOAD x4, 8(x15)
ALU: ADDI x5, x0, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
BRU: BNE x4, x5, fin_if_0
ALU: ADDI x4, x0, 1
# devuelva de factorial
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADD x2, x0, x4
BRU: JMP ret_factorial
fin_if_0:
# inicio mientras linea 30
inicio_while_1:
LSU: LOAD x4, -8(x15)
LSU: LOAD x5, 8(x15)
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLT x4, x5, x4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
BRU: BNE x4, x0, fin_while_2
LSU: LOAD x4, -4(x15)
LSU: LOAD x5, -8(x15)
# multiplicacion por software: shift-and-add
ALU: ADDI x6, x0, 0
ALU: ADDI x7, x0, 0
ALU: ADDI x8, x0, 1
BRU: BGE x4, x0, mul_izq_positivo_3
ALU: SUB x4, x0, x4
ALU: ADDI x7, x7, 1
mul_izq_positivo_3:
BRU: BGE x5, x0, mul_der_positivo_4
ALU: SUB x5, x0, x5
ALU: ADDI x7, x7, 1
mul_der_positivo_4:
mul_bucle_5:
# scheduler RAW: 1 NOP(s)
ALU: NOP
BRU: BEQ x5, x0, mul_fin_bucle_7
ALU: AND x9, x5, x8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
BRU: BEQ x9, x0, mul_sin_suma_6
ALU: ADD x6, x6, x4
mul_sin_suma_6:
ALU: SLLI x4, x4, 1
ALU: SRLI x5, x5, 1
BRU: JMP mul_bucle_5
mul_fin_bucle_7:
BRU: BNE x7, x8, mul_fin_signo_8
ALU: SUB x6, x0, x6
mul_fin_signo_8:
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x6, -4(x15)
LSU: LOAD x4, -8(x15)
ALU: ADDI x5, x0, 1
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADD x4, x4, x5
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15)
BRU: JMP inicio_while_1
fin_while_2:
LSU: LOAD x4, -4(x15)
# devuelva de factorial
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADD x2, x0, x4
BRU: JMP ret_factorial
ret_factorial:
ALU: ADD x14, x15, x0
LSU: LOAD x3, 0(x15)
LSU: LOAD x13, 4(x15)
ALU: ADDI x14, x15, 8
ALU: ADD x15, x3, x0
ALU: ADD x1, x13, x0
principal:
ALU: ADD x15, x14, x0
ALU: ADDI x13, x14, -8
ALU: ADDI x3, x0, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 128
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
BRU: BGE x13, x3, pila_ok_9
BRU: JMP error_pila
pila_ok_9:
ALU: ADDI x14, x14, -8
# declaracion n
ALU: ADDI x4, x0, 5
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -4(x15)
# declaracion resultado
# llamada a factorial: caller-save, parametros, retorno
ALU: ADDI x13, x14, -56
ALU: ADDI x3, x0, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 128
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: SLLI x3, x3, 8
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
ALU: ADDI x3, x3, 0
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
BRU: BGE x13, x3, pila_ok_10
BRU: JMP error_pila
pila_ok_10:
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x5, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x6, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x7, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x8, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x9, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x10, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x11, 0(x14)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x12, 0(x14)
LSU: LOAD x4, -4(x15)
ALU: ADDI x14, x14, -4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, 0(x14)
ALU: ADDI x14, x14, -4
ALU: ADDI x3, x1, ret_call_11
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x3, 0(x14)
BRU: JMP func_factorial
ret_call_11:
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x12, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x11, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x10, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x9, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x8, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x7, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x6, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x5, 0(x14)
ALU: ADDI x14, x14, 4
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: LOAD x4, 0(x14)
ALU: ADDI x14, x14, 4
ALU: ADD x4, x0, x2
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15)
ALU: ADD x14, x15, x0
BRU: JMP fin_programa
fin_programa:
BRU: JMP fin_programa
error_pila:
BRU: JMP error_pila
