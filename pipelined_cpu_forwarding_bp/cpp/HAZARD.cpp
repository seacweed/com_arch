#include "HAZARD.h"
//#include <iostream>

using namespace std;

bool HAZARD::checkHazard(uint32_t RegWrite,
                                const CTRL::Controls& ctrl,
                                uint32_t wr_addr,
                                const CTRL::ParsedInst& id_inst) {
    if(RegWrite){
        //cout<<"wr = "<<wr_addr<<" rs = "<<id_inst.rs<<" rt = "<<id_inst.rt<<endl;
        if(ctrl.read_rs&&(id_inst.rs==wr_addr)){
            return true;
        }
        if(ctrl.read_rt&&(id_inst.rt==wr_addr)){
            return true;
        }
    }
    return false;
}