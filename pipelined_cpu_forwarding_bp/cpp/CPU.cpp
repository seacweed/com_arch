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
	cnt = 6;

    status = CONTINUE;

    // Clear all pipeline registers
    IF_ID_reg = {};
    ID_EX_reg = {};
    EX_MEM_reg = {};
    MEM_WB_reg = {};

	stall = false;
	branch_flush = false;

    for (int i = 0; i < 64; ++i) {
        BTB[i] = {false, 0, 0, false};
    }
    for (int i = 0; i < 256; ++i) {
        PHT[i] = 1; // 01 (weakly not taken)
    }
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
		//cout<<"stall...\n";
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

    IF_ID_reg.PC_plus_4 = PC + 4;
    IF_ID_reg.inst = inst;

    if(inst==0){
        if(cnt==0){
            status = TERMINATE;
        }
        cnt--;
        return;
    }

    uint32_t idx = (PC >> 2) & 0x3F;
    uint32_t tag = PC >> 8;
    bool pred_taken = false;
    uint32_t pred_target = PC + 4;

    if (BTB[idx].valid && BTB[idx].tag == tag) {
        if (BTB[idx].isJump) {
            pred_taken = true;
        } else {
            uint8_t pht_idx = (PC >> 2) & 0xFF;
            if (PHT[pht_idx] >= 2) pred_taken = true;
        }
        if (pred_taken) pred_target = BTB[idx].target;
    }

    PC = pred_target;
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
    if (EX_MEM_reg.ctrl.MemRead) {
        hazard |= HAZARD::checkHazard(EX_MEM_reg.ctrl.RegWrite, controls, EX_MEM_reg.wr_addr, parsed_inst);
    }
	//hazard |= HAZARD::checkHazard(EX_MEM_reg.ctrl.RegWrite, controls, EX_MEM_reg.wr_addr, parsed_inst);
	//cout<<"ex h = "<<HAZARD::checkHazard(EX_MEM_reg.ctrl.RegWrite, controls, EX_MEM_reg.wr_addr, parsed_inst)<<"\n";
	//hazard |= HAZARD::checkHazard(MEM_WB_reg.ctrl.RegWrite, controls, MEM_WB_reg.wr_addr, parsed_inst);
	//cout<<"mem h = "<<HAZARD::checkHazard(MEM_WB_reg.ctrl.RegWrite, controls, MEM_WB_reg.wr_addr, parsed_inst)<<"\n";
	//hazard |= HAZARD::checkHazard(WB_last.RegWrite, controls, WB_last.wr_addr, parsed_inst);
	//cout<<"wb h = "<<HAZARD::checkHazard(WB_last.RegWrite, controls, WB_last.wr_addr, parsed_inst)<<"\n";
    //cout<<"PC = "<<hex<<IF_ID_reg.PC_plus_4-4<<'\n';

	if (hazard) {
		ID_EX_reg = {}; 
		stall = true; 
		return;
	}
	else{
		stall = false;
	}
    
    if (WB_last.RegWrite && WB_last.wr_addr != 0) {
        if (WB_last.wr_addr == parsed_inst.rs)
            rs_data = WB_last.wr_data; 
        if ((!controls.ALUSrc || controls.MemWrite) && WB_last.wr_addr == parsed_inst.rt){
            //cout<<"WB\n";
            rt_data = WB_last.wr_data;
        }
    }


    if (MEM_WB_reg.ctrl.RegWrite && MEM_WB_reg.wr_addr != 0) {
        if (MEM_WB_reg.wr_addr == parsed_inst.rs){
            if(MEM_WB_reg.ctrl.MemtoReg)
                rs_data = MEM_WB_reg.mem_data;
            else
                rs_data = MEM_WB_reg.alu_result;
        }
        if ((!controls.ALUSrc || controls.MemWrite) && MEM_WB_reg.wr_addr == parsed_inst.rt){
            if(MEM_WB_reg.ctrl.MemtoReg)
                rt_data = MEM_WB_reg.mem_data;
            else 
                rt_data = MEM_WB_reg.alu_result;
            //cout<<"MEM\n";
        }
    }

    if (EX_MEM_reg.ctrl.RegWrite && EX_MEM_reg.wr_addr != 0) {
        if (EX_MEM_reg.wr_addr == parsed_inst.rs)
            rs_data = EX_MEM_reg.alu_result;
        if ((!controls.ALUSrc || controls.MemWrite) && EX_MEM_reg.wr_addr == parsed_inst.rt){
            rt_data = EX_MEM_reg.alu_result;
            //cout<<"EX\n";
        }
    }

    //cout<<"rs : "<<hex<<rs_data<<" rt : "<<hex<<rt_data<<" opcode : "<<parsed_inst.opcode<<'\n';
    //cout<<"rsaddr : "<<parsed_inst.rs<<" rtaddr : "<<parsed_inst.rt<<'\n';


	
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

        uint32_t idx = ((IF_ID_reg.PC_plus_4 - 4) >> 2) & 0x3F;
        uint32_t tag = (IF_ID_reg.PC_plus_4 - 4) >> 8;
        uint32_t target = (IF_ID_reg.PC_plus_4 & 0xFC000000) | (parsed_inst.immj << 2);

        BTB[idx].valid = true;
        BTB[idx].tag = tag;
        BTB[idx].target = target;
        BTB[idx].isJump = true;

        PC = target;
        IF_ID_reg = {};
    }

    if (controls.Branch) {
        bool actual_taken = (parsed_inst.opcode == 0x04 && rs_data == rt_data) ||
                            (parsed_inst.opcode == 0x05 && rs_data != rt_data);

        uint32_t actual_target = IF_ID_reg.PC_plus_4 + (ext_imm << 2);
        uint32_t idx = ((IF_ID_reg.PC_plus_4 - 4) >> 2) & 0x3F;
        uint32_t tag = (IF_ID_reg.PC_plus_4 - 4) >> 8;
        uint32_t pht_idx = ((IF_ID_reg.PC_plus_4 - 4) >> 2) & 0xFF;

        BTB[idx].valid = true;
        BTB[idx].tag = tag;
        BTB[idx].target = actual_target;
        BTB[idx].isJump = false;

        bool pred_taken = false;
        if (PHT[pht_idx] >= 2) pred_taken = true;

        if (actual_taken != pred_taken) {
            branch_flush = true;
            PC = actual_taken
                ? actual_target  
                : IF_ID_reg.PC_plus_4; 
            IF_ID_reg = {};
        }

        if (actual_taken) {
            if (PHT[pht_idx] < 3) PHT[pht_idx]++;
        } else {
            if (PHT[pht_idx] > 0) PHT[pht_idx]--;
        }
    }

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
    MEM_WB_reg.PC_plus_4 = EX_MEM_reg.PC_plus_4;
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

	WB_last.RegWrite = MEM_WB_reg.ctrl.RegWrite;
    WB_last.wr_addr = MEM_WB_reg.wr_addr;
    WB_last.wr_data = wr_data;
    //WB_last.pc = MEM_WB_reg.PC_plus_4-4;
}