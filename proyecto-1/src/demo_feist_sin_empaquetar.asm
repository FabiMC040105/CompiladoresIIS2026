# Demo de las instrucciones FEIST en codigo maquina VLIW.
# Separado a proposito: el empaquetador FEIST debe crear el bundle de 4 slots.
ALU: NOP
FEIST: AUTH x2, x3
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
FEIST: KSTORE 0, 0, x4
ALU: NOP
ALU: NOP
ALU: NOP
ALU: NOP
FEIST: FROUND x13, x14, x5, x6, 0, 0, 0
ALU: ADDI x11, x0, 9 | LSU: LOAD x12, 0(x0) | BRU: BEQ x0, x2, 16
ALU: NOP
ALU: NOP
ALU: NOP
