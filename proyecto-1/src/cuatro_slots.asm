# El assembler reconoce una linea por bundle; slots no usados son 0.
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
ALU: ADDI x11, x0, 9 | LSU: LOAD x12, 0(x0) | BRU: BEQ x0, x2, 16 | FEIST: FROUND x13, x14, x5, x6, 0, 0, 0
ALU: NOP
ALU: NOP
ALU: NOP
