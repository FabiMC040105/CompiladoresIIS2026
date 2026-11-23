`timescale 1ns/1ps

`ifndef PROGRAM_FILE
  `define PROGRAM_FILE "programa.bin"
`endif
`ifndef MAX_CICLOS
  `define MAX_CICLOS 200000
`endif
`ifndef EXPECTED_ADDR
  `define EXPECTED_ADDR 32'd0
`endif
`ifndef EXPECTED_VALUE
  `define EXPECTED_VALUE 32'd35
`endif

// Integracion CE1108 -> CE4301 sobre el RTL actual:
// - carga directamente el bundle de 128 bits emitido por cdff
// - ejecuta CPU + pipeline + L1 + L2 + RAM
// - valida el resultado arquitectonico en L2 (write-back), no en una RAM antigua
module tb_integracion_rtl_actual;
    logic clk_i = 1'b0;
    logic clk_mem_i = 1'b0;
    logic rst_i;
    logic stall_i;

    logic global_stall_o;
    logic [31:0] pc_o;
    logic [127:0] bundle_o;
    logic fetch_valid_o;

    logic [63:0] perf_cycles_o;
    logic [63:0] perf_bundles_o;
    logic [63:0] perf_instructions_o;
    logic [63:0] perf_l2_read_hits_o;
    logic [63:0] perf_l2_read_misses_o;
    logic [63:0] perf_l2_write_hits_o;
    logic [63:0] perf_l2_write_misses_o;
    logic [63:0] perf_mem_reads_o;
    logic [63:0] perf_mem_writes_o;
    logic [63:0] perf_mem_busy_cycles_o;
    logic [63:0] perf_mem_words_o;
    logic [63:0] perf_cache_stall_cycles_o;
    logic [63:0] perf_l2_lookup_stall_cycles_o;
    logic [63:0] perf_memory_stall_cycles_o;
    logic [63:0] perf_buffer_stall_cycles_o;

    top #(
        .PROGRAM_FILE(`PROGRAM_FILE),
        .DATA_FILE("")
    ) dut (
        .clk_i(clk_i),
        .clk_mem_i(clk_mem_i),
        .rst_i(rst_i),
        .stall_i(stall_i),
        .global_stall_o(global_stall_o),
        .perf_cycles_o(perf_cycles_o),
        .perf_bundles_o(perf_bundles_o),
        .perf_instructions_o(perf_instructions_o),
        .perf_l2_read_hits_o(perf_l2_read_hits_o),
        .perf_l2_read_misses_o(perf_l2_read_misses_o),
        .perf_l2_write_hits_o(perf_l2_write_hits_o),
        .perf_l2_write_misses_o(perf_l2_write_misses_o),
        .perf_mem_reads_o(perf_mem_reads_o),
        .perf_mem_writes_o(perf_mem_writes_o),
        .perf_mem_busy_cycles_o(perf_mem_busy_cycles_o),
        .perf_mem_words_o(perf_mem_words_o),
        .perf_cache_stall_cycles_o(perf_cache_stall_cycles_o),
        .perf_l2_lookup_stall_cycles_o(perf_l2_lookup_stall_cycles_o),
        .perf_memory_stall_cycles_o(perf_memory_stall_cycles_o),
        .perf_buffer_stall_cycles_o(perf_buffer_stall_cycles_o),
        .pc_o(pc_o),
        .bundle_o(bundle_o),
        .fetch_valid_o(fetch_valid_o)
    );

    always #5  clk_i = ~clk_i;       // 100 MHz
    always #20 clk_mem_i = ~clk_mem_i; // 25 MHz

    task automatic leer_l2(
        input logic [31:0] addr,
        output logic encontrado,
        output logic [31:0] valor
    );
        integer set_idx;
        integer word_idx;
        integer way;
        logic [19:0] tag;
        begin
            set_idx = addr[11:5];
            word_idx = addr[4:2];
            tag = addr[31:12];
            encontrado = 1'b0;
            valor = 32'b0;
            for (way = 0; way < 4; way = way + 1) begin
                if (dut.l2_data_system_inst.controller.cache_inst.valid_array[set_idx][way] &&
                    dut.l2_data_system_inst.controller.cache_inst.tag_array[set_idx][way] == tag) begin
                    encontrado = 1'b1;
                    valor = dut.l2_data_system_inst.controller.cache_inst.data_array[set_idx][way]
                              [word_idx*32 +: 32];
                end
            end
        end
    endtask

    integer ciclos;
    integer estable;
    logic encontrado;
    logic [31:0] valor;

    initial begin
        $dumpfile("evidencia-vliw/rtl-integracion-final/integracion_rtl.vcd");
        $dumpvars(0, tb_integracion_rtl_actual);

        rst_i = 1'b1;
        stall_i = 1'b0;
        ciclos = 0;
        estable = 0;

        repeat (4) @(posedge clk_i);
        rst_i = 1'b0;

        while (ciclos < `MAX_CICLOS && estable < 5) begin
            @(posedge clk_i);
            #1;
            ciclos = ciclos + 1;
            leer_l2(`EXPECTED_ADDR, encontrado, valor);
            if (encontrado && valor === `EXPECTED_VALUE)
                estable = estable + 1;
            else
                estable = 0;
        end

        leer_l2(`EXPECTED_ADDR, encontrado, valor);

        $display("");
        $display("=== INTEGRACION COMPILADOR -> RTL ACTUAL ===");
        $display("PC observado: 0x%08h", pc_o);
        $display("Resultado L2: encontrado=%0d addr=%0d valor=%0d esperado=%0d",
                 encontrado, `EXPECTED_ADDR, valor, `EXPECTED_VALUE);
        $display("Ciclos HW: %0d", perf_cycles_o);
        $display("Bundles retirados HW: %0d", perf_bundles_o);
        $display("Instrucciones retiradas HW: %0d", perf_instructions_o);
        $display("Stalls cache HW: %0d", perf_cache_stall_cycles_o);
        $display("Stalls lookup L2 HW: %0d", perf_l2_lookup_stall_cycles_o);
        $display("Stalls memoria HW: %0d", perf_memory_stall_cycles_o);
        $display("Stalls buffers HW: %0d", perf_buffer_stall_cycles_o);
        $display("L2 read H/M: %0d/%0d", perf_l2_read_hits_o, perf_l2_read_misses_o);
        $display("L2 write H/M: %0d/%0d", perf_l2_write_hits_o, perf_l2_write_misses_o);
        $display("RAM reads/writes: %0d/%0d", perf_mem_reads_o, perf_mem_writes_o);

        if (!(encontrado && valor === `EXPECTED_VALUE))
            $fatal(1, "FAIL RTL: no se obtuvo el resultado esperado antes de %0d ciclos", `MAX_CICLOS);

        $display("PASS RTL: binario de CE1108 ejecuto correctamente sobre CPU+L1+L2+RAM de CE4301");
        $finish;
    end
endmodule
