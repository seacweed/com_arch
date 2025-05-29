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
	output reg SavePC=0
    );

	always @(*) begin
		case (opcode)
            `OP_RTYPE:begin
                RegDst = 1;
                RegWrite = 1;
                case (funct)
                    `FUNCT_JR: JR = 1;
                    `FUNCT_SLL: ALUOp = `ALU_SLL;
                    `FUNCT_SRL: ALUOp = `ALU_SRL;
                    `FUNCT_SRA: ALUOp = `ALU_SRA;
                    `FUNCT_ADDU: ALUOp = `ALU_ADDU;
                    `FUNCT_SUBU: ALUOp = `ALU_SUBU;
                    `FUNCT_AND: ALUOp = `ALU_AND;
                    `FUNCT_OR: ALUOp = `ALU_OR;
                    `FUNCT_XOR: ALUOp = `ALU_XOR;
                    `FUNCT_NOR: ALUOp = `ALU_NOR;
                    `FUNCT_SLT: ALUOp = `ALU_SLT;
                    `FUNCT_SLTU: ALUOp = `ALU_SLTU;
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
            end
            `OP_BNE:begin
                Branch = 1;
                ALUOp = 12;
                SignExtend = 1;
                RegDst = 1;
            end
            `OP_ADDIU:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 0;
                SignExtend = 1;
            end
            `OP_SLTI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 9;
                SignExtend = 1;
            end
            `OP_SLTIU:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 10;
                SignExtend = 1;
            end
            `OP_ANDI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 1;
            end
            `OP_ORI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 3;
            end
            `OP_XORI:begin
                ALUSrc = 1;
                RegWrite = 1;
                ALUOp = 8;
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
            end
            `OP_SW:begin
                MemWrite = 1;
                ALUSrc = 1;
                SignExtend = 1;
                ALUOp = 0;
            end
        endcase
	end
endmodule
