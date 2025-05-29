#include <iostream>
#include "CTRL.h"
#include "ALU.h"
#include "globals.h"


CTRL::CTRL() {}

void CTRL::controlSignal(uint32_t opcode, uint32_t funct, Controls *controls) {
	*controls={0,0,0,0,0,0,0,0,0,0,0,0};
	switch (opcode)
	{
		case OP_RTYPE:
			controls->RegDst = 1;
			controls->RegWrite = 1;
			switch (funct)
			{
				case FUNCT_JR:
					controls->JR = 1;
					break;
				case FUNCT_SLL:
					controls->ALUOp = ALU_SLL;
					break;
				case FUNCT_SRL:
					controls->ALUOp = ALU_SRL;
					break;
				case FUNCT_SRA:
					controls->ALUOp = ALU_SRA;
					break;
				case FUNCT_ADDU:
					controls->ALUOp = ALU_ADDU;
					break;
				case FUNCT_SUBU:
					controls->ALUOp = ALU_SUBU;
					break;
				case FUNCT_AND:
					controls->ALUOp = ALU_AND;
					break;
				case FUNCT_OR:
					controls->ALUOp = ALU_OR;
					break;
				case FUNCT_XOR:
					controls->ALUOp = ALU_XOR;
					break;
				case FUNCT_NOR:
					controls->ALUOp = ALU_NOR;
					break;
				case FUNCT_SLT:
					controls->ALUOp = ALU_SLT;
					break;
				case FUNCT_SLTU:
					controls->ALUOp = ALU_SLTU;
					break;
			}
			break;
		case OP_J: 
			controls->Jump = 1;
			break;
		case OP_JAL:
			controls->Jump = 1;
			controls->SavePC = 1;
			controls->RegWrite = 1;
			break;
		case OP_BEQ: 
			controls->Branch = 1;
			controls->ALUOp = 11;
			controls->SignExtend = 1;
			controls->RegDst = 1;
			break;
		case OP_BNE:
			controls->Branch = 1;
			controls->ALUOp = 12;
			controls->SignExtend = 1;
			controls->RegDst = 1;
			break;
		case OP_ADDIU:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 0;
			controls->SignExtend = 1;
			break;
		case OP_SLTI:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 9;
			controls->SignExtend = 1;
			break;
		case OP_SLTIU:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 10;
			controls->SignExtend = 1;
			break;
		case OP_ANDI:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 1;
			break;
		case OP_ORI:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 3;
			break;
		case OP_XORI:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 8;
			break;
		case OP_LUI:
			controls->ALUSrc = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 13;
			break;
		case OP_LW:
			controls->MemRead = 1;
			controls->MemtoReg = 1;
			controls->ALUSrc = 1;
			controls->SignExtend = 1;
			controls->RegWrite = 1;
			controls->ALUOp = 0;
			break;
		case OP_SW:
			controls->MemWrite = 1;
			controls->ALUSrc = 1;
			controls->SignExtend = 1;
			controls->ALUOp = 0;
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
