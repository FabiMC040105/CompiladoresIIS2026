# Ensamblador intermedio generado por cdff
# Cada instruccion emitida ocupa un bundle de 16 bytes
principal:
# declaracion a
ALU: ADDI x4, x0, 5
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
ALU: ADDI x4, x0, 3
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
# declaracion c
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
ALU: ADD x4, x4, x5
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
ALU: ADDI x13, x0, 8
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
