`timescale 1ns / 1ps
`include "GLOBAL.v"

module CTRL (
    input wire clk,
    input wire rst,
    input wire [5:0] opcode,
    input wire [5:0] funct,
    output reg MemRead,
    output reg MemWrite,
    output reg PCWrite,
    output reg PCWriteCond,
    output reg IRWrite,
    output reg RegWrite,
    output reg SavePC,
    output reg IorD,
    output reg MemtoReg,
    output reg RegDst,
    output reg ALUSrcA,
    output reg [1:0] ALUSrcB,
    output reg [3:0] ALUOp,
    output reg [1:0] PCSource,
    output reg SignExtend
);

reg [2:0] cur_state, next_state;

// 상태 전이
always @(posedge clk or posedge rst) begin
    if (rst) cur_state <= `STATE_IF;
    else cur_state <= next_state;
end

// control signal 설정
always @(*) begin
    // 기본값 초기화
    MemRead = 0; MemWrite = 0; PCWrite = 0; PCWriteCond = 0;
    IRWrite = 0; RegWrite = 0; SavePC = 0; IorD = 0; MemtoReg = 0;
    RegDst = 0; ALUSrcA = 0; ALUSrcB = 2'd0; ALUOp = `ALU_ADDU;
    PCSource = 2'd0; SignExtend = 0;

    //next_state = cur_state;

    case (cur_state)
        `STATE_IF: begin
            MemRead = 1;
            IRWrite = 1;
            PCWrite = 1;
            ALUSrcA = 0;
            ALUSrcB = 2'd1;
            ALUOp = `ALU_ADDU;
            PCSource = 2'd0;
            next_state = `STATE_ID;
        end

        `STATE_ID: begin
            ALUSrcA = 0;
            ALUSrcB = 2'd3;
            ALUOp = `ALU_ADDU;

            case (opcode)
                `OP_J: begin
                    PCWrite = 1;
                    PCSource = 2'd2;
                    next_state = `STATE_IF;
                end
                `OP_JAL: begin
                    PCWrite = 1;
                    PCSource = 2'd2;
                    SavePC = 1;
                    RegWrite = 1;
                    next_state = `STATE_IF;
                end
                default: next_state = `STATE_EX;
            endcase
        end

        `STATE_EX: begin
            case (opcode)
                `OP_RTYPE: begin
                    if (funct == `FUNCT_JR) begin
                        PCWrite = 1;
                        PCSource = 2'd3;
                        next_state = `STATE_IF;
                    end else begin
                        RegDst = 1;
                        ALUSrcA = 1;
                        ALUSrcB = 2'd0;
                            case (funct)
                                `FUNCT_ADDU: ALUOp = `ALU_ADDU;
                                `FUNCT_SUBU: ALUOp = `ALU_SUBU;
                                `FUNCT_AND:  ALUOp = `ALU_AND;
                                `FUNCT_OR:   ALUOp = `ALU_OR;
                                `FUNCT_XOR:  ALUOp = `ALU_XOR;
                                `FUNCT_NOR:  ALUOp = `ALU_NOR;
                                `FUNCT_SLL:  ALUOp = `ALU_SLL;
                                `FUNCT_SRL:  ALUOp = `ALU_SRL;
                                `FUNCT_SRA:  ALUOp = `ALU_SRA;
                                `FUNCT_SLT:  ALUOp = `ALU_SLT;
                                `FUNCT_SLTU: ALUOp = `ALU_SLTU;
                                default:    ALUOp = 4'd0;
                            endcase
                        next_state = `STATE_WB;
                    end
                end

                `OP_LW, `OP_SW: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_ADDU;
                    SignExtend = 1;
                    next_state = `STATE_MEM;
                end

                `OP_BEQ: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd0;
                    ALUOp = `ALU_EQ;
                    PCWriteCond = 1;
                    PCSource = 2'd1;
                    SignExtend = 1;
                    next_state = `STATE_IF;
                end

                `OP_BNE: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd0;
                    ALUOp = `ALU_NEQ;
                    PCWriteCond = 1;
                    PCSource = 2'd1;
                    SignExtend = 1;
                    next_state = `STATE_IF;
                end

                `OP_LUI: begin
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_LUI;
                    next_state = `STATE_WB;
                end

                `OP_ORI: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_OR;
                    next_state = `STATE_WB;
                end

                `OP_ADDIU: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_ADDU;
                    SignExtend = 1;
                    next_state = `STATE_WB;
                end

                `OP_SLTI: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_SLT;
                    SignExtend = 1;
                    next_state = `STATE_WB;
                end

                `OP_SLTIU: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_SLTU;
                    SignExtend = 1;
                    next_state = `STATE_WB;
                end

                `OP_ANDI: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_AND;
                    next_state = `STATE_WB;
                end

                `OP_XORI: begin
                    ALUSrcA = 1;
                    ALUSrcB = 2'd2;
                    ALUOp = `ALU_XOR;
                    next_state = `STATE_WB;
                end

                default: next_state = `STATE_IF;
            endcase
        end

        `STATE_MEM: begin
            if (opcode == `OP_LW) begin
                MemRead = 1;
                IorD = 1;
                next_state = `STATE_WB;
            end else if (opcode == `OP_SW) begin
                MemWrite = 1;
                IorD = 1;
                next_state = `STATE_IF;
            end else begin
                next_state = `STATE_IF;
            end
        end

        `STATE_WB: begin
            RegWrite = 1;
            if (opcode == `OP_LW)
                MemtoReg = 1;
            else if (opcode == `OP_RTYPE || opcode == `OP_BEQ || opcode == `OP_BNE)
                RegDst = 1;
            next_state = `STATE_IF;
        end
    endcase
end

endmodule