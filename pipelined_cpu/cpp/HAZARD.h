#pragma once
#include "ctrl.h"

class HAZARD {
public:
    static bool checkHazard(uint32_t RegWrite,
                                const CTRL::Controls& ctrl,
                                uint32_t wr_addr,
                                const CTRL::ParsedInst& id_inst);
};
