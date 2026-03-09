module dac_interface_enhanced (
    input wire clk_120m,
    input wire reset_n,
    input wire [7:0] chirp_data,
    input wire chirp_valid,
    output reg [7:0] dac_data,
    output wire dac_clk,
	output wire dac_sleep
);

// Register DAC data to meet timing
always @(posedge clk_120m or negedge reset_n) begin
    if (!reset_n) begin
        dac_data <= 8'd128;  // Center value
    end else if (chirp_valid) begin
        dac_data <= chirp_data;
    end else begin
        dac_data <= 8'd128;  // Default to center when no chirp
    end
end

// DAC clock is same as input clock (120MHz)
assign dac_clk = clk_120m;
assign dac_sleep = 1'b0;

endmodule