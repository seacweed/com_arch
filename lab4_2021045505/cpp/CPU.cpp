#include <iomanip>
#include <iostream>
#include "CPU.h"
#include "globals.h"

#define VERBOSE 0

using namespace std;

CPU::CPU() {}

// Reset stateful modules
void CPU::init(string inst_file) {
	// Initialize the register file
	rf.init(false);
	// Load the instructions from the memory
	mem.load(inst_file);

	ctrl.cur_step = IF;
	// Reset the program counter
	PC = 0;

	//set initial step

	// Set the debugging status
	status = CONTINUE;
}

// This is a cycle-accurate simulation
uint32_t CPU::tick() {
	// Access the instruction memory

	// parsed & control signals (wire)
	CTRL::ParsedInst parsed_inst;
	CTRL::Controls controls;
	uint32_t ext_imm;

	// Default wires and control signals
	uint32_t rs_data, rt_data;
	uint32_t wr_addr;
	uint32_t wr_data;
	uint32_t operand1;
	uint32_t operand2;
	uint32_t alu_result;

	// PC_next
	uint32_t PC_next;

	// You can declare your own wires (if you want ...)
	uint32_t mem_data;
	uint32_t mem_addr;





	ctrl.splitInst(IR, &parsed_inst);
	ctrl.controlSignal(parsed_inst.opcode, parsed_inst.funct ,&controls);
	ctrl.signExtend(parsed_inst.immi, controls.SignExtend, &ext_imm);
	if (status != CONTINUE) return 0;

	if(!controls.IorD){
		mem_addr = PC;
		if (status != CONTINUE) return 0;
	}
	else{
		mem_addr = ALUOut;
	}
	

	mem.memAccess(mem_addr, &mem_data, B, controls.MemRead, controls.MemWrite);
	if (status != CONTINUE) return 0;

	if(controls.IRWrite){
		IR = mem_data;
		if (IR == 0)
			status = TERMINATE;
	}

	MDR = mem_data;
	
	if(controls.MemtoReg){
		wr_data = MDR;
	}
	else{
		wr_data = ALUOut;
	}

	rf.read(parsed_inst.rs, parsed_inst.rt, &rs_data, &rt_data);
	if (status != CONTINUE) return 0;

	A = rs_data;
	B = rt_data;

	if(controls.ALUSrcA){
		operand1 = A;
	}
	else{
		operand1 = PC;
	}

	switch (controls.ALUSrcB){
		case 0:
			operand2 = B;
			break;
		case 1:
			operand2 = 4;
			break;
		case 2:
			operand2 = ext_imm;
			break;
		case 3:
			operand2 = ext_imm<<2;
			break;
	}

	alu.compute(operand1, operand2, parsed_inst.shamt, controls.ALUOp, &alu_result);
	if (status != CONTINUE) return 0;


	if(controls.RegDst){
		wr_addr = parsed_inst.rd;
	}
	else{
		wr_addr = parsed_inst.rt;
	}

	switch (controls.PCSource){
		case 0:
			PC_next = alu_result;
			break;
		case 1:
			PC_next = ALUOut;
			break;
		case 2:
			PC_next = (PC & 0xF0000000) | (parsed_inst.immj << 2);
			break;
		case 3:
			PC_next = rs_data;
			break;
	}

	ALUOut = alu_result;

	if(controls.SavePC){
		wr_addr = 31;
		wr_data = PC;
	}
	
	if((controls.PCWriteCond&&alu_result)||controls.PCWrite){
		PC = PC_next;
	}

	rf.write(wr_addr, wr_data, controls.RegWrite);
	if (status != CONTINUE) return 0;

	return 1;
}

