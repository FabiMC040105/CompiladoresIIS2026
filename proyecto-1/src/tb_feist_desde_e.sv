`timescale 1ns/1ps
module tb_feist_desde_e;
    logic clk_i = 0, rst_i = 1, stall_i = 0;
    wire [31:0] pc_o;
    integer end_pc, cycles = 0, fin = 0, feist_auth = 0, feist_kstore = 0;
    integer feist_round = 0, feist_logout = 0;
    reg [31:0] l, r, lc, rc;
    reg [31:0] subkey = 32'h11223344;
    reg [31:0] expected_r;
    always #5 clk_i = ~clk_i;

    top #(
        .IMEM_WORDS(16384),
        .PROGRAM_FILE("programa_feist_e.mem"),
        .DATA_FILE("")
    ) dut (.clk_i(clk_i), .rst_i(rst_i), .stall_i(stall_i), .pc_o(pc_o));

    function automatic [31:0] rol(input [31:0] x, input integer n);
        rol = (x << n) | (x >> (32-n));
    endfunction
    function automatic [31:0] f(input [31:0] x, input [31:0] k);
        f = (rol(x,5)+k) ^ rol(x,13);
    endfunction
    initial begin
        if (!$value$plusargs("fin_pc=%d", end_pc))
            $fatal(1,"Debe indicar +fin_pc=NUMERO");
        repeat(4) @(negedge clk_i);
        rst_i=0;
        while(cycles < 25000 && fin==0) begin
            @(negedge clk_i);
            cycles++;
            if (dut.rr_ex_valid_out && dut.ex_feist_valid) begin
                if (dut.ex_feist_auth_req) feist_auth++;
                if (dut.ex_feist_kstore_req) feist_kstore++;
                if (dut.ex_feist_fround_req) feist_round++;
                if (dut.ex_feist_logout_req) feist_logout++;
            end
            if (dut.rr_ex_valid_out && dut.ex_bru_valid &&
                dut.ex_bru_opcode == 3'b001 && dut.ex_bru_branch_taken &&
                dut.ex_pc == end_pc) fin=1;
        end
        if (fin==0) $fatal(1,"TIMEOUT: no alcanzo fin_programa en EX");
        repeat(12) @(negedge clk_i);
        // El marco de principal tiene 7 enteros: -4, -8 ... -28 (FP=65536).
        l = {dut.data_memory_inst.memory[65523],dut.data_memory_inst.memory[65522],dut.data_memory_inst.memory[65521],dut.data_memory_inst.memory[65520]};
        r = {dut.data_memory_inst.memory[65519],dut.data_memory_inst.memory[65518],dut.data_memory_inst.memory[65517],dut.data_memory_inst.memory[65516]};
        lc = {dut.data_memory_inst.memory[65515],dut.data_memory_inst.memory[65514],dut.data_memory_inst.memory[65513],dut.data_memory_inst.memory[65512]};
        rc = {dut.data_memory_inst.memory[65511],dut.data_memory_inst.memory[65510],dut.data_memory_inst.memory[65509],dut.data_memory_inst.memory[65508]};
        expected_r = 32'd42 ^ f(32'd99,subkey);
        $display("FIN_PC=%0d CICLOS=%0d AUTH=%0d KSTORE=%0d FROUND=%0d LOGOUT=%0d",end_pc,cycles,feist_auth,feist_kstore,feist_round,feist_logout);
        $display("L=%0d R=%0d ENC_L=%08h ENC_R=%08h EXPECT_ENC_R=%08h SP=%0d", l,r,lc,rc,expected_r,dut.regfile_inst.regs[14]);
        if (feist_auth!=1 || feist_kstore!=1 || feist_round!=2 || feist_logout!=1)
            $fatal(1,"FAIL: conteos FEIST incorrectos");
        if (l!==32'd42 || r!==32'd99 || lc!==32'd99 || rc!==expected_r)
            $fatal(1,"FAIL: cifrado y descifrado no coinciden con referencia");
        if (dut.regfile_inst.regs[14] !== 32'd65536)
            $fatal(1,"FAIL: pila SP incorrecta");
        $display("PASS: FEIST desde fuente .e ejecutado con cifrado y descifrado reales");
        $finish;
    end
endmodule
