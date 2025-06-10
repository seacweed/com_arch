#ifndef CPU_H
#define CPU_H


#include <stdint.h>
#include "ALU.h"
#include "RF.h"
#include "MEM.h"
#include "CTRL.h"

// IF/ID Register
struct IF_ID {
    uint32_t inst = 0;
    uint32_t PC_plus_4 = 0;
};

// ID/EX Register
struct ID_EX {
    CTRL::ParsedInst parsed;
    CTRL::Controls ctrl;
    uint32_t rs_data = 0;
    uint32_t rt_data = 0;
    uint32_t ext_imm = 0;
    uint32_t PC_plus_4 = 0;
};

// EX/MEM Register
struct EX_MEM {
    CTRL::ParsedInst parsed;
    CTRL::Controls ctrl;
    uint32_t alu_result = 0;
    uint32_t rs_data = 0;
    uint32_t rt_data = 0;
    uint32_t wr_addr = 0;
    uint32_t ext_imm = 0;
    uint32_t PC_plus_4 = 0;
};

// MEM/WB Register
struct MEM_WB {
    CTRL::ParsedInst parsed;
    CTRL::Controls ctrl;
    uint32_t alu_result = 0;
    uint32_t mem_data = 0;
    uint32_t wr_addr = 0;
    uint32_t PC_plus_4 = 0;
};

struct WB_sub {
    uint32_t RegWrite = 0;
    uint32_t wr_addr = 0;
    uint32_t pc = 0;

};

class CPU {
public:
    CPU(); // Constructor
	void init(std::string inst_file);
    uint32_t tick(); // Run simulation
    ALU alu;
    RF rf;
    CTRL ctrl;
	MEM mem;

    //stage reg
    IF_ID IF_ID_reg;
    ID_EX ID_EX_reg;
    EX_MEM EX_MEM_reg;
    MEM_WB MEM_WB_reg;
    WB_sub WB_last;

	// Act like a storage element
	uint32_t PC;
    bool stall;
    bool branch_flush;
    uint32_t cnt;


    void IF_stage();
    void ID_stage();
    void EX_stage();
    void MEM_stage();
    void WB_stage();

};

#endif // CPU_H

