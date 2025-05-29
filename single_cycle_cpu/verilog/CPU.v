`timescale 1ns / 1ps


module CPU(
	input		clk,
	input		rst,
	output 		halt
	);
	
	// Split the instructions
	// Instruction-related wires
	wire [31:0]		inst;
	wire [5:0]		opcode = inst[31:26];
	wire [4:0]		rs = inst[25:21];
	wire [4:0]		rt = inst[20:16];
	wire [4:0]		rd = inst[15:11];
	wire [4:0]		shamt = inst[10:6];
	wire [5:0]		funct = inst[5:0];
	wire [15:0]		immi = inst[15:0];
	wire [25:0]		immj = inst[25:0];

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

	// Sign extend the immediate
	wire [31:0]		ext_imm = (SignExtend) ? {{16{immi[15]}}, immi} : {16'b0, immi};

	// RF-related wires
	wire [4:0]		rd_addr1;
	wire [4:0]		rd_addr2;
	wire [31:0]		rd_data1;
	wire [31:0]		rd_data2;
	reg [4:0]		wr_addr;
	reg [31:0]		wr_data;

	// MEM-related wires
	wire [31:0]		mem_addr;
	wire [31:0]		mem_write_data;
	wire [31:0]		mem_read_data;

	// ALU-related wires
	wire [31:0]		operand1 = rd_data1;
	wire [31:0]		operand2 = (ALUSrc) ? ext_imm : rd_data2;
	wire [31:0]		alu_result;

	// Define PC
	reg [31:0]	PC;
	reg [31:0]	PC_next;

	// Define the wires

	assign halt				= (inst == 32'b0);

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
		.ALUOp(ALUOp)
	);

	// --- Register File ---
	RF rf (
		.clk(clk),
		.rst(rst),
		.rd_addr1(rd_addr1),
		.rd_addr2(rd_addr1),
		.rd_data1(rd_data1),
		.rd_data2(rd_data2),
		.RegWrite(RegWrite),
		.wr_addr(wr_addr),
		.wr_data(wr_data)
	);

	// --- ALU 연산 ---
	ALU alu (
		.operand1(operand1),
		.operand2(operand2),
		.shamt(shamt),
		.funct(ALUOp),
		.alu_result(alu_result)
	);

	// --- Memory ---
	MEM mem (
        .clk(clk),
        .rst(rst),
        .inst_addr(PC),
        .inst(inst),
        .mem_addr(alu_result),
        .MemWrite(MemWrite),
        .mem_write_data(rd_data2),
        .mem_read_data(mem_read_data)
    );
    
	// --- Writeback Logic ---
	always @(*) begin
		// 목적 레지스터 주소 설정
		wr_addr = (RegDst) ? rd : rt;
		if (Jump && SavePC)
			wr_addr = 5'd31;  // JAL: $ra

		// write-back 데이터 설정
		wr_data = (MemtoReg) ? mem_read_data : alu_result;
	end

	// --- PC Update Logic ---
	always @(*) begin
		PC_next = PC + 4;
		if (Jump)
			PC_next = {PC[31:28], immj, 2'b00};
		else if (JR)
			PC_next = rd_data1;
		else if (Branch && (alu_result != 0))
			PC_next = PC + 4 + (ext_imm << 2);
	end

	// --- PC Register ---
	always @(posedge clk or posedge rst) begin
		if (rst) PC <= 0;
		else PC <= PC_next;
	end

endmodule