
module gaa_fitness(input logic clk,
		   input  logic       reset,

		   /* Avalon MM slave signals */
		   input  logic[ 1:0] hps_address,
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
                   input  logic[15:0] sdram_readdata,
		   input  logic       sdram_readdatavalid,
                   input  logic       sdram_waitrequest
                   /* End Avalon MM master signals */
);

	//logic [7:0] p1, p2, p1xorp2;
	logic [24:0] read_addr;
	enum logic [2:0] {READY, WAIT, DONE} read_state;

	// this should be unnecessary, just use always comb once you have proper addresses and data widths
	always_ff @(posedge clk) begin
		if (hps_chipselect && hps_read) begin
			case (hps_address)
				2'b00:   read_addr <= 25'd0;
				2'b01:   read_addr <= 25'd2;
				2'b10:   read_addr <= 25'd4;
				default: read_addr <= 25'd0;
			endcase
		end
	end

	always_ff @(posedge clk) begin
		case (read_state)
			/*
			READY: begin
				
			end

			START_READ: begin
				sdram_read_n <= 1'b0;
				read_state <= WAIT;
			end
			*/

			READY: begin
				if (hps_chipselect && hps_read) begin
					sdram_chipselect <= 1'b1;
					read_state <= WAIT;
				end
				else read_state <= READY;
			end

			WAIT: begin
				if (sdram_readdatavalid && !sdram_waitrequest) read_state <= DONE;
				else read_state <= WAIT;
			end

			DONE: begin
				hps_readdata <= sdram_readdata[7:0];
				read_state <= WAIT;
			end
			default: read_state <= READY;
		endcase
	end

	always_comb begin
                //p1xorp2 = p1 ^ p2;
                //hps_waitrequest = sdram_waitrequest;
		//sdram_address = read_addr;
		//sdram_read_n = ~hps_read;
		if (reset) hps_waitrequest = 1'b1;              // assert waitrequest during reset to avoid system lockup
		else       hps_waitrequest = sdram_waitrequest;

		sdram_address = read_addr;
		sdram_read_n = ~hps_read;	
        end

	/*
	always_ff @(posedge clk) begin
		if (reset) begin
			p1      <= 8'h00;
			p2      <= 8'h00;
		end
		else if (chipselect && write) begin
			case (address)
				2'b00: p1 <= writedata[7:0];
				2'b01: p2 <= writedata[7:0];
				default: ;
			endcase
		end
		else if (chipselect && read) begin
			case (address)
				2'b00: readdata <= p1[7:0];
				2'b01: readdata <= p2[7:0];
				2'b10: readdata <= p1xorp2[7:0];
				default: readdata <= 8'dx;
			endcase
		end
	end
	*/		
endmodule

