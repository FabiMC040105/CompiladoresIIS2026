`timescale 1ns/1ps
// Prueba independiente del assembler + procesador de Arquitectura.
// NO demuestra FEIST generado desde codigo fuente CE1108.
module tb_cuatro_slots_desde_mem;
    logic clk_i = 0;
    logic rst_i = 1;
    logic stall_i = 0;
    wire [31:0] pc_o;
    integer ciclos = 0;
    integer coincidencias = 0;
    always #5 clk_i = ~clk_i;

    top #(
        .IMEM_WORDS(16384),
        .PROGRAM_FILE("cuatro_slots.mem"),
        .DATA_FILE("")
    ) dut (
        .clk_i(clk_i), .rst_i(rst_i), .stall_i(stall_i), .pc_o(pc_o)
    );

    // Se exige que las 4 unidades sean validas en LA MISMA etapa EX.
    always @(posedge clk_i) begin
        if (!rst_i && dut.rr_ex_valid_out &&
            dut.ex_alu_valid &&
            dut.ex_lsu_valid && dut.ex_lsu_mem_read &&
            dut.ex_bru_valid && dut.ex_bru_opcode == 3'b100 &&
            dut.ex_bru_branch_taken == 1'b0 &&
            dut.ex_feist_valid && dut.ex_feist_result_valid)
            coincidencias <= coincidencias + 1;
    end

    initial begin
        // La misma configuracion de autentificacion y llaves del
        // tb_vliw_cuatro_unidades.sv existente en Arquitectura.
        repeat (2) @(negedge clk_i);
        rst_i = 0;
        dut.regfile_inst.regs[2] = 32'hC0DE1234;
        dut.regfile_inst.regs[3] = 32'hABCD5678;
        dut.regfile_inst.regs[4] = 32'hDEADBEEF;
        dut.regfile_inst.regs[5] = 32'h01234567;
        dut.regfile_inst.regs[6] = 32'h89ABCDEF;

        dut.data_memory_inst.memory[0] = 8'h44;
        dut.data_memory_inst.memory[1] = 8'h33;
        dut.data_memory_inst.memory[2] = 8'h22;
        dut.data_memory_inst.memory[3] = 8'h11;

        repeat (24) @(posedge clk_i);
        #1;
        $display("UNIDADES_SIMULTANEAS=%0d", coincidencias);
        if (coincidencias < 1)
            $fatal(1,"No coincidieron las cuatro unidades en EX");
        if (dut.key_vault_inst.authorized !== 1'b1)
            $fatal(1,"AUTH fallo");
        if (dut.key_vault_inst.subkeys[0][0] !== 32'hDEADBEEF)
            $fatal(1,"KSTORE fallo");
        if (dut.regfile_inst.regs[11] !== 32'd9)
            $fatal(1,"ALU no escribio 9 en x11");
        if (dut.regfile_inst.regs[12] !== 32'h11223344)
            $fatal(1,"LSU no leyo 11223344 en x12");
        if (dut.regfile_inst.regs[13] !== 32'h89ABCDEF ||
            dut.regfile_inst.regs[14] !== 32'h6CB9C8B2)
            $fatal(1,"FROUND no produjo las salidas esperadas");
        $display("PASS: 4 UNIDADES EN PARALELO DESDE .mem CODIFICADO POR ASSEMBLER");
        $finish;
    end
endmodule
