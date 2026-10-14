`timescale 1ns/1ps
module tb_feist_4r;
    logic clk_i = 0;
    logic rst_i = 1;
    logic stall_i = 0;
    wire [31:0] pc_o;
    integer fin_pc;
    integer ciclos = 0;
    integer fin = 0;
    integer auth = 0;
    integer kstores = 0;
    integer frounds = 0;
    integer logouts = 0;
    integer mixtos = 0;
    integer errores = 0;
    integer j;
    integer fd;
    string snapshot;
    reg [31:0] subkeys [0:3];
    reg [31:0] ref_l, ref_r, siguiente_l, siguiente_r;
    reg [31:0] cifra_l, cifra_r;
    reg [31:0] dato_l, dato_r, dato_cl, dato_cr, marca;

    always #5 clk_i = ~clk_i;

    top #(
        .IMEM_WORDS(16384),
        .PROGRAM_FILE("programa_feist_validacion.mem"),
        .DATA_FILE("")
    ) dut (.clk_i(clk_i), .rst_i(rst_i), .stall_i(stall_i), .pc_o(pc_o));

    function automatic [31:0] rol32(input [31:0] x, input integer n);
        rol32 = (x << n) | (x >> (32-n));
    endfunction
    function automatic [31:0] ronda_f(input [31:0] x, input [31:0] k);
        ronda_f = (rol32(x,5) + k) ^ rol32(x,13);
    endfunction
    function automatic [31:0] palabra(input integer dir);
        palabra = {dut.data_memory_inst.memory[dir+3],
                   dut.data_memory_inst.memory[dir+2],
                   dut.data_memory_inst.memory[dir+1],
                   dut.data_memory_inst.memory[dir]};
    endfunction

    initial begin
        if (!$value$plusargs("fin_pc=%d", fin_pc) ||
            !$value$plusargs("snapshot=%s", snapshot))
            $fatal(1, "Faltan +fin_pc=NUM y +snapshot=RUTA");

        subkeys[0] = 32'h11223344;
        subkeys[1] = 32'h13579BDF;
        subkeys[2] = 32'h2468ACE0;
        subkeys[3] = 32'h0F1E2D3C;

        // Oraculo independiente del procesador y del compilador.
        ref_l = 32'd42;
        ref_r = 32'd99;
        for (j=0; j<4; j=j+1) begin
            siguiente_l = ref_r;
            siguiente_r = ref_l ^ ronda_f(ref_r, subkeys[j]);
            ref_l = siguiente_l;
            ref_r = siguiente_r;
        end
        cifra_l = ref_l;
        cifra_r = ref_r;
        if (cifra_l !== 32'h2660080A || cifra_r !== 32'h28CC25AB)
            $fatal(1,"El oraculo FEIST no coincide con el vector de referencia");
        for (j=3; j>=0; j=j-1) begin
            siguiente_l = ref_r ^ ronda_f(ref_l, subkeys[j]);
            siguiente_r = ref_l;
            ref_l = siguiente_l;
            ref_r = siguiente_r;
        end
        if (ref_l !== 32'd42 || ref_r !== 32'd99)
            $fatal(1, "El oraculo FEIST no recupera el bloque original");

        repeat (4) @(negedge clk_i);
        rst_i=0;
        while(ciclos < 50000 && fin == 0) begin
            @(negedge clk_i);
            ciclos = ciclos + 1;
            if (dut.rr_ex_valid_out === 1'b1) begin
                if (dut.ex_feist_valid &&
                    (dut.ex_alu_valid || dut.ex_lsu_valid || dut.ex_bru_valid))
                    mixtos = mixtos + 1;
                if (dut.ex_feist_auth_req) begin
                    if (auth != 0 || kstores != 0 || frounds != 0) errores=errores+1;
                    auth=auth+1;
                end
                if (dut.ex_feist_kstore_req) begin
                    if (auth != 1 || frounds != 0 ||
                        dut.ex_feist_key !== 2'd0 ||
                        dut.ex_feist_round !== kstores) errores=errores+1;
                    kstores=kstores+1;
                end
                if (dut.ex_feist_fround_req) begin
                    if (kstores != 4 || logouts != 0 ||
                        dut.ex_feist_key !== 2'd0) errores=errores+1;
                    if (frounds < 4) begin
                        if (dut.ex_feist_mode !== 1'b0 ||
                            dut.ex_feist_round !== frounds) errores=errores+1;
                    end else if (frounds < 8) begin
                        if (dut.ex_feist_mode !== 1'b1 ||
                            dut.ex_feist_round !== (7-frounds)) errores=errores+1;
                    end else errores=errores+1;
                    frounds=frounds+1;
                end
                if (dut.ex_feist_logout_req) begin
                    if (frounds != 8 || logouts != 0) errores=errores+1;
                    logouts=logouts+1;
                end
                // fin_programa contiene un JMP a si mismo; se confirma en EX.
                if (dut.ex_bru_valid && dut.ex_bru_opcode === 3'b001 &&
                    dut.ex_bru_branch_taken && dut.ex_pc === fin_pc)
                    fin=1;
            end
        end
        if (fin == 0) $fatal(1,"TIMEOUT antes del final en EX");
        repeat(12) @(negedge clk_i);

        // 11 variables de 4 bytes; offsets -28,-32,-36,-40,-44 para
        // izquierda, derecha, cifradoizq, cifradoder, marca respectivamente.
        dato_l = palabra(65508);
        dato_r = palabra(65504);
        dato_cl = palabra(65500);
        dato_cr = palabra(65496);
        marca = palabra(65492);

        $display("FIN_EX=%0d CICLOS=%0d AUTH=%0d KSTORE=%0d FROUND=%0d LOGOUT=%0d FEIST_MIXTO=%0d",
                 fin_pc,ciclos,auth,kstores,frounds,logouts,mixtos);
        $display("CIFRADO=%08h/%08h ESPERADO=%08h/%08h DESCIFRADO=%08h/%08h MARCA=%0d SP=%0d",
                 dato_cl,dato_cr,cifra_l,cifra_r,dato_l,dato_r,marca,dut.regfile_inst.regs[14]);

        if (auth!=1 || kstores!=4 || frounds!=8 || logouts!=1 || errores!=0)
            $fatal(1,"Conteos/orden/indices FEIST incorrectos; errores=%0d",errores);
        if (dato_cl !== cifra_l || dato_cr !== cifra_r ||
            dato_l !== 32'd42 || dato_r !== 32'd99 || marca !== 32'd7)
            $fatal(1,"Datos FEIST incorrectos");
        if (dut.regfile_inst.regs[14] !== 32'd65536)
            $fatal(1,"SP incorrecto");
        if ($test$plusargs("exigir_mixto") && mixtos==0)
            $fatal(1,"No hubo FEIST ejecutado junto con otra unidad");

        // Snapshot completo para comparar con la version no empaquetada.
        fd = $fopen(snapshot,"w");
        if (fd==0) $fatal(1,"No se pudo crear snapshot: %s",snapshot);
        for (j=0;j<16;j=j+1)
            $fdisplay(fd,"R%02d=%08h",j,dut.regfile_inst.regs[j]);
        for (j=0;j<65536;j=j+1)
            $fdisplay(fd,"%02h",dut.data_memory_inst.memory[j]);
        $fclose(fd);
        $display("PASS: 4 rondas cifrado + 4 descifrado; resultado y estado verificados");
        $finish;
    end
endmodule
