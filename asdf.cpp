#include <iomanip>
#include <iostream>
#include "CPU.h"
#include "globals.h"

#define VERBOSE 0

using namespace std;

// Constructor
CPU::CPU() {}

void CPU::init(string inst_file) {
    rf.init(false);
    mem.load(inst_file);
    PC = 0;

    status = CONTINUE;

    // Clear all pipeline registers
    IF_ID_reg = {};
    ID_EX_reg = {};
    EX_MEM_reg = {};
    MEM_WB_reg = {};
}

// Tick = one cycle in pipeline
uint32_t CPU::tick() {
    if (status != CONTINUE) return 0;

    WB_stage();
    MEM_stage();
    EX_stage();
    ID_stage();
    IF_stage();

    return 1;
}

// Instruction Fetch Stage
void CPU::IF_stage() {
    uint32_t inst;
    mem.imemAccess(PC, &inst);

    IF_ID_reg.inst = inst;
    IF_ID_reg.PC_plus_4 = PC + 4;

    // Predict not taken (PC += 4)
    PC += 4;
}

// Instruction Decode Stage
void CPU::ID_stage() {
    // Parse instruction
    CTRL::ParsedInst parsed_inst;
    ctrl.splitInst(IF_ID_reg.inst, &parsed_inst);

    // Control signal
    CTRL::Controls controls;
    ctrl.controlSignal(parsed_inst.opcode, parsed_inst.funct, &controls);

    // Register read
    uint32_t rs_data, rt_data;
    rf.read(parsed_inst.rs, parsed_inst.rt, &rs_data, &rt_data);

    // Immediate
    uint32_t ext_imm;
    ctrl.signExtend(parsed_inst.immi, controls.SignExtend, &ext_imm);

    // Save into pipeline register
    ID_EX_reg.ctrl = ctrl_signals;
    ID_EX_reg.parsed = parsed;
    ID_EX_reg.rs_data = rs_data;
    ID_EX_reg.rt_data = rt_data;
    ID_EX_reg.ext_imm = ext_imm;
    ID_EX_reg.PC_plus_4 = IF_ID_reg.PC_plus_4;

    // (예: branch 처리 및 flush는 이후 추가)
}

// Execute Stage
void CPU::EX_stage() {
    uint32_t operand1 = ID_EX_reg.rs_data;
    uint32_t operand2;
	if(ID_EX_reg.ctrl.ALUSrc){
		operand2 = ID_EX_reg.ext_imm;
	}
	else{
		operand2 = ID_EX_reg.rt_data;
	}


    uint32_t alu_result;
    alu.compute(operand1, operand2, ID_EX_reg.parsed.shamt, ID_EX_reg.ctrl.ALUOp, &alu_result);

    EX_MEM_reg.alu_result = alu_result;
    EX_MEM_reg.rt_data = ID_EX_reg.rt_data;
    EX_MEM_reg.ctrl = ID_EX_reg.ctrl;
    EX_MEM_reg.PC_plus_4 = ID_EX_reg.PC_plus_4;

	if(ID_EX_reg.ctrl.RegDst){
		EX_MEM_reg.wr_addr = ID_EX_reg.parsed.rd;
	}
	else{
		EX_MEM_reg.wr_addr = ID_EX_reg.parsed.rt;
	}

    // JAL 처리
    if (ID_EX_reg.ctrl.SavePC) {
        EX_MEM_reg.wr_addr = 31;
        EX_MEM_reg.alu_result = ID_EX_reg.PC_plus_4;
    }
}

// Memory Access Stage
void CPU::MEM_stage() {
    uint32_t mem_data;
    mem.dmemAccess(EX_MEM_reg.alu_result, &mem_data, EX_MEM_reg.rt_data,
                   EX_MEM_reg.ctrl.MemRead, EX_MEM_reg.ctrl.MemWrite);

    MEM_WB_reg.ctrl = EX_MEM_reg.ctrl;
    MEM_WB_reg.alu_result = EX_MEM_reg.alu_result;
    MEM_WB_reg.mem_data = mem_data;
    MEM_WB_reg.wr_addr = EX_MEM_reg.wr_addr;
}

// Write Back Stage
void CPU::WB_stage() {
    uint32_t wr_data;

	if(MEM_WB_reg.ctrl.MemtoReg){
		wr_data = MEM_WB_reg.mem_data;
	}
	else{
		wr_data = MEM_WB_reg.alu_result;
	}


    if (MEM_WB_reg.ctrl.RegWrite) {
        rf.write(MEM_WB_reg.wr_addr, wr_data, true);
    }
}
