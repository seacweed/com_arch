#pragma once
#include "ctrl.h"

class HAZARD {
public:
    static bool checkEXHazard(const CTRL::Controls& ex_ctrl,
                                    const CTRL::Controls& ctrl,
                                    const CTRL::ParsedInst& ex_inst,
                                    const CTRL::ParsedInst& id_inst);
    static bool checkMEMHazard(const CTRL::Controls& mem_ctrl,
                                    const CTRL::Controls& ctrl,
                                    const CTRL::ParsedInst& mem_inst,
                                    const CTRL::ParsedInst& id_inst);
    static bool checkWBHazard(const CTRL::Controls& wb_ctrl,
                                    const CTRL::Controls& ctrl,
                                    const CTRL::ParsedInst& wb_inst,
                                    const CTRL::ParsedInst& id_inst);
};
