#include <iostream>
#include "CTRL.h"
#include "ALU.h"
#include "globals.h"


CTRL::CTRL() {}


void CTRL::controlSignal(uint32_t opcode, uint32_t funct, Controls *controls) {
    *controls = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    switch (cur_step) {
        case IF:
            controls->MemRead = 1;
            controls->IorD = 0;
            controls->ALUSrcA = 0;
            controls->ALUSrcB = 1;
            controls->ALUOp = ALU_ADDU;
            controls->PCWrite = 1;
            controls->PCSource = 0;
            controls->IRWrite = 1;
            cur_step = ID;
            break;

        case ID:
			switch (opcode){
				case OP_J:
                    controls->PCWrite = 1;
                    controls->PCSource = 2;
					cur_step = IF;
                    break;

                case OP_JAL:
                    controls->PCWrite = 1;
                    controls->PCSource = 2;
                    controls->SavePC = 1;
                    controls->RegWrite = 1;
                    cur_step = IF;
                    break;
			}
            controls->ALUSrcA = 0;
            controls->ALUSrcB = 3;
            controls->ALUOp = ALU_ADDU;
            cur_step = EX;
            break;

        case EX:
            switch (opcode) {
                case OP_RTYPE:
					if (funct == FUNCT_JR) {
						controls->PCWrite = 1;
						controls->PCSource = 3;
						cur_step = IF;
						break;
					}
                    controls->RegDst = 1;
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 0;
                    switch (funct) {
                        case FUNCT_ADDU: 
                            controls->ALUOp = ALU_ADDU;
                            break;
                        case FUNCT_AND: 
                            controls->ALUOp = ALU_AND;
                            break;
                        case FUNCT_NOR:  
                            controls->ALUOp = ALU_NOR;
                            break;
                        case FUNCT_OR:   
                            controls->ALUOp = ALU_OR;
                            break;
                        case FUNCT_SLL:  
                            controls->ALUOp = ALU_SLL;
                            break;
                        case FUNCT_SRA:  
                            controls->ALUOp = ALU_SRA;
                            break;
                        case FUNCT_SRL:  
                            controls->ALUOp = ALU_SRL;
                            break;
                        case FUNCT_SUBU: 
                            controls->ALUOp = ALU_SUBU;
                            break;
                        case FUNCT_XOR:  
                            controls->ALUOp = ALU_XOR;
                            break;
                        case FUNCT_SLT:  
                            controls->ALUOp = ALU_SLT;
                            break;
                        case FUNCT_SLTU:  
                            controls->ALUOp = ALU_SLTU;
                            break;
                        default: 
                            break;
                    }
					cur_step = WB;
                    break;

                case OP_LW:
                case OP_SW:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_ADDU;
                    controls->SignExtend = 1;
					cur_step = ME;
                    break;

                case OP_BEQ:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 0;
                    controls->ALUOp = ALU_EQ;
                    controls->PCWriteCond = 1;
                    controls->PCSource = 1;
                    controls->SignExtend = 1;
					cur_step = IF;
                    break;

				case OP_BNE:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 0;
                    controls->ALUOp = ALU_NEQ;
                    controls->PCWriteCond = 1;
                    controls->PCSource = 1;
                    controls->SignExtend = 1;
					cur_step = IF;
                    break;

                case OP_LUI:
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_LUI;
					cur_step = WB;
                    break;

                case OP_ORI:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_OR;
					cur_step = WB;
                    break;
                case OP_ADDIU:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_ADDU;
                    controls->SignExtend = 1;
					cur_step = WB;
                    break;
                case OP_SLTI:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_SLT;
                    controls->SignExtend = 1;
                    cur_step = WB;
                    break;
                case OP_SLTIU:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_SLTU;
                    controls->SignExtend = 1;
                    cur_step = WB;
                    break;
                case OP_ANDI:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_AND;
                    cur_step = WB;
                    break;
                case OP_XORI:
                    controls->ALUSrcA = 1;
                    controls->ALUSrcB = 2;
                    controls->ALUOp = ALU_XOR;
                    cur_step = WB;
                    break;

                default:
					cur_step = IF;
                    break;
            }
            break;

        case ME:
            if (opcode == OP_LW) {
                controls->MemRead = 1;
                controls->IorD = 1;
				cur_step = WB;
            } else if (opcode == OP_SW) {
                controls->MemWrite = 1;
                controls->IorD = 1;
				cur_step = IF;
            } else {
				cur_step = IF;
            }
            break;

        case WB:
            controls->RegWrite = 1;
            switch (opcode){
                case OP_LW:
                    controls->MemtoReg = 1;
                    break;
                case OP_BEQ:
                case OP_BNE:
                case OP_RTYPE:
                    controls->RegDst = 1;
                    break;
            }
            cur_step = IF;
            break;
    }
}


void CTRL::splitInst(uint32_t inst, ParsedInst *parsed_inst) {
	parsed_inst->opcode = inst>>26;
	switch (parsed_inst->opcode)
	{
		case OP_RTYPE:
			parsed_inst->funct = inst%0b1000000;
			inst/=0b1000000;
			parsed_inst->shamt = inst%0b100000;
			inst/=0b100000;
			parsed_inst->rd = inst%0b100000;
			inst/=0b100000;
			parsed_inst->rt = inst%0b100000;
			inst/=0b100000;
			parsed_inst->rs = inst%0b100000;
			parsed_inst->immi = 0;
			parsed_inst->immj = 0;
			break;
		case OP_J: case OP_JAL:
			parsed_inst->immj = inst%0b100000000000000000000000000;
			parsed_inst->rs = 0;
			parsed_inst->rt = 0;
			parsed_inst->rd = 0;
			parsed_inst->shamt = 0;
			parsed_inst->funct = 0;
			parsed_inst->immi = 0;
			break;
		case OP_BEQ: case OP_BNE: case OP_ADDIU: case OP_SLTI: case OP_SLTIU: case OP_ANDI: 
		case OP_ORI: case OP_XORI: case OP_LUI: case OP_LW: case OP_SW:
			parsed_inst->immi = inst%0b10'00000'00000'00000;
			inst/=0b10'00000'00000'00000;
			parsed_inst->rt = inst%0b100000;
			inst/=0b100000;
			parsed_inst->rs = inst%0b100000;
			parsed_inst->rd = 0;
			parsed_inst->shamt = 0;
			parsed_inst->funct = 0;
			parsed_inst->immj = 0;
			break;
		default:
			break;
	}
}

// Sign extension using bitwise shift
void CTRL::signExtend(uint32_t immi, uint32_t SignExtend, uint32_t *ext_imm) {
	if(SignExtend){
		if(immi/0b1'00000'00000'00000!=0) *ext_imm = immi+0xffff0000;
		else *ext_imm = immi;
	}
	else *ext_imm = immi;
}
