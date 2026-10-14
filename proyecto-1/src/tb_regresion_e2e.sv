`timescale 1ns/1ps

// Validacion externa: NO modifica ningun modulo de Arquitectura.
// Se ejecuta sobre los .mem emitidos por el compilador de CE1108.
module tb_regresion_e2e;
    logic clk_i = 0;
    logic rst_i = 1;
    logic stall_i = 0;
    wire [31:0] pc_o;

    integer end_pc;
    integer result_addr;
    integer expected;
    integer cycles;
    integer end_visits;
    integer parallel_cycles;
    integer alu_bru_cycles;
    integer lsu_bru_cycles;
    integer feist_mixto_cycles;
    integer fd;
    integer i;
    integer require_parallel;
    logic [31:0] result_value;
    string snapshot_prefix;
    integer trace_fd;
    integer fallos;
    integer pos;
    logic [31:0] trace_pc [0:127];
    logic [31:0] trace_ex_pc [0:127];
    logic [31:0] trace_x14 [0:127];
    logic [31:0] trace_x15 [0:127];
    logic trace_ex_valid [0:127];
    logic trace_alu_valid [0:127];
    logic [3:0] trace_alu_rd [0:127];
    logic [31:0] trace_alu_result [0:127];
    logic trace_bru_valid [0:127];
    logic trace_bru_taken [0:127];
    logic [31:0] trace_wb_pc [0:127];
    logic trace_wb_write [0:127];
    logic [3:0] trace_wb_rd [0:127];
    logic [31:0] trace_wb_result [0:127];

    task automatic guardar_muestra;
        integer k;
        begin
            k = cycles % 128;
            trace_pc[k] = pc_o;
            trace_ex_pc[k] = dut.ex_pc;
            trace_x14[k] = dut.regfile_inst.regs[14];
            trace_x15[k] = dut.regfile_inst.regs[15];
            trace_ex_valid[k] = dut.rr_ex_valid_out;
            trace_alu_valid[k] = dut.ex_alu_valid;
            trace_alu_rd[k] = dut.ex_alu_rd;
            trace_alu_result[k] = dut.ex_alu_result;
            trace_bru_valid[k] = dut.ex_bru_valid;
            trace_bru_taken[k] = dut.ex_bru_branch_taken;
            trace_wb_pc[k] = dut.wb_pc;
            trace_wb_write[k] = dut.wb_alu_write_enable;
            trace_wb_rd[k] = dut.wb_alu_rd;
            trace_wb_result[k] = dut.wb_alu_result;
        end
    endtask

    task automatic exportar_traza;
        integer k;
        integer comienzo;
        begin
            trace_fd = $fopen({snapshot_prefix, ".trace.txt"}, "w");
            if (trace_fd != 0) begin
                $fdisplay(trace_fd, "ciclo PC_FETCH PC_EX SP FP EXV ALUV ALURD ALURES BRUV BRUT WBPC WB_WR WB_RD WB_RESULT");
                comienzo = (cycles > 128) ? cycles - 127 : 1;
                for (k = comienzo; k <= cycles; k = k + 1) begin
                    pos = k % 128;
                    $fdisplay(trace_fd,
                      "%0d %08h %08h %08h %08h %b %b %0d %08h %b %b %08h %b %0d %08h",
                      k,trace_pc[pos],trace_ex_pc[pos],trace_x14[pos],trace_x15[pos],
                      trace_ex_valid[pos],trace_alu_valid[pos],trace_alu_rd[pos],
                      trace_alu_result[pos],trace_bru_valid[pos],trace_bru_taken[pos],
                      trace_wb_pc[pos],trace_wb_write[pos],trace_wb_rd[pos],trace_wb_result[pos]);
                end
                $fclose(trace_fd);
            end
        end
    endtask

    always #5 clk_i = ~clk_i;

    top #(
        .IMEM_WORDS(16384),
        .PROGRAM_FILE("programa_prueba.mem"),
        .DATA_FILE("")
    ) dut (
        .clk_i(clk_i),
        .rst_i(rst_i),
        .stall_i(stall_i),
        .pc_o(pc_o)
    );

    initial begin
        if (!$value$plusargs("fin_pc=%d", end_pc) ||
            !$value$plusargs("resultado_dir=%d", result_addr) ||
            !$value$plusargs("esperado=%d", expected) ||
            !$value$plusargs("snapshot=%s", snapshot_prefix)) begin
            $fatal(1, "Faltan +fin_pc, +resultado_dir, +esperado o +snapshot");
        end
        require_parallel = $test$plusargs("exigir_paralelo");
        if (result_addr < 0 || result_addr > 65532)
            $fatal(1, "Direccion de resultado invalida");

        fallos = 0;
        cycles = 0;
        end_visits = 0;
        parallel_cycles = 0;
        alu_bru_cycles = 0;
        lsu_bru_cycles = 0;
        feist_mixto_cycles = 0;

        repeat (4) @(negedge clk_i);
        rst_i = 0;

        while (cycles < 20000 && end_visits == 0) begin
            @(negedge clk_i);
            cycles = cycles + 1;
            guardar_muestra();
            if (dut.rr_ex_valid_out &&
                dut.ex_alu_valid &&
                dut.ex_lsu_valid &&
                dut.ex_lsu_mem_read)
                parallel_cycles = parallel_cycles + 1;
            if (dut.rr_ex_valid_out && dut.ex_alu_valid && dut.ex_bru_valid)
                alu_bru_cycles = alu_bru_cycles + 1;
            if (dut.rr_ex_valid_out && dut.ex_lsu_valid && dut.ex_bru_valid)
                lsu_bru_cycles = lsu_bru_cycles + 1;
            if (dut.rr_ex_valid_out && dut.ex_feist_valid &&
                (dut.ex_alu_valid || dut.ex_lsu_valid || dut.ex_bru_valid))
                feist_mixto_cycles = feist_mixto_cycles + 1;

            // No confundir Fetch especulativo con ejecucion comprometida.
            // La etiqueta fin_programa contiene JMP fin_programa.
            // Considerar final solo cuando su salto es valido en EX.
            if (dut.rr_ex_valid_out === 1'b1 &&
                dut.ex_bru_valid === 1'b1 &&
                dut.ex_bru_opcode === 3'b001 &&
                dut.ex_bru_branch_taken === 1'b1 &&
                dut.ex_pc === end_pc) begin
                end_visits = 1;
                $display("FIN_CONFIRMADO_EX: PC=%0d ciclo=%0d", dut.ex_pc, cycles);
            end
        end

        if (end_visits == 0) begin
            $display("FAIL: TIMEOUT fin_programa, PC=%h", pc_o);
            fallos = fallos + 1;
        end

        // Se observo el salto real de fin_programa en EX (no en Fetch).
        // Drenar etapas pendientes de WB antes de inspeccionar el estado.
        // Durante el drenaje, el JMP final puede seguir repitiendose.
        $display("DEBUG: al detectar fin, x14=%0d x15=%0d PC=%0d",
                 dut.regfile_inst.regs[14], dut.regfile_inst.regs[15], pc_o);
        repeat (12) begin
            @(negedge clk_i);
            cycles = cycles + 1;
            guardar_muestra();
            if (dut.rr_ex_valid_out && dut.ex_alu_valid &&
                dut.ex_lsu_valid && dut.ex_lsu_mem_read)
                parallel_cycles = parallel_cycles + 1;
            if (dut.rr_ex_valid_out && dut.ex_alu_valid && dut.ex_bru_valid)
                alu_bru_cycles = alu_bru_cycles + 1;
            if (dut.rr_ex_valid_out && dut.ex_lsu_valid && dut.ex_bru_valid)
                lsu_bru_cycles = lsu_bru_cycles + 1;
            if (dut.rr_ex_valid_out && dut.ex_feist_valid &&
                (dut.ex_alu_valid || dut.ex_lsu_valid || dut.ex_bru_valid))
                feist_mixto_cycles = feist_mixto_cycles + 1;
        end
        $display("DEBUG: despues de drenar pipeline, x14=%0d x15=%0d PC=%0d",
                 dut.regfile_inst.regs[14], dut.regfile_inst.regs[15], pc_o);

        result_value = {
            dut.data_memory_inst.memory[result_addr+3],
            dut.data_memory_inst.memory[result_addr+2],
            dut.data_memory_inst.memory[result_addr+1],
            dut.data_memory_inst.memory[result_addr]
        };

        $display("RESUMEN: CICLOS=%0d PARALELOS=%0d RESULTADO=%0d", cycles, parallel_cycles, result_value);
        $display("RESUMEN_UNIDADES: ALU_LSU=%0d ALU_BRU=%0d LSU_BRU=%0d FEIST_MIXTO=%0d",
                 parallel_cycles, alu_bru_cycles, lsu_bru_cycles, feist_mixto_cycles);

        if (result_value !== expected) begin
            $display("FAIL: RESULTADO=%0d, esperado=%0d", result_value, expected);
            fallos = fallos + 1;
        end
        if (dut.regfile_inst.regs[0] !== 32'd0) begin
            $display("FAIL: x0 fue modificado");
            fallos = fallos + 1;
        end
        if (dut.regfile_inst.regs[14] !== 32'd65536) begin
            $display("FAIL: SP x14=%0d, esperado=65536", dut.regfile_inst.regs[14]);
            fallos = fallos + 1;
        end
        if (require_parallel && parallel_cycles == 0) begin
            $display("FAIL: no hubo ejecucion ALU+LSU en paralelo");
            fallos = fallos + 1;
        end
        if (!require_parallel && parallel_cycles != 0) begin
            $display("FAIL: version secuencial ejecuto instrucciones paralelas");
            fallos = fallos + 1;
        end

        // Comparaciones exactas de memoria y registros, no solo del resultado.
        $writememh({snapshot_prefix, ".memory.hex"}, dut.data_memory_inst.memory);
        fd = $fopen({snapshot_prefix, ".registers.txt"}, "w");
        if (!fd)
            $fatal(1, "No se pudo crear snapshot de registros");
        for (i = 0; i < 16; i = i + 1)
            $fdisplay(fd, "%08h", dut.regfile_inst.regs[i]);
        $fclose(fd);

        exportar_traza();
        if (fallos != 0) begin
            $display("RESULTADO_TB: FAIL (%0d errores). Revisar %s.trace.txt", fallos, snapshot_prefix);
            $fatal(1, "El programa fallo; snapshots y traza guardados");
        end
        $display("RESULTADO_TB: PASS");
        $finish;
    end
endmodule
