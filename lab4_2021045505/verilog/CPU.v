`timescale 1ns / 1ps


module CPU(
	input		clk,
	input		rst,
	output 		halt
	);
	
	// Split the instructions
	// Instruction-related wires
	wire [5:0]		opcode;
	wire [4:0]		rs;
	wire [4:0]		rt;
	wire [4:0]		rd;
	wire [4:0]		shamt;
	wire [5:0]		funct;
	wire [15:0]		immi;
	wire [25:0]		immj;

	// Control-related wires
    wire 			MemRead;
    wire 			MemWrite;
    wire 			PCWrite;
    wire 			PCWriteCond;
    wire 			IRWrite;
    wire 			RegWrite;
    wire 			SavePC;
    wire 			IorD;
    wire 			MemtoReg;
    wire 			RegDst;
    wire 			ALUSrcA;
    wire [1:0] 		ALUSrcB;
    wire [3:0] 		ALUOp;
    wire [1:0] 		PCSource;
    wire 			SignExtend;




	// Sign extend the immediate
	wire [31:0]		ext_imm = (SignExtend) ? {{16{immi[15]}}, immi} : {16'b0, immi};

	// RF-related wires
	wire [31:0]		rs_data;
	wire [31:0]		rt_data;
	reg [4:0]		wr_addr;
	reg [31:0]		wr_data;

	// MEM-related regs
	wire [31:0]		mem_addr;
	wire [31:0]		mem_data;

	// ALU-related wires
	wire [31:0]		operand1;
	wire [31:0]		operand2;
	wire [31:0]		alu_result;

	// Define PC
	reg [31:0]	PC;
	reg [31:0]	PC_next;

	//registers
	reg	[31:0]		A;
	reg	[31:0]		B;
	reg	[31:0]		ALUOut;
	reg	[31:0]		MDR;
	reg	[31:0]		IR;

	// Define the wires

	assign halt				= (IR == 32'b0);

	// --- Control Unit 연결 ---
	CTRL ctrl (
		.clk(clk),
		.rst(rst),
		.opcode(opcode),
		.funct(funct),
		.MemRead(MemRead),
		.MemWrite(MemWrite),
		.PCWrite(PCWrite),
		.PCWriteCond(PCWriteCond),
		.IRWrite(IRWrite),
		.RegWrite(RegWrite),
		.SavePC(SavePC),
		.IorD(IorD),
		.MemtoReg(MemtoReg),
		.RegDst(RegDst),
		.ALUSrcA(ALUSrcA),
		.ALUSrcB(ALUSrcB),
		.ALUOp(ALUOp),
		.PCSource(PCSource),
		.SignExtend(SignExtend)
	);

	// --- Register File ---
	RF rf (
		.clk(clk),
		.rst(rst),
		.rd_addr1(rs),
		.rd_addr2(rt),
		.rd_data1(rs_data),
		.rd_data2(rt_data),
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
        .mem_addr(mem_addr),
        .MemWrite(MemWrite),
        .mem_write_data(B),
        .mem_read_data(mem_data)
    );

	assign opcode = IR[31:26];
	assign rs = IR[25:21];
	assign rt = IR[20:16];
	assign rd = IR[15:11];
	assign shamt = IR[10:6];
	assign funct = IR[5:0];
	assign immi = IR[15:0];
	assign immj = IR[25:0];

	// Memory address selection
    assign mem_addr = IorD ? ALUOut : PC;
    
	// ALU operand selection
    assign operand1 = ALUSrcA ? A : PC;
    assign operand2 = (ALUSrcB == 2'd0) ? B :
                      (ALUSrcB == 2'd1) ? 32'd4 :
                      (ALUSrcB == 2'd2) ? ext_imm :
                      (ALUSrcB == 2'd3) ? (ext_imm << 2) : 32'd0;
    
	// --- Writeback Logic ---
	always @(*) begin
		// 목적 레지스터 주소 설정
		wr_addr = (RegDst) ? rd : rt;
		if (SavePC)
			wr_addr = 5'd31;  // JAL: $ra
	end

	// Write data selection
	always @(*) begin
		if (MemtoReg)
			wr_data = MDR;
		else
			wr_data = ALUOut;
	end

	// PC_next selection
	always @(*) begin
		if (PCSource == 2'd0)
			PC_next = alu_result;
		else if (PCSource == 2'd1)
			PC_next = ALUOut;
		else if (PCSource == 2'd2)
			PC_next = {PC[31:28], immj, 2'b00};
		else if (PCSource == 2'd3)
			PC_next = rs_data;
		else
			PC_next = 32'd0;
	end

	// after 1 tick
	always @(posedge clk or posedge rst) begin
		if (rst) begin
			PC <= 0;
			ALUOut <= 0;
			A <= 0;
			B <= 0;
			MDR <= 0;
			IR <= 0;
		end else begin
			if (IRWrite) IR <= mem_data;
			MDR <= mem_data;
			ALUOut <= alu_result;
			A <= rs_data;
			B <= rt_data;
			if (PCWrite || (PCWriteCond && alu_result != 0))
				PC <= PC_next;
		end
	end

endmodule