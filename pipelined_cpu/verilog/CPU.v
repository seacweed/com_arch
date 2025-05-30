`timescale 1ns / 1ps


module CPU(
	input		clk,
	input		rst,
	output 		halt
	);
	
	// Split the instructions
	// Instruction-related wires
	wire [31:0]		inst;
    wire [5:0] 		opcode = IF_ID_inst[31:26];
    wire [4:0] 		rs = IF_ID_inst[25:21];
    wire [4:0] 		rt = IF_ID_inst[20:16];
    wire [4:0] 		rd = IF_ID_inst[15:11];
    wire [4:0] 		shamt = IF_ID_inst[10:6];
    wire [5:0] 		funct = IF_ID_inst[5:0];
    wire [15:0] 	immi = IF_ID_inst[15:0];
    wire [25:0] 	immj = IF_ID_inst[25:0];

	// Control-related wires
	wire			RegDst;
	wire			Jump;
	wire 			Branch;
	wire 			JR;
	wire			MemRead;
	wire			MemtoReg;
	wire 			MemWrite;
	wire			ALUSrc;
	wire			SignExtend;
	wire			RegWrite;
	wire [3:0]		ALUOp;
	wire			SavePC;
	wire			read_rs;
	wire			read_rt;

	// Sign extend the immediate
	wire [31:0]		ext_imm = (SignExtend) ? {{16{immi[15]}}, immi} : {16'b0, immi};

	// RF-related wires
	wire [31:0]		rs_data;
	wire [31:0]		rt_data;

	// MEM-related wires
	wire [31:0]		mem_addr;
	wire [31:0]		mem_write_data;
	wire [31:0]		mem_read_data;

	// ALU-related wires
	wire [31:0]		operand1 = rs_data;

	// Define PC
	reg [31:0]	PC;
	reg [31:0]	PC_next;
	
	// in execute
    wire [31:0] operand2 = (ID_EX_ALUSrc) ? ID_EX_ext_imm : ID_EX_rt_data;
    wire [31:0] alu_result;

	// IF/ID Pipeline Register
    reg [31:0] IF_ID_inst, IF_ID_PC_plus_4;

    // ID/EX Pipeline Register
    reg [31:0] ID_EX_rs_data, ID_EX_rt_data, ID_EX_ext_imm, ID_EX_PC_plus_4;
    reg [25:0] ID_EX_immj;
    reg [4:0]  ID_EX_rs, ID_EX_rt, ID_EX_rd, ID_EX_shamt, ID_EX_wr_addr;
    reg [5:0]  ID_EX_opcode, ID_EX_funct;
    reg        ID_EX_RegDst, ID_EX_ALUSrc, ID_EX_MemRead, ID_EX_MemWrite;
    reg        ID_EX_MemtoReg, ID_EX_RegWrite, ID_EX_Branch, ID_EX_Jump, ID_EX_JR, ID_EX_SavePC;
    reg [3:0]  ID_EX_ALUOp;

    // EX/MEM Pipeline Register
    reg [31:0] EX_MEM_alu_result, EX_MEM_rt_data;
    reg [4:0]  EX_MEM_wr_addr;
    reg        EX_MEM_MemRead, EX_MEM_MemWrite, EX_MEM_MemtoReg, EX_MEM_RegWrite;

    // MEM/WB Pipeline Register
    reg [31:0] MEM_WB_alu_result, MEM_WB_mem_data;
    reg [4:0]  MEM_WB_wr_addr;
    reg        MEM_WB_MemtoReg, MEM_WB_RegWrite;

	// Define the wires

	//assign halt				= (inst == 32'b0);
	reg[3:0] cnt;

	wire flush;
	reg stall;

	//initial cnt = 6;

	// ---------- IF Stage ----------- 
	always @(posedge clk or posedge rst) begin
        if (rst || flush) begin
            IF_ID_inst <= 0;
            IF_ID_PC_plus_4 <= 0;
        end else if (!stall) begin
            IF_ID_inst <= inst;
            IF_ID_PC_plus_4 <= PC + 4;
        end
    end

    // -------------------- ID Stage --------------------
    always @(posedge clk or posedge rst) begin
        if (rst || stall) begin
            ID_EX_RegDst    <= 0;
            ID_EX_ALUSrc    <= 0;
            ID_EX_MemRead   <= 0;
            ID_EX_MemWrite  <= 0;
            ID_EX_MemtoReg  <= 0;
            ID_EX_RegWrite  <= 0;
            ID_EX_Branch    <= 0;
            ID_EX_Jump      <= 0;
            ID_EX_JR        <= 0;
            ID_EX_SavePC    <= 0;
            ID_EX_ALUOp     <= 0;
            ID_EX_wr_addr   <= 0;
        end else begin
            ID_EX_rs        <= rs;
            ID_EX_rt        <= rt;
            ID_EX_rd        <= rd;
            ID_EX_shamt     <= shamt;
            ID_EX_rs_data   <= rs_data;
            ID_EX_rt_data   <= rt_data;
            ID_EX_ext_imm   <= ext_imm;
            ID_EX_PC_plus_4 <= IF_ID_PC_plus_4;
            ID_EX_opcode    <= opcode;
            ID_EX_funct     <= funct;
            ID_EX_immj      <= immj;
            ID_EX_RegDst    <= RegDst;
            ID_EX_ALUSrc    <= ALUSrc;
            ID_EX_MemRead   <= MemRead;
            ID_EX_MemWrite  <= MemWrite;
            ID_EX_MemtoReg  <= MemtoReg;
            ID_EX_RegWrite  <= RegWrite;
            ID_EX_Branch    <= Branch;
            ID_EX_Jump      <= Jump;
            ID_EX_JR        <= JR;
            ID_EX_SavePC    <= SavePC;
            ID_EX_ALUOp     <= ALUOp;
            ID_EX_wr_addr   <= (RegDst) ? rd : rt;
        end
    end

	always @(*) begin
		stall = 0;

		if (ID_EX_RegWrite && (
			(read_rs && (ID_EX_wr_addr != 0) && (ID_EX_wr_addr == rs)) ||
			(read_rt && (ID_EX_wr_addr != 0) && (ID_EX_wr_addr == rt))
		)) stall = 1;

		else if (EX_MEM_RegWrite && (
			(read_rs && (EX_MEM_wr_addr != 0) && (EX_MEM_wr_addr == rs)) ||
			(read_rt && (EX_MEM_wr_addr != 0) && (EX_MEM_wr_addr == rt))
		)) stall = 1;

		else if (MEM_WB_RegWrite && (
			(read_rs && (MEM_WB_wr_addr != 0) && (MEM_WB_wr_addr == rs)) ||
			(read_rt && (MEM_WB_wr_addr != 0) && (MEM_WB_wr_addr == rt))
		)) stall = 1;
	end


    // ---------- Execute ----------

    always @(posedge clk) begin
        EX_MEM_alu_result <= (ID_EX_SavePC) ? ID_EX_PC_plus_4 : alu_result;
        EX_MEM_rt_data    <= ID_EX_rt_data;
        EX_MEM_MemRead    <= ID_EX_MemRead;
        EX_MEM_MemWrite   <= ID_EX_MemWrite;
        EX_MEM_MemtoReg   <= ID_EX_MemtoReg;
        EX_MEM_RegWrite   <= ID_EX_RegWrite;
        EX_MEM_wr_addr    <= (ID_EX_SavePC) ? 5'd31 : ID_EX_wr_addr;
    end

    // ---------- Memory Stage ----------
    always @(posedge clk) begin
        MEM_WB_alu_result <= EX_MEM_alu_result;
        MEM_WB_mem_data   <= mem_read_data;
        MEM_WB_wr_addr    <= EX_MEM_wr_addr;
        MEM_WB_MemtoReg   <= EX_MEM_MemtoReg;
        MEM_WB_RegWrite   <= EX_MEM_RegWrite;
    end

    // ---------- Branch Flush 처리 ----------
    wire branch_taken = Branch && (
		(opcode == `OP_BEQ && rs_data == rt_data) ||
		(opcode == `OP_BNE && rs_data != rt_data)
	);
    assign flush = branch_taken || Jump || JR;

	// --- Control Unit 연결 ---
	CTRL ctrl (
		.opcode(opcode),
		.funct(funct),
		.RegDst(RegDst),
		.Jump(Jump),
		.Branch(Branch),
		.JR(JR),
		.MemRead(MemRead),
		.MemtoReg(MemtoReg),
		.MemWrite(MemWrite),
		.ALUSrc(ALUSrc),
		.SignExtend(SignExtend),
		.RegWrite(RegWrite),
		.SavePC(SavePC),
		.ALUOp(ALUOp),
		.read_rs(read_rs),
		.read_rt(read_rt)
	);

	// --- Register File ---
	RF rf (
		.clk(clk),
		.rst(rst),
		.rd_addr1(rs),
		.rd_addr2(rt),
		.rd_data1(rs_data),
		.rd_data2(rt_data),
		.RegWrite(MEM_WB_RegWrite),
		.wr_addr(MEM_WB_wr_addr),
		.wr_data((MEM_WB_MemtoReg) ? MEM_WB_mem_data : MEM_WB_alu_result)
	);

	// --- ALU 연산 ---
	ALU alu (
		.operand1(ID_EX_rs_data),
		.operand2(operand2),
		.shamt(ID_EX_shamt),
		.funct(ID_EX_ALUOp),
		.alu_result(alu_result)
	);

	// --- Memory ---
	MEM mem (
        .clk(clk),
        .rst(rst),
        .inst_addr(PC),
        .inst(inst),
        .mem_addr(EX_MEM_alu_result),
        .MemWrite(EX_MEM_MemWrite),
        .mem_write_data(EX_MEM_rt_data),
        .mem_read_data(mem_read_data)
    );

	// --- PC Update Logic ---
// PC update logic (ID 기준)
	always @(*) begin
		PC_next = PC + 4;

		if (branch_taken)
			PC_next = IF_ID_PC_plus_4 + (ext_imm << 2);
		else if (Jump)
			PC_next = {IF_ID_PC_plus_4[31:28], immj, 2'b00};
		else if (JR)
			PC_next = rs_data;
	end
    
    reg term;
    assign halt = term;
	// --- PC Register ---
	always @(posedge clk or posedge rst) begin
		if (rst) begin
			PC <= 0;
			cnt <= 6;
			term <= 0;
		end
		else if (!stall) begin
			if(inst == 32'b0) begin
				if(cnt == 0) begin
					term <= 1;
				end else begin
					cnt <= cnt - 1;
				end
			end else begin
				PC <= PC_next;
				cnt <= 6;
			end
		end
	end

endmodule