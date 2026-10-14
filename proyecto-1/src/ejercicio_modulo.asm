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
BRU: JMP 16
principal:
ALU: ADD x15, x14, x0
ALU: ADDI x13, x14, -12
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
BRU: BGE x13, x3, 32
BRU: JMP 656
pila_ok_0:
ALU: ADDI x14, x14, -12
# declaracion a
ALU: ADDI x4, x0, 17
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -4(x15)
# declaracion b
ALU: ADDI x4, x0, 5
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -8(x15)
# declaracion resultado
LSU: LOAD x4, -4(x15)
LSU: LOAD x5, -8(x15)
# modulo por software: resta repetida
ALU: ADDI x6, x0, 0
ALU: ADDI x7, x0, 0
ALU: ADDI x8, x0, 1
ALU: ADDI x9, x0, 0
BRU: BGE x4, x0, 64
ALU: SUB x4, x0, x4
ALU: ADDI x7, x7, 1
ALU: ADDI x9, x0, 1
div_izq_positivo_1:
BRU: BGE x5, x0, 48
ALU: SUB x5, x0, x5
ALU: ADDI x7, x7, 1
div_der_positivo_2:
# scheduler RAW: 1 NOP(s)
ALU: NOP
BRU: BNE x5, x0, 32
# division entre cero: cociente=0, resto=dividendo
BRU: JMP 96
div_divisor_valido_3:
div_bucle_4:
BRU: BGE x4, x5, 32
BRU: JMP 64
div_restar_5:
ALU: SUB x4, x4, x5
ALU: ADDI x6, x6, 1
BRU: JMP -64
div_fin_bucle_6:
BRU: BNE x7, x8, 32
ALU: SUB x6, x0, x6
div_fin_signo_cociente_7:
BRU: BNE x9, x8, 32
ALU: SUB x4, x0, x4
div_fin_signo_resto_8:
# scheduler RAW: 2 NOP(s)
ALU: NOP
ALU: NOP
LSU: STORE x4, -12(x15)
ALU: ADD x14, x15, x0
BRU: JMP 16
fin_programa:
BRU: JMP 0
error_pila:
BRU: JMP 0
