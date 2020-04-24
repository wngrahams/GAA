module gaa_fitness(input logic clk,
		   input logic reset,

		   input logic[1:0] address,
		   input logic chipselect,
		   input logic write,
		   input logic[7:0] writedata,
	           input logic read,
		   output logic[7:0] readdata);

	logic [7:0] p1, p2, p1xorp2;

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

	always_comb begin
		p1xorp2 = p1 ^ p2;
	end
			
endmodule

