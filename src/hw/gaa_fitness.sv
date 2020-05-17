`define SDRAM_FITNESS_START 25'h0080_0000  // halfway through sdram, leaves room for 8388608 edges
`define MAX_NUM_IDV 9'd128

module gaa_fitness(input logic clk,
		   input  logic       reset,

		   /* Avalon MM slave signals */
		   input  logic[ 3:0] hps_address,
		   input  logic       hps_chipselect,
		   input  logic       hps_write,
		   input  logic[ 7:0] hps_writedata,
	           input  logic       hps_read,
		   output logic[ 7:0] hps_readdata,
		   output logic       hps_waitrequest,
                   /* End Avalon MM slave signals */

		   /* Avalon MM master signals */
                   output logic[24:0] sdram_address,
                   output logic[ 1:0] sdram_byteenable_n,
	           output logic       sdram_chipselect,
		   output logic       sdram_read_n,
	   	   output logic       sdram_write_n,
		   output logic[15:0] sdram_writedata,
                   input  logic[15:0] sdram_readdata,
		   input  logic       sdram_readdatavalid,
                   input  logic       sdram_waitrequest
                   /* End Avalon MM master signals */
);

	// memory for up to 128 individuals with up to 8192 nodes each
	logic [12:0]       idv_addr_a;
	logic [12:0]       idv_addr_b;
	logic [15:0][7:0]  idv_data_a;
	logic [15:0][7:0]  idv_data_b;
	logic [15:0]       idv_wren_a;
	logic [15:0]       idv_wren_b;
	logic [15:0][7:0]  idv_q_a; 
	logic [15:0][7:0]  idv_q_b;
	idvmem_dualport_m10k idvmem[15:0](.address_a(idv_addr_a), .clock(clk), .data_a(idv_data_a), .wren_a(idv_wren_a), .q_a(idv_q_a),
					  .address_b(idv_addr_b), .data_b(idv_data_b), .wren_b(idv_wren_b), .q_b(idv_q_b)); 

	// memory to store fitness of each individual
	logic [ 6:0] fitness_addr;
	logic [31:0] fitness_data;
	logic        fitness_wren;
	logic [31:0] fitness_q;
	fitnessmem_m10k fitnessmem(.address(fitness_addr), .clock(clk), .data(fitness_data), .wren(fitness_wren), .q(fitness_q));

	logic [127:0] edge_diffs;
	logic [127:0][31:0] edge_weights;

	// number of total edges, nodes, and individuals in the problem
	logic [31:0] num_edges;
	logic [15:0] num_nodes;
	logic [15:0] num_idv;

	// counters
	logic [24:0] edge_cnt;
	logic [12:0] node_cnt;
	logic [ 4:0] idv_cnt;

	// for calculating fitness
	logic        calc_fitness;
	logic [15:0] n1;
	logic [15:0] n2;
	enum logic [8:0] { WAIT1, READ1, WAIT2, READ2, PARTITION_START, PARTITION_XOR, PARTITION_ADD, WRITE_RESULTS, WRITE_WAIT } fitness_state;

	logic [24:0] addr;
	logic [15:0] tempdata;

	always_ff @(posedge clk) begin
		if (reset) begin
			hps_waitrequest <= 1'b1;
			num_edges <= 32'd0;
			num_idv <= 16'd0;
			num_nodes <= 16'd0;
			edge_cnt <= 25'd0;
			node_cnt <= 13'd0;
			idv_cnt <= 5'd0;
			calc_fitness <= 1'b0;
			idv_wren_a <= 16'b0;
			idv_wren_b <= 16'b0;
			edge_weights <= 4096'b0;
		end
		else if (hps_chipselect && hps_write) begin
			case (hps_address)
				// Initialize constraining values:
				4'd0: begin  
					num_edges[7:0] <= hps_writedata;
				end
				4'd1: begin
					num_edges[15:8] <= hps_writedata;
				end
				4'd2: begin
					num_edges[23:16] <= hps_writedata;
				end
				4'd3: begin
					num_edges[31:24] <= hps_writedata;
				end
				4'd4: begin
					num_nodes[7:0] <= hps_writedata;
				end
				4'd5: begin
					num_nodes[15:8] <= hps_writedata;
				end
				4'd6: begin
					num_idv[7:0] <= hps_writedata;
				end
				4'd7: begin
					num_idv[15:8] <= hps_writedata;
				end

				// write new population
				4'd8: begin
					// while the row is empty, keep filling up the same row
					if (idv_cnt < 5'd15) begin
						idv_data_a[idv_cnt] <= hps_writedata;
						idv_cnt <= idv_cnt + 5'd1;
						idv_wren_a <= 16'b0;
					end
					// when we get to the end of the row, write the whole thing to block mem
					else if (idv_cnt == 5'd15) begin
						idv_data_a[idv_cnt] <= hps_writedata;
						idv_addr_a <= node_cnt;		
						idv_wren_a <= 16'b1;	
						idv_cnt <= 5'd0;
						if (node_cnt < (node_cnt - 1'd1)) begin
							// move to the next row
							node_cnt <= node_cnt + 13'd1;
						end
						else begin
							// idvmem is full
							node_cnt <= 13'd0;
							idv_wren_a <= 16'b0;
						end
					end
				end

				// start fitness calculations
				4'd9: begin
					calc_fitness <= 1'b1;
					idv_cnt <= 5'd0;
					node_cnt <= 13'd0;	
				end

				default: begin
					num_edges <= 32'd0;
                        		num_idv <= 16'd0;
	                	        num_nodes <= 16'd0;
        		                edge_cnt <= 25'd0;
        	        	        node_cnt <= 13'd0;
	                        	idv_cnt <= 5'd0;
                        		calc_fitness <= 1'b0;
				end
				
			endcase		
		end
	end  // end alwaysff

	// fitness calculations
	always_ff @(posedge clk) begin
		if (calc_fitness && !reset) begin
			case (fitness_state)
				WAIT1: begin
					// avalon mm master read protocol:
                                        // 1. assert address, byteenable and read after rising edge
                                        sdram_address <= edge_cnt;
                                        sdram_byteenable_n <= 2'b00;
                                        sdram_read_n <= 1'b0;
                                        sdram_chipselect <= 1'b1;
					
					// data is not ready, so assert waitrequest to this module's master
                                        hps_waitrequest <= 1'b1;

                                        // store the output of the sdram controller, even if it isn't ready
                                        n1 <= sdram_readdata;

                                        // once the sdram controller signals the data is ready, move to the
                                        // next state
                                        if (!sdram_waitrequest) begin
                                                fitness_state <= READ1;
                                        end
                                        else fitness_state <= WAIT1;
				end
				READ1: begin
					// data is ready
                                        n1 <= sdram_readdata;

					// begin read for second edge
					sdram_address <= edge_cnt + 1'b1;

					fitness_state <= WAIT2;
				end
				WAIT2: begin
					n2 <= sdram_readdata;
					if (!sdram_waitrequest) fitness_state <= READ2;
					else fitness_state <= WAIT2;
				end
				READ2: begin
					// data is ready
					n2 <= sdram_readdata;

                                        // deassert sdram signals and go to PARTITION1 state
                                        sdram_address <= 25'bx;
                                        sdram_byteenable_n <= 2'bx;
                                        sdram_read_n <= 1'b1;
                                        sdram_chipselect <= 1'b0;
                                        fitness_state <= PARTITION_START;
				end
				PARTITION_START: begin
					// read the partition at two nodes
					idv_addr_a <= n1[12:0];
					idv_wren_a <= 16'b0;
					idv_addr_b <= n2[12:0];
					idv_wren_b <= 16'b0;
					fitness_state <= PARTITION_XOR;
				end
				PARTITION_XOR: begin
					// store the xor of the read data:
					edge_diffs <= idv_q_a ^ idv_q_b;
				end
				PARTITION_ADD: begin
					edge_weights[0] <= edge_weights[0] + edge_diffs[0];
					edge_weights[1] <= edge_weights[1] + edge_diffs[1];
					edge_weights[2] <= edge_weights[2] + edge_diffs[2];
					edge_weights[3] <= edge_weights[3] + edge_diffs[3];
					edge_weights[4] <= edge_weights[4] + edge_diffs[4];
                                        edge_weights[5] <= edge_weights[5] + edge_diffs[5];
                                        edge_weights[6] <= edge_weights[6] + edge_diffs[6];
                                        edge_weights[7] <= edge_weights[7] + edge_diffs[7];
					edge_weights[8] <= edge_weights[8] + edge_diffs[8];
                                        edge_weights[9] <= edge_weights[9] + edge_diffs[9];
                                        edge_weights[10] <= edge_weights[10] + edge_diffs[10];
                                        edge_weights[11] <= edge_weights[11] + edge_diffs[11];
                                        edge_weights[12] <= edge_weights[12] + edge_diffs[12];
                                        edge_weights[13] <= edge_weights[13] + edge_diffs[13];
                                        edge_weights[14] <= edge_weights[14] + edge_diffs[14];
                                        edge_weights[15] <= edge_weights[15] + edge_diffs[15];
					edge_weights[16] <= edge_weights[16] + edge_diffs[16];
                                        edge_weights[17] <= edge_weights[17] + edge_diffs[17];
                                        edge_weights[18] <= edge_weights[18] + edge_diffs[18];
                                        edge_weights[19] <= edge_weights[19] + edge_diffs[19];
                                        edge_weights[20] <= edge_weights[20] + edge_diffs[20];
                                        edge_weights[21] <= edge_weights[21] + edge_diffs[21];
                                        edge_weights[22] <= edge_weights[22] + edge_diffs[22];
                                        edge_weights[23] <= edge_weights[23] + edge_diffs[23];
                                        edge_weights[24] <= edge_weights[24] + edge_diffs[24];
                                        edge_weights[25] <= edge_weights[25] + edge_diffs[25];
                                        edge_weights[26] <= edge_weights[26] + edge_diffs[26];
                                        edge_weights[27] <= edge_weights[27] + edge_diffs[27];
                                        edge_weights[28] <= edge_weights[28] + edge_diffs[28];
                                        edge_weights[29] <= edge_weights[29] + edge_diffs[29];
                                        edge_weights[30] <= edge_weights[30] + edge_diffs[30];
                                        edge_weights[31] <= edge_weights[31] + edge_diffs[31];
					edge_weights[32] <= edge_weights[32] + edge_diffs[32];
                                        edge_weights[33] <= edge_weights[33] + edge_diffs[33];
                                        edge_weights[34] <= edge_weights[34] + edge_diffs[34];
                                        edge_weights[35] <= edge_weights[35] + edge_diffs[35];
                                        edge_weights[36] <= edge_weights[36] + edge_diffs[36];
                                        edge_weights[37] <= edge_weights[37] + edge_diffs[37];
                                        edge_weights[38] <= edge_weights[38] + edge_diffs[38];
                                        edge_weights[39] <= edge_weights[39] + edge_diffs[39];
                                        edge_weights[40] <= edge_weights[40] + edge_diffs[40];
                                        edge_weights[41] <= edge_weights[41] + edge_diffs[41];
                                        edge_weights[42] <= edge_weights[42] + edge_diffs[42];
                                        edge_weights[43] <= edge_weights[43] + edge_diffs[43];
                                        edge_weights[44] <= edge_weights[44] + edge_diffs[44];
                                        edge_weights[45] <= edge_weights[45] + edge_diffs[45];
                                        edge_weights[46] <= edge_weights[46] + edge_diffs[46];
                                        edge_weights[47] <= edge_weights[47] + edge_diffs[47];
                                        edge_weights[48] <= edge_weights[48] + edge_diffs[48];
                                        edge_weights[49] <= edge_weights[49] + edge_diffs[49];
                                        edge_weights[50] <= edge_weights[50] + edge_diffs[50];
                                        edge_weights[51] <= edge_weights[51] + edge_diffs[51];
					edge_weights[52] <= edge_weights[52] + edge_diffs[52];
                                        edge_weights[53] <= edge_weights[53] + edge_diffs[53];
                                        edge_weights[54] <= edge_weights[54] + edge_diffs[54];
                                        edge_weights[55] <= edge_weights[55] + edge_diffs[55];
                                        edge_weights[56] <= edge_weights[56] + edge_diffs[56];
                                        edge_weights[57] <= edge_weights[57] + edge_diffs[57];
                                        edge_weights[58] <= edge_weights[58] + edge_diffs[58];
                                        edge_weights[59] <= edge_weights[59] + edge_diffs[59];
                                        edge_weights[60] <= edge_weights[60] + edge_diffs[60];
                                        edge_weights[61] <= edge_weights[61] + edge_diffs[61];
                                        edge_weights[62] <= edge_weights[62] + edge_diffs[62];
                                        edge_weights[63] <= edge_weights[63] + edge_diffs[63];
                                        edge_weights[64] <= edge_weights[64] + edge_diffs[64];
                                        edge_weights[65] <= edge_weights[65] + edge_diffs[65];
                                        edge_weights[66] <= edge_weights[66] + edge_diffs[66];
                                        edge_weights[67] <= edge_weights[67] + edge_diffs[67];
                                        edge_weights[68] <= edge_weights[68] + edge_diffs[68];
                                        edge_weights[69] <= edge_weights[69] + edge_diffs[69];
                                        edge_weights[70] <= edge_weights[70] + edge_diffs[70];
                                        edge_weights[71] <= edge_weights[71] + edge_diffs[71];
                                        edge_weights[72] <= edge_weights[72] + edge_diffs[72];
                                        edge_weights[73] <= edge_weights[73] + edge_diffs[73];
                                        edge_weights[74] <= edge_weights[74] + edge_diffs[74];
                                        edge_weights[75] <= edge_weights[75] + edge_diffs[75];
                                        edge_weights[76] <= edge_weights[76] + edge_diffs[76];
                                        edge_weights[77] <= edge_weights[77] + edge_diffs[77];
                                        edge_weights[78] <= edge_weights[78] + edge_diffs[78];
                                        edge_weights[79] <= edge_weights[79] + edge_diffs[79];
                                        edge_weights[80] <= edge_weights[80] + edge_diffs[80];
                                        edge_weights[81] <= edge_weights[81] + edge_diffs[81];
                                        edge_weights[82] <= edge_weights[82] + edge_diffs[82];
                                        edge_weights[83] <= edge_weights[83] + edge_diffs[83];
                                        edge_weights[84] <= edge_weights[84] + edge_diffs[84];
                                        edge_weights[85] <= edge_weights[85] + edge_diffs[85];
                                        edge_weights[86] <= edge_weights[86] + edge_diffs[86];
                                        edge_weights[87] <= edge_weights[87] + edge_diffs[87];
                                        edge_weights[88] <= edge_weights[88] + edge_diffs[88];
                                        edge_weights[89] <= edge_weights[89] + edge_diffs[89];
                                        edge_weights[90] <= edge_weights[90] + edge_diffs[90];
                                        edge_weights[91] <= edge_weights[91] + edge_diffs[91];
					edge_weights[92] <= edge_weights[92] + edge_diffs[92];
                                        edge_weights[93] <= edge_weights[93] + edge_diffs[93];
                                        edge_weights[94] <= edge_weights[94] + edge_diffs[94];
                                        edge_weights[95] <= edge_weights[95] + edge_diffs[95];
                                        edge_weights[96] <= edge_weights[96] + edge_diffs[96];
                                        edge_weights[97] <= edge_weights[97] + edge_diffs[97];
                                        edge_weights[98] <= edge_weights[98] + edge_diffs[98];
                                        edge_weights[99] <= edge_weights[99] + edge_diffs[99];
                                        edge_weights[100] <= edge_weights[100] + edge_diffs[100];
                                        edge_weights[101] <= edge_weights[101] + edge_diffs[101];
                                        edge_weights[102] <= edge_weights[102] + edge_diffs[102];
                                        edge_weights[103] <= edge_weights[103] + edge_diffs[103];
                                        edge_weights[104] <= edge_weights[104] + edge_diffs[104];
                                        edge_weights[105] <= edge_weights[105] + edge_diffs[105];
                                        edge_weights[106] <= edge_weights[106] + edge_diffs[106];
                                        edge_weights[107] <= edge_weights[107] + edge_diffs[107];
                                        edge_weights[108] <= edge_weights[108] + edge_diffs[108];
                                        edge_weights[109] <= edge_weights[109] + edge_diffs[109];
                                        edge_weights[110] <= edge_weights[110] + edge_diffs[110];
                                        edge_weights[111] <= edge_weights[111] + edge_diffs[111];
                                        edge_weights[112] <= edge_weights[112] + edge_diffs[112];
                                        edge_weights[113] <= edge_weights[113] + edge_diffs[113];
                                        edge_weights[114] <= edge_weights[114] + edge_diffs[114];
                                        edge_weights[115] <= edge_weights[115] + edge_diffs[115];
                                        edge_weights[116] <= edge_weights[116] + edge_diffs[116];
                                        edge_weights[117] <= edge_weights[117] + edge_diffs[117];
                                        edge_weights[118] <= edge_weights[118] + edge_diffs[118];
                                        edge_weights[119] <= edge_weights[119] + edge_diffs[119];
                                        edge_weights[120] <= edge_weights[120] + edge_diffs[120];
                                        edge_weights[122] <= edge_weights[121] + edge_diffs[121];
					edge_weights[123] <= edge_weights[123] + edge_diffs[123];
                                        edge_weights[124] <= edge_weights[124] + edge_diffs[124];
                                        edge_weights[125] <= edge_weights[125] + edge_diffs[125];
                                        edge_weights[126] <= edge_weights[126] + edge_diffs[126];
                                        edge_weights[127] <= edge_weights[127] + edge_diffs[127];

					if ((edge_cnt >> 1) + 1 >= num_edges) begin
						edge_cnt <= 0;
						fitness_state <= WRITE_RESULTS;
						idv_cnt <= 0;
					end
					else begin
						edge_cnt <= edge_cnt + 2;
						fitness_state <= WAIT1;
					end
				end
				WRITE_RESULTS: begin
					if (idv_cnt < num_idv) begin
						sdram_write_n <= 1'b0;
						sdram_address <= `SDRAM_FITNESS_START + idv_cnt;
						sdram_writedata <= edge_weights[idv_cnt][31:16];
						fitness_state <= WRITE_WAIT;
					end
					else begin
						hps_waitrequest <= 1'b0;
						calc_fitness <= 1'b0;
						idv_cnt <= 5'd0;
						fitness_state <= WAIT1;
					end
				end
				WRITE_WAIT: begin
					if (sdram_writedata) fitness_state <= WRITE_WAIT;
					else begin
						idv_cnt <= idv_cnt + 1;
						fitness_state <= WRITE_RESULTS;
					end
				end
				default: begin
					fitness_state <= WAIT1;	
					calc_fitness <= 1'b0;
					idv_cnt <= 5'd0;
				end
			endcase
		end
	end  // end always ff

endmodule


