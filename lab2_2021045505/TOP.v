`timescale 1ns / 100ps

module TOP(
    input clk,
    input rst,
    
    // Read-related ports
    input [4:0] rd_addr1,
    input [4:0] rd_addr2,
    
    // Write-related ports
    input RegWrite,
    input [4:0] wr_addr,
    
    // ALU ctrl and output
    input [4:0] shamt,
    input [3:0] funct,
    output [31:0] rd_data1,
    output [31:0] rd_data2,
    output [31:0] wr_data
    );
    
	// FIXME
	// Instantiate modules and connect them!
	
	RF rf_u(clk, rst, rd_addr1, rd_addr2, rd_data1, rd_data2, RegWrite, wr_addr, wr_data);
    ALU alu_u(rd_data1, rd_data2, shamt, funct, wr_data);
    
endmodule