`define SDRAM_FITNESS_START 25'h0080_0000  // halfway through sdram, leaves room for 8388608 edges
`define MAX_NUM_IDV 8'd128

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
	logic [ 7:0] num_idv;

	// counters
	logic [24:0] edge_cnt;
	logic [12:0] node_cnt;
	logic [ 7:0] idv_cnt;

	// for calculating fitness
	logic        calc_fitness;
	logic [15:0] n1;
	logic [15:0] n2;
	enum logic [8:0] { WAIT1, READ1, WAIT2, READ2, PARTITION_START, PARTITION_XOR, PARTITION_READ, WRITE_RESULTS, WRITE_WAIT } fitness_state;

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
			idv_cnt <= 8'd0;
			calc_fitness <= 1'b0;
			idv_wren_a <= 16'b0;
			idv_wren_b <= 16'b0;
			edge_weights <= 4096'b0;
		end
		// process and calculate fitness of given individuals written from ioctl call
		else if (calc_fitness) begin
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
					fitness_state <= PARTITION_READ;
				end
				PARTITION_READ: begin
					// we need one clock cycle to read from the m10k 
					fitness_state <= PARTITION_XOR;
				end
				PARTITION_XOR: begin

					edge_weights[0] <= edge_weights[0] + (idv_q_a[0][0] ^ idv_q_b[0][0]);
					edge_weights[1] <= edge_weights[1] + (idv_q_a[0][1] ^ idv_q_b[0][1]);
					edge_weights[2] <= edge_weights[2] + (idv_q_a[0][2] ^ idv_q_b[0][2]);
					edge_weights[3] <= edge_weights[3] + (idv_q_a[0][3] ^ idv_q_b[0][3]);
					edge_weights[4] <= edge_weights[4] + (idv_q_a[0][4] ^ idv_q_b[0][4]);
					edge_weights[5] <= edge_weights[5] + (idv_q_a[0][5] ^ idv_q_b[0][5]);
					edge_weights[6] <= edge_weights[6] + (idv_q_a[0][6] ^ idv_q_b[0][6]);
					edge_weights[7] <= edge_weights[7] + (idv_q_a[0][7] ^ idv_q_b[0][7]);
					edge_weights[8] <= edge_weights[8] + (idv_q_a[1][0] ^ idv_q_b[1][0]);
					edge_weights[9] <= edge_weights[9] + (idv_q_a[1][1] ^ idv_q_b[1][1]);
					edge_weights[10] <= edge_weights[10] + (idv_q_a[1][2] ^ idv_q_b[1][2]);
					edge_weights[11] <= edge_weights[11] + (idv_q_a[1][3] ^ idv_q_b[1][3]);
					edge_weights[12] <= edge_weights[12] + (idv_q_a[1][4] ^ idv_q_b[1][4]);
					edge_weights[13] <= edge_weights[13] + (idv_q_a[1][5] ^ idv_q_b[1][5]);
					edge_weights[14] <= edge_weights[14] + (idv_q_a[1][6] ^ idv_q_b[1][6]);
					edge_weights[15] <= edge_weights[15] + (idv_q_a[1][7] ^ idv_q_b[1][7]);
					edge_weights[16] <= edge_weights[16] + (idv_q_a[2][0] ^ idv_q_b[2][0]);
					edge_weights[17] <= edge_weights[17] + (idv_q_a[2][1] ^ idv_q_b[2][1]);
					edge_weights[18] <= edge_weights[18] + (idv_q_a[2][2] ^ idv_q_b[2][2]);
					edge_weights[19] <= edge_weights[19] + (idv_q_a[2][3] ^ idv_q_b[2][3]);
					edge_weights[20] <= edge_weights[20] + (idv_q_a[2][4] ^ idv_q_b[2][4]);
					edge_weights[21] <= edge_weights[21] + (idv_q_a[2][5] ^ idv_q_b[2][5]);
					edge_weights[22] <= edge_weights[22] + (idv_q_a[2][6] ^ idv_q_b[2][6]);
					edge_weights[23] <= edge_weights[23] + (idv_q_a[2][7] ^ idv_q_b[2][7]);
					edge_weights[24] <= edge_weights[24] + (idv_q_a[3][0] ^ idv_q_b[3][0]);
					edge_weights[25] <= edge_weights[25] + (idv_q_a[3][1] ^ idv_q_b[3][1]);
					edge_weights[26] <= edge_weights[26] + (idv_q_a[3][2] ^ idv_q_b[3][2]);
					edge_weights[27] <= edge_weights[27] + (idv_q_a[3][3] ^ idv_q_b[3][3]);
					edge_weights[28] <= edge_weights[28] + (idv_q_a[3][4] ^ idv_q_b[3][4]);
					edge_weights[29] <= edge_weights[29] + (idv_q_a[3][5] ^ idv_q_b[3][5]);
					edge_weights[30] <= edge_weights[30] + (idv_q_a[3][6] ^ idv_q_b[3][6]);
					edge_weights[31] <= edge_weights[31] + (idv_q_a[3][7] ^ idv_q_b[3][7]);
					edge_weights[32] <= edge_weights[32] + (idv_q_a[4][0] ^ idv_q_b[4][0]);
					edge_weights[33] <= edge_weights[33] + (idv_q_a[4][1] ^ idv_q_b[4][1]);
					edge_weights[34] <= edge_weights[34] + (idv_q_a[4][2] ^ idv_q_b[4][2]);
					edge_weights[35] <= edge_weights[35] + (idv_q_a[4][3] ^ idv_q_b[4][3]);
					edge_weights[36] <= edge_weights[36] + (idv_q_a[4][4] ^ idv_q_b[4][4]);
					edge_weights[37] <= edge_weights[37] + (idv_q_a[4][5] ^ idv_q_b[4][5]);
					edge_weights[38] <= edge_weights[38] + (idv_q_a[4][6] ^ idv_q_b[4][6]);
					edge_weights[39] <= edge_weights[39] + (idv_q_a[4][7] ^ idv_q_b[4][7]);
					edge_weights[40] <= edge_weights[40] + (idv_q_a[5][0] ^ idv_q_b[5][0]);
					edge_weights[41] <= edge_weights[41] + (idv_q_a[5][1] ^ idv_q_b[5][1]);
					edge_weights[42] <= edge_weights[42] + (idv_q_a[5][2] ^ idv_q_b[5][2]);
					edge_weights[43] <= edge_weights[43] + (idv_q_a[5][3] ^ idv_q_b[5][3]);
					edge_weights[44] <= edge_weights[44] + (idv_q_a[5][4] ^ idv_q_b[5][4]);
					edge_weights[45] <= edge_weights[45] + (idv_q_a[5][5] ^ idv_q_b[5][5]);
					edge_weights[46] <= edge_weights[46] + (idv_q_a[5][6] ^ idv_q_b[5][6]);
					edge_weights[47] <= edge_weights[47] + (idv_q_a[5][7] ^ idv_q_b[5][7]);
					edge_weights[48] <= edge_weights[48] + (idv_q_a[6][0] ^ idv_q_b[6][0]);
					edge_weights[49] <= edge_weights[49] + (idv_q_a[6][1] ^ idv_q_b[6][1]);
					edge_weights[50] <= edge_weights[50] + (idv_q_a[6][2] ^ idv_q_b[6][2]);
					edge_weights[51] <= edge_weights[51] + (idv_q_a[6][3] ^ idv_q_b[6][3]);
					edge_weights[52] <= edge_weights[52] + (idv_q_a[6][4] ^ idv_q_b[6][4]);
					edge_weights[53] <= edge_weights[53] + (idv_q_a[6][5] ^ idv_q_b[6][5]);
					edge_weights[54] <= edge_weights[54] + (idv_q_a[6][6] ^ idv_q_b[6][6]);
					edge_weights[55] <= edge_weights[55] + (idv_q_a[6][7] ^ idv_q_b[6][7]);
					edge_weights[56] <= edge_weights[56] + (idv_q_a[7][0] ^ idv_q_b[7][0]);
					edge_weights[57] <= edge_weights[57] + (idv_q_a[7][1] ^ idv_q_b[7][1]);
					edge_weights[58] <= edge_weights[58] + (idv_q_a[7][2] ^ idv_q_b[7][2]);
					edge_weights[59] <= edge_weights[59] + (idv_q_a[7][3] ^ idv_q_b[7][3]);
					edge_weights[60] <= edge_weights[60] + (idv_q_a[7][4] ^ idv_q_b[7][4]);
					edge_weights[61] <= edge_weights[61] + (idv_q_a[7][5] ^ idv_q_b[7][5]);
					edge_weights[62] <= edge_weights[62] + (idv_q_a[7][6] ^ idv_q_b[7][6]);
					edge_weights[63] <= edge_weights[63] + (idv_q_a[7][7] ^ idv_q_b[7][7]);
					edge_weights[64] <= edge_weights[64] + (idv_q_a[8][0] ^ idv_q_b[8][0]);
					edge_weights[65] <= edge_weights[65] + (idv_q_a[8][1] ^ idv_q_b[8][1]);
					edge_weights[66] <= edge_weights[66] + (idv_q_a[8][2] ^ idv_q_b[8][2]);
					edge_weights[67] <= edge_weights[67] + (idv_q_a[8][3] ^ idv_q_b[8][3]);
					edge_weights[68] <= edge_weights[68] + (idv_q_a[8][4] ^ idv_q_b[8][4]);
					edge_weights[69] <= edge_weights[69] + (idv_q_a[8][5] ^ idv_q_b[8][5]);
					edge_weights[70] <= edge_weights[70] + (idv_q_a[8][6] ^ idv_q_b[8][6]);
					edge_weights[71] <= edge_weights[71] + (idv_q_a[8][7] ^ idv_q_b[8][7]);
					edge_weights[72] <= edge_weights[72] + (idv_q_a[9][0] ^ idv_q_b[9][0]);
					edge_weights[73] <= edge_weights[73] + (idv_q_a[9][1] ^ idv_q_b[9][1]);
					edge_weights[74] <= edge_weights[74] + (idv_q_a[9][2] ^ idv_q_b[9][2]);
					edge_weights[75] <= edge_weights[75] + (idv_q_a[9][3] ^ idv_q_b[9][3]);
					edge_weights[76] <= edge_weights[76] + (idv_q_a[9][4] ^ idv_q_b[9][4]);
					edge_weights[77] <= edge_weights[77] + (idv_q_a[9][5] ^ idv_q_b[9][5]);
					edge_weights[78] <= edge_weights[78] + (idv_q_a[9][6] ^ idv_q_b[9][6]);
					edge_weights[79] <= edge_weights[79] + (idv_q_a[9][7] ^ idv_q_b[9][7]);
					edge_weights[80] <= edge_weights[80] + (idv_q_a[10][0] ^ idv_q_b[10][0]);
					edge_weights[81] <= edge_weights[81] + (idv_q_a[10][1] ^ idv_q_b[10][1]);
					edge_weights[82] <= edge_weights[82] + (idv_q_a[10][2] ^ idv_q_b[10][2]);
					edge_weights[83] <= edge_weights[83] + (idv_q_a[10][3] ^ idv_q_b[10][3]);
					edge_weights[84] <= edge_weights[84] + (idv_q_a[10][4] ^ idv_q_b[10][4]);
					edge_weights[85] <= edge_weights[85] + (idv_q_a[10][5] ^ idv_q_b[10][5]);
					edge_weights[86] <= edge_weights[86] + (idv_q_a[10][6] ^ idv_q_b[10][6]);
					edge_weights[87] <= edge_weights[87] + (idv_q_a[10][7] ^ idv_q_b[10][7]);
					edge_weights[88] <= edge_weights[88] + (idv_q_a[11][0] ^ idv_q_b[11][0]);
					edge_weights[89] <= edge_weights[89] + (idv_q_a[11][1] ^ idv_q_b[11][1]);
					edge_weights[90] <= edge_weights[90] + (idv_q_a[11][2] ^ idv_q_b[11][2]);
					edge_weights[91] <= edge_weights[91] + (idv_q_a[11][3] ^ idv_q_b[11][3]);
					edge_weights[92] <= edge_weights[92] + (idv_q_a[11][4] ^ idv_q_b[11][4]);
					edge_weights[93] <= edge_weights[93] + (idv_q_a[11][5] ^ idv_q_b[11][5]);
					edge_weights[94] <= edge_weights[94] + (idv_q_a[11][6] ^ idv_q_b[11][6]);
					edge_weights[95] <= edge_weights[95] + (idv_q_a[11][7] ^ idv_q_b[11][7]);
					edge_weights[96] <= edge_weights[96] + (idv_q_a[12][0] ^ idv_q_b[12][0]);
					edge_weights[97] <= edge_weights[97] + (idv_q_a[12][1] ^ idv_q_b[12][1]);
					edge_weights[98] <= edge_weights[98] + (idv_q_a[12][2] ^ idv_q_b[12][2]);
					edge_weights[99] <= edge_weights[99] + (idv_q_a[12][3] ^ idv_q_b[12][3]);
					edge_weights[100] <= edge_weights[100] + (idv_q_a[12][4] ^ idv_q_b[12][4]);
					edge_weights[101] <= edge_weights[101] + (idv_q_a[12][5] ^ idv_q_b[12][5]);
					edge_weights[102] <= edge_weights[102] + (idv_q_a[12][6] ^ idv_q_b[12][6]);
					edge_weights[103] <= edge_weights[103] + (idv_q_a[12][7] ^ idv_q_b[12][7]);
					edge_weights[104] <= edge_weights[104] + (idv_q_a[13][0] ^ idv_q_b[13][0]);
					edge_weights[105] <= edge_weights[105] + (idv_q_a[13][1] ^ idv_q_b[13][1]);
					edge_weights[106] <= edge_weights[106] + (idv_q_a[13][2] ^ idv_q_b[13][2]);
					edge_weights[107] <= edge_weights[107] + (idv_q_a[13][3] ^ idv_q_b[13][3]);
					edge_weights[108] <= edge_weights[108] + (idv_q_a[13][4] ^ idv_q_b[13][4]);
					edge_weights[109] <= edge_weights[109] + (idv_q_a[13][5] ^ idv_q_b[13][5]);
					edge_weights[110] <= edge_weights[110] + (idv_q_a[13][6] ^ idv_q_b[13][6]);
					edge_weights[111] <= edge_weights[111] + (idv_q_a[13][7] ^ idv_q_b[13][7]);
					edge_weights[112] <= edge_weights[112] + (idv_q_a[14][0] ^ idv_q_b[14][0]);
					edge_weights[113] <= edge_weights[113] + (idv_q_a[14][1] ^ idv_q_b[14][1]);
					edge_weights[114] <= edge_weights[114] + (idv_q_a[14][2] ^ idv_q_b[14][2]);
					edge_weights[115] <= edge_weights[115] + (idv_q_a[14][3] ^ idv_q_b[14][3]);
					edge_weights[116] <= edge_weights[116] + (idv_q_a[14][4] ^ idv_q_b[14][4]);
					edge_weights[117] <= edge_weights[117] + (idv_q_a[14][5] ^ idv_q_b[14][5]);
					edge_weights[118] <= edge_weights[118] + (idv_q_a[14][6] ^ idv_q_b[14][6]);
					edge_weights[119] <= edge_weights[119] + (idv_q_a[14][7] ^ idv_q_b[14][7]);
					edge_weights[120] <= edge_weights[120] + (idv_q_a[15][0] ^ idv_q_b[15][0]);
					edge_weights[121] <= edge_weights[121] + (idv_q_a[15][1] ^ idv_q_b[15][1]);
					edge_weights[122] <= edge_weights[122] + (idv_q_a[15][2] ^ idv_q_b[15][2]);
					edge_weights[123] <= edge_weights[123] + (idv_q_a[15][3] ^ idv_q_b[15][3]);
					edge_weights[124] <= edge_weights[124] + (idv_q_a[15][4] ^ idv_q_b[15][4]);
					edge_weights[125] <= edge_weights[125] + (idv_q_a[15][5] ^ idv_q_b[15][5]);
					edge_weights[126] <= edge_weights[126] + (idv_q_a[15][6] ^ idv_q_b[15][6]);
					edge_weights[127] <= edge_weights[127] + (idv_q_a[15][7] ^ idv_q_b[15][7]);

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
						sdram_chipselect <= 1'b1;
						sdram_byteenable_n <= 2'b00;
						sdram_address <= `SDRAM_FITNESS_START + idv_cnt;
						if (edge_weights[idv_cnt][31:16] == 16'b0) begin
							sdram_writedata <= edge_weights[idv_cnt][15:0];
						end
						else begin
							sdram_writedata <= 16'hFFFF;
						end
						fitness_state <= WRITE_WAIT;
					end
					else begin
						sdram_write_n <= 1'b1;
						
						hps_waitrequest <= 1'b0;
						calc_fitness <= 1'b0;
						idv_cnt <= 8'd0;
						fitness_state <= WAIT1;
					end
				end
				WRITE_WAIT: begin
					if (sdram_waitrequest) fitness_state <= WRITE_WAIT;
					else begin
						idv_cnt <= idv_cnt + 1;
						fitness_state <= WRITE_RESULTS;
					end
				end
				default: begin
					fitness_state <= WAIT1;	
					idv_cnt <= 8'd0;
				end
			endcase
		end
		// ioctl write behavior:
		else if (hps_chipselect && hps_write) begin

			hps_waitrequest <= 0;

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

				// write new population -- currently only works for exactly 128 individuals
				4'd7: begin
					// while the row is empty, keep filling up the same row
					if (idv_cnt < 8'd15) begin
						idv_data_a[idv_cnt] <= hps_writedata;
						idv_cnt <= idv_cnt + 8'd1;
						idv_wren_a <= 16'b0;
					end
					// when we get to the end of the row, write the whole thing to block mem
					else if (idv_cnt == 8'd15) begin
						idv_data_a[idv_cnt] <= hps_writedata;
						idv_addr_a <= node_cnt;		
						idv_wren_a <= 16'hFFFF;	
						idv_cnt <= 8'd0;
						if (node_cnt < (num_nodes - 1'b1)) begin
							// move to the next row
							node_cnt <= node_cnt + 13'd1;
						end
						else begin
							// idvmem is full
							node_cnt <= 13'd0;
						end
					end
				end

				// start fitness calculations
				4'd8: begin
					calc_fitness <= 1'b1;
					idv_cnt <= 8'd0;
					node_cnt <= 13'd0;	
					idv_wren_a <= 16'b0;
				end

				default: begin
					num_edges <= 32'd0;
                        		num_idv <= 16'd0;
	                	        num_nodes <= 16'd0;
        		                edge_cnt <= 25'd0;
        	        	        node_cnt <= 13'd0;
	                        	idv_cnt <= 8'd0;
                        		calc_fitness <= 1'b0;
					idv_wren_a <= 16'b0;
				end
				
			endcase		
		end
		else begin
			idv_wren_a <= 16'b0;
			idv_wren_b <= 16'b0;
		end
	end  // end alwaysff

endmodule

