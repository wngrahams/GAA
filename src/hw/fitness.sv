/*
 * Avalon memory-mapped peripheral that generates VGA
 *
 * Stephen A. Edwards
 * Columbia University
 */

module fitness (input logic        clk,
	        input logic 	   reset,
		input logic [7:0]  writedata,
		input logic 	   write,
		input 		   chipselect,
		input logic [2:0]  address);

   logic [7:0] 	   background_r, background_g, background_b;

   always_ff @(posedge clk)
     if (reset) begin
	background_r <= 8'h0;
	background_g <= 8'h0;
	background_b <= 8'h80;
     end else if (chipselect && write)
       case (address)
	 3'h0 : background_r <= writedata;
	 3'h1 : background_g <= writedata;
	 3'h2 : background_b <= writedata;
       endcase

endmodule

