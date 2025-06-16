`timescale 1ns / 1ps
`include "GLOBAL.v"

module CPU(
        input clk,
        input rst,
        output halt
    );

    reg [31:0] PC;
    reg [31:0] IF_ID_inst;
    reg [31:0] IF_ID_PC;
    reg [31:0] ID_EX_PC;
    reg [31:0] ID_EX_rs_val;
    reg [31:0] ID_EX_rt_val;
    reg [31:0] ID_EX_imm_ext;
    reg [4:0]  ID_EX_shamt;
    reg [4:0]  ID_EX_rs;
    reg [4:0]  ID_EX_rt;
    reg [4:0]  ID_EX_rd;
    reg [5:0]  ID_EX_funct;
    reg        ID_EX_RegDst;
    reg        ID_EX_RegWrite;
    reg        ID_EX_ALUSrcA;
    reg        ID_EX_MemRead;
    reg        ID_EX_MemWrite;
    reg        ID_EX_MemtoReg;
    reg [1:0]  ID_EX_ALUSrcB;
    reg [3:0]  ID_EX_ALUOp;
    reg [4:0]  ID_EX_write_reg;
    reg [31:0] EX_MEM_ALU_out;
    reg [31:0] EX_MEM_rt_val;
    reg [4:0]  EX_MEM_write_reg;
    reg        EX_MEM_RegWrite;
    reg        EX_MEM_MemRead;
    reg        EX_MEM_MemWrite;
    reg        EX_MEM_MemtoReg;
    reg [31:0] MEM_WB_ALU_out;
    reg [31:0] MEM_WB_mem_data;
    reg [4:0]  MEM_WB_RegDst;
    reg        MEM_WB_RegWrite;
    reg        MEM_WB_MemtoReg;

    reg [31:0] IR;

    wire [31:0] inst;
    wire [31:0] next_PC;

    wire [5:0]  opcode = IF_ID_inst[31:26];
    wire [4:0]  rs     = IF_ID_inst[25:21];
    wire [4:0]  rt     = IF_ID_inst[20:16];
    wire [4:0]  rd     = IF_ID_inst[15:11];
    wire [15:0] imm    = IF_ID_inst[15:0];

    wire [31:0] rs_val;
    wire [31:0] rt_val;
    wire        use_rs;
    wire        use_rt;
    wire [31:0] branch_target = IF_ID_PC + {{14{imm[15]}}, imm, 2'b00};
    wire [31:0] imm_ext = SignExtend ? {{16{imm[15]}}, imm} : {16'b0, imm};
    wire [31:0] ALU_in1 = ID_EX_ALUSrcA ? ID_EX_rs_val : ID_EX_PC;
    wire [31:0] ALU_in2 = (ID_EX_ALUSrcB == 2'b00) ? ID_EX_rt_val : 
                          (ID_EX_ALUSrcB == 2'b01) ? 32'd4 : ID_EX_imm_ext;
    wire [31:0] ALU_result;
    wire [4:0]  write_reg = (ID_EX_RegDst == 1'b1) ? ID_EX_rd : ID_EX_rt;
    wire [31:0] mem_read_data;
    wire [31:0] MEM_WB_WriteData = MEM_WB_MemtoReg ? MEM_WB_mem_data : MEM_WB_ALU_out;
    wire stall;

    wire RegDst;
    wire RegWrite;
    wire ALUSrcA;
    wire MemRead;
    wire MemWrite;
    wire MemtoReg;
    wire Branch;
    wire Jump;
    wire JR;
    wire SavePC;
    wire SignExtend;
    wire [1:0] ALUSrcB;
    wire [3:0] ALUOp;

    wire is_branch_taken = Branch && ((opcode == `OP_BEQ && rs_val == rt_val) || (opcode == `OP_BNE && rs_val != rt_val));

    assign next_PC = JR              ? rs_val :
                     Jump            ? {IF_ID_PC[31:28], IF_ID_inst[25:0], 2'b00} :
                     is_branch_taken ? branch_target :
                                       PC + 4;

    MEM mem (
        .clk(clk),
        .rst(rst),
        .inst_addr(PC),
        .inst(inst),
        .mem_addr(EX_MEM_ALU_out),
        .MemWrite(EX_MEM_MemWrite),
        .mem_write_data(EX_MEM_rt_val),
        .mem_read_data(mem_read_data)
    );

    CTRL ctrl (
        .inst(IF_ID_inst),
        .RegDst(RegDst),
        .RegWrite(RegWrite),
        .ALUSrcA(ALUSrcA),
        .ALUSrcB(ALUSrcB),
        .ALUOp(ALUOp),
        .MemRead(MemRead),
        .MemWrite(MemWrite),
        .MemtoReg(MemtoReg),
        .Branch(Branch),
        .Jump(Jump),
        .JR(JR),
        .SavePC(SavePC),
        .SignExtend(SignExtend),
        .use_rs(use_rs),
        .use_rt(use_rt)
    );

    RF rf (
        .clk(clk),
        .rst(rst),
        .rd_addr1(rs),
        .rd_addr2(rt),
        .rd_data1(rs_val),
        .rd_data2(rt_val),
        .RegWrite(MEM_WB_RegWrite),
        .wr_addr(MEM_WB_RegDst),
        .wr_data(MEM_WB_WriteData)
    );

    HAZARD hazard_unit(
        .ID_rs(rs),
        .ID_rt(rt),
        .use_rs(use_rs),
        .use_rt(use_rt),
        .EX_RegWrite(ID_EX_RegWrite),
        .EX_rd(ID_EX_write_reg),
        .MEM_RegWrite(EX_MEM_RegWrite),
        .MEM_rd(EX_MEM_write_reg),
        .WB_RegWrite(MEM_WB_RegWrite),
        .WB_rd(MEM_WB_RegDst),
        .stall(stall)
    );

    ALU alu (
        .operand1(ALU_in1),
        .operand2(ALU_in2),
        .shamt(ID_EX_shamt),
        .funct(ID_EX_funct),
        .alu_result(ALU_result)
    );

    always @(posedge clk or posedge rst) begin
        if (rst) PC <= 32'b0;
        else if (!stall) PC <= next_PC;
    end

    always @(posedge clk) begin
        if (rst) IR <= 32'b0;
        else if(!stall) IR <= inst;
    end

    always @(posedge clk) begin
        if (rst || is_branch_taken) begin
            IF_ID_inst <= 32'b0;
            IF_ID_PC   <= 32'b0;
        end
        else if (!stall) begin
            IF_ID_inst <= IR;
            IF_ID_PC   <= PC;
        end
    end

    always @(posedge clk) begin
        if (rst || stall) begin
            ID_EX_PC        <= 32'h00000000;
            ID_EX_rs_val    <= 32'h00000000;
            ID_EX_rt_val    <= 32'h00000000;
            ID_EX_imm_ext   <= 32'h00000000;
            ID_EX_shamt     <= 5'b00000;
            ID_EX_rs        <= 5'b00000;
            ID_EX_rt        <= 5'b00000;
            ID_EX_rd        <= 5'b00000;
            ID_EX_funct     <= 6'b000000;
            ID_EX_RegDst    <= 1'b0;
            ID_EX_RegWrite  <= 1'b0;
            ID_EX_ALUSrcA   <= 1'b0;
            ID_EX_ALUSrcB   <= 2'b00;
            ID_EX_ALUOp     <= 4'b0000;
            ID_EX_MemRead   <= 1'b0;
            ID_EX_MemWrite  <= 1'b0;
            ID_EX_MemtoReg  <= 1'b0;
        end
        else begin
            ID_EX_PC        <= IF_ID_PC;
            ID_EX_rs_val    <= rs_val;
            ID_EX_rt_val    <= rt_val;
            ID_EX_imm_ext   <= imm_ext;
            ID_EX_shamt     <= IF_ID_inst[10:6];
            ID_EX_rs        <= rs;
            ID_EX_rt        <= rt;
            ID_EX_rd        <= rd;
            ID_EX_funct     <= IF_ID_inst[5:0];
            ID_EX_RegDst    <= RegDst;
            ID_EX_RegWrite  <= RegWrite;
            ID_EX_ALUSrcA   <= ALUSrcA;
            ID_EX_ALUSrcB   <= ALUSrcB;
            ID_EX_ALUOp     <= ALUOp;
            ID_EX_MemRead   <= MemRead;
            ID_EX_MemWrite  <= MemWrite;
            ID_EX_MemtoReg  <= MemtoReg;
            ID_EX_write_reg <= (RegDst == 1'b1) ? rd : rt;
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            EX_MEM_ALU_out   <= 32'h00000000;
            EX_MEM_rt_val    <= 32'h00000000;
            EX_MEM_write_reg <= 5'b00000;
            EX_MEM_RegWrite  <= 1'b0;
            EX_MEM_MemRead   <= 1'b0;
            EX_MEM_MemWrite  <= 1'b0;
            EX_MEM_MemtoReg  <= 1'b0;
        end
        else begin
            EX_MEM_ALU_out   <= ALU_result;
            EX_MEM_rt_val    <= ID_EX_rt_val;
            EX_MEM_write_reg <= ID_EX_write_reg;
            EX_MEM_RegWrite  <= ID_EX_RegWrite;
            EX_MEM_MemRead   <= ID_EX_MemRead;
            EX_MEM_MemWrite  <= ID_EX_MemWrite;
            EX_MEM_MemtoReg  <= ID_EX_MemtoReg;
        end
    end

    always @(posedge clk) begin
        $display("[CLK=%0t] PC=%08x | inst=%08x | next_PC=%08x | stall=%b", $time, PC, inst, next_PC, stall);
        $display("         rs=%2d(%08x) rt=%2d(%08x) rd=%2d", rs, rs_val, rt, rt_val, write_reg);
        $display("         RegWrite=%b MemRead=%b MemWrite=%b", RegWrite, MemRead, MemWrite);
        $display("         ALU_result=%08x | MEM_read=%08x", ALU_result, mem_read_data);
    end

    always @(posedge clk) begin
        if (rst) begin
            MEM_WB_ALU_out   <= 32'h00000000;
            MEM_WB_mem_data  <= 32'h00000000;
            MEM_WB_RegDst    <= 5'b00000;
            MEM_WB_RegWrite  <= 1'b0;
            MEM_WB_MemtoReg  <= 1'b0;
        end
        else begin
            MEM_WB_ALU_out   <= EX_MEM_ALU_out;
            MEM_WB_mem_data  <= mem_read_data;
            MEM_WB_RegDst    <= EX_MEM_write_reg;
            MEM_WB_RegWrite  <= EX_MEM_RegWrite;
            MEM_WB_MemtoReg  <= EX_MEM_MemtoReg;
        end
    end

    assign halt = (IF_ID_inst === 32'h00000000);

endmodule
