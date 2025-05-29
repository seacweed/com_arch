#include <iomanip>
#include <iostream>
#include "CPU.h"
#include "globals.h"
#include "HAZARD.h"

#define VERBOSE 0

using namespace std;

// Constructor
CPU::CPU() {}

void CPU::init(string inst_file) {
    rf.init(false);
    mem.load(inst_file);
    PC = 0;
	cnt = 10;

    status = CONTINUE;

    // Clear all pipeline registers
    IF_ID_reg = {};
    ID_EX_reg = {};
    EX_MEM_reg = {};
    MEM_WB_reg = {};

	stall = false;
	branch_flush = false;
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
	if(stall) {
		cout<<"stall...\n";
		return;
	}
	if(branch_flush){
		branch_flush = false;
		IF_ID_reg = {};
		return;
	}
	//cout<<"PC : "<<PC<<"\n";
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

	bool hazard = false;
	hazard |= HAZARD::checkEXHazard(MEM_WB_reg_sub.ctrl, controls, MEM_WB_reg_sub.parsed, parsed_inst);
	cout<<"ex h = "<<HAZARD::checkEXHazard(MEM_WB_reg_sub.ctrl, controls, MEM_WB_reg_sub.parsed, parsed_inst)<<"\n";
	hazard |= HAZARD::checkMEMHazard(EX_MEM_reg.ctrl, controls, EX_MEM_reg.parsed, parsed_inst);
	cout<<"mem h = "<<HAZARD::checkMEMHazard(EX_MEM_reg.ctrl, controls, EX_MEM_reg.parsed, parsed_inst)<<"\n";
	hazard |= HAZARD::checkWBHazard(MEM_WB_reg.ctrl, controls, MEM_WB_reg.parsed, parsed_inst);
	cout<<"wb h = "<<HAZARD::checkWBHazard(MEM_WB_reg.ctrl, controls, MEM_WB_reg.parsed, parsed_inst)<<"\n";

	// ID_stage 안에서 hazard 발견 시
	if (hazard) {
		ID_EX_reg = {};  // bubble
		stall = true;    // IF 멈추게 함
		return;
	}
	else{
		stall = false;
	}

	
    ID_EX_reg.PC_plus_4 = IF_ID_reg.PC_plus_4;
    // JR
    if (controls.JR) {
		branch_flush = true;
		//cout<<"jr where = "<<hex<<rs_data<<'\n';
        PC = rs_data;
        IF_ID_reg = {};
    }

	// Jump
    if (controls.Jump) {
		branch_flush = true;
        PC = (IF_ID_reg.PC_plus_4 & 0xF0000000) | (parsed_inst.immj << 2);
        IF_ID_reg = {};
    }

    if (controls.Branch) {
        bool taken = (parsed_inst.opcode == 0x04 && rs_data == rt_data) || // BEQ
                     (parsed_inst.opcode == 0x05 && rs_data != rt_data);  // BNE
        if (taken) {
			branch_flush = true;
            PC = IF_ID_reg.PC_plus_4 + (ext_imm << 2);
            // Flush IF/ID pipeline register
            IF_ID_reg = {};
        }
    }

    // Save into pipeline register
    ID_EX_reg.ctrl = controls;
    ID_EX_reg.parsed = parsed_inst;
    ID_EX_reg.rs_data = rs_data;
    ID_EX_reg.rt_data = rt_data;
    ID_EX_reg.ext_imm = ext_imm;
	//cout<<"PC_plus_4 = "<<ID_EX_reg.PC_plus_4<<'\n';



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

	//cout<<"op1 = "<<hex<<operand1<<" op2 = "<<hex<<operand2<<'\n';


    uint32_t alu_result;
    alu.compute(operand1, operand2, ID_EX_reg.parsed.shamt, ID_EX_reg.ctrl.ALUOp, &alu_result);

    EX_MEM_reg.alu_result = alu_result;
    EX_MEM_reg.rt_data = ID_EX_reg.rt_data;
    EX_MEM_reg.ctrl = ID_EX_reg.ctrl;
    EX_MEM_reg.PC_plus_4 = ID_EX_reg.PC_plus_4;
    EX_MEM_reg.parsed = ID_EX_reg.parsed;

	if(ID_EX_reg.ctrl.RegDst){
		EX_MEM_reg.wr_addr = ID_EX_reg.parsed.rd;
	}
	else{
		EX_MEM_reg.wr_addr = ID_EX_reg.parsed.rt;
	}
	// JAL 처리
    if (ID_EX_reg.ctrl.SavePC) {
		cout<<"savepc : "<<ID_EX_reg.PC_plus_4<<'\n';
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
    MEM_WB_reg.parsed = EX_MEM_reg.parsed;
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

	MEM_WB_reg_sub.ctrl = MEM_WB_reg.ctrl;
	MEM_WB_reg_sub.parsed = MEM_WB_reg.parsed;
}
