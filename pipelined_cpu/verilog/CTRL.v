`timescale 1ns / 1ps
`include "GLOBAL.v"

module CTRL(
	// input opcode and funct
	input [5:0] opcode,
	input [5:0] funct,

	// output various ports
	output reg RegDst = 0,
	output reg Jump=0,
	output reg Branch=0,
	output reg JR=0,
	output reg MemRead=0,
	output reg MemtoReg=0,
	output reg MemWrite=0,
	output reg ALUSrc=0,
	output reg SignExtend=0,
	output reg RegWrite=0,
	output reg [3:0] ALUOp=0,
	output reg SavePC=0,
	output reg read_rs=0,
	output reg read_rt=0
    );

	always @(*) begin
        RegDst = 0; Jump = 0; Branch = 0; JR = 0;
        MemRead = 0; MemtoReg = 0; MemWrite = 0;
        ALUSrc = 0; SignExtend = 0; RegWrite = 0;
        ALUOp = 0; SavePC = 0; read_rs = 0; read_rt = 0;
		case (opcode)
            `OP_RTYPE:begin
                RegDst = 1;
                RegWrite = 1;
                case (funct)
                    `FUNCT_JR: begin JR = 1; RegDst = 0; RegWrite = 0; read_rs = 1; end
                    `FUNCT_SLL: begin ALUOp = `ALU_SLL; read_rt = 1;end
                    `FUNCT_SRL: begin ALUOp = `ALU_SRL; read_rt = 1;end
                    `FUNCT_SRA: begin ALUOp = `ALU_SRA; read_rt = 1;end
                    `FUNCT_ADDU: begin ALUOp = `ALU_ADDU; read_rs = 1; read_rt = 1;end
                    `FUNCT_SUBU: begin ALUOp = `ALU_SUBU; read_rs = 1; read_rt = 1;end
                    `FUNCT_AND: begin ALUOp = `ALU_AND; read_rs = 1; read_rt = 1;end
                    `FUNCT_OR: begin ALUOp = `ALU_OR; read_rs = 1; read_rt = 1;end
                    `FUNCT_XOR: begin ALUOp = `ALU_XOR; read_rs = 1; read_rt = 1;end
                    `FUNCT_NOR: begin ALUOp = `ALU_NOR; read_rs = 1; read_rt = 1;end
                    `FUNCT_SLT: begin ALUOp = `ALU_SLT; read_rs = 1; read_rt = 1;end
                    `FUNCT_SLTU: begin ALUOp = `ALU_SLTU; read_rs = 1; read_rt = 1;end
                endcase
            end
            `OP_J: Jump = 1;
            `OP_JAL:begin
                Jump = 1;
                SavePC = 1;
                RegWrite = 1;
            end
            `OP_BEQ:begin
                Branch = 1;
                ALUOp = 11;
                SignExtend = 1;
                RegDst = 1;
                read_rs = 1; 
                read_rt = 1;
            end
            `OP_BNE:begin
                Branch = 1;
                ALUOp = 12;
                SignExtend = 1;
                RegDst = 1;
                read_rs = 1; 
                read_rt = 1;
            end
            `OP_ADDIU:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 0;
                SignExtend = 1;
                read_rs = 1;
            end
            `OP_SLTI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 9;
                SignExtend = 1;
                read_rs = 1;
            end
            `OP_SLTIU:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 10;
                SignExtend = 1;
                read_rs = 1;
            end
            `OP_ANDI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 1;
                read_rs = 1;
            end
            `OP_ORI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 3;
                read_rs = 1;
            end
            `OP_XORI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 8;
                read_rs = 1;
            end
            `OP_LUI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 13;
            end
            `OP_LW:begin
                MemRead = 1;
                MemtoReg = 1;
                ALUSrc = 1;
                SignExtend = 1;
                RegWrite = 1;
                ALUOp = 0;
                read_rs = 1;
            end
            `OP_SW:begin
                MemWrite = 1;
                ALUSrc = 1;
                SignExtend = 1;
                ALUOp = 0;
                read_rs = 1;
                read_rt = 1;
            end
        endcase
	end
endmodule
