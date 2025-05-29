#include "HAZARD.h"


bool HAZARD::checkEXHazard(const CTRL::Controls& ex_ctrl,
                                const CTRL::Controls& ctrl,
                                const CTRL::ParsedInst& ex_inst,
                                const CTRL::ParsedInst& id_inst) {
    uint32_t wr_addr;
    if(ex_ctrl.RegWrite){
        if(ctrl.RegDst){
            wr_addr = ex_inst.rd;
        }
        else{
            wr_addr = ex_inst.rt;
        }
        if(ctrl.read_rs&&(id_inst.rs==wr_addr)){
            return true;
        }
        if(ctrl.read_rt&&(id_inst.rt==wr_addr)){
            return true;
        }
    }
    return false;
}

bool HAZARD::checkMEMHazard(const CTRL::Controls& mem_ctrl,
                                const CTRL::Controls& ctrl,
                                const CTRL::ParsedInst& mem_inst,
                                const CTRL::ParsedInst& id_inst) {
    uint32_t wr_addr;
    if(mem_ctrl.RegWrite){
        if(ctrl.RegDst){
            wr_addr = mem_inst.rd;
        }
        else{
            wr_addr = mem_inst.rt;
        }
        if(ctrl.read_rs&&(id_inst.rs==wr_addr)){
            return true;
        }
        if(ctrl.read_rt&&(id_inst.rt==wr_addr)){
            return true;
        }
    }
    return false;
}

bool HAZARD::checkWBHazard(const CTRL::Controls& wb_ctrl,
                                const CTRL::Controls& ctrl,
                                const CTRL::ParsedInst& wb_inst,
                                const CTRL::ParsedInst& id_inst) {
    uint32_t wr_addr;
    if(wb_ctrl.RegWrite){
        if(ctrl.RegDst){
            wr_addr = wb_inst.rd;
        }
        else{
            wr_addr = wb_inst.rt;
        }
        if(ctrl.read_rs&&(id_inst.rs==wr_addr)){
            return true;
        }
        if(ctrl.read_rt&&(id_inst.rt==wr_addr)){
            return true;
        }
    }
    return false;
}