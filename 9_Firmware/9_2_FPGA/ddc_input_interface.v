`timescale 1ns / 1ps
// ddc_input_interface.v
module ddc_input_interface (
    input wire clk,           // 100MHz
    input wire reset_n,
    
    // DDC Input (18-bit)
    input wire signed [17:0] ddc_i,
    input wire signed [17:0] ddc_q,
    input wire valid_i,
    input wire valid_q,
    
    // Scaled output (16-bit)
    output reg signed [15:0] adc_i,
    output reg signed [15:0] adc_q,
    output reg adc_valid,
    
    // Status
    output wire data_sync_error
);

// Synchronize valid signals
reg valid_i_reg, valid_q_reg;
reg valid_sync;

always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
        valid_i_reg <= 1'b0;
        valid_q_reg <= 1'b0;
        valid_sync <= 1'b0;
        adc_valid <= 1'b0;
    end else begin
        valid_i_reg <= valid_i;
        valid_q_reg <= valid_q;
        
        // Require both I and Q valid simultaneously
        valid_sync <= valid_i_reg && valid_q_reg;
        adc_valid <= valid_sync;
    end
end

// Scale 18-bit to 16-bit with rounding
// Option: Keep most significant 16 bits with rounding
always @(posedge clk) begin
    if (valid_sync) begin
        // Round to nearest: add 0.5 LSB before truncation
        adc_i <= ddc_i[17:2] + ddc_i[1];  // Rounding
        adc_q <= ddc_q[17:2] + ddc_q[1];  // Rounding
    end
end

// Error detection
assign data_sync_error = (valid_i_reg ^ valid_q_reg);

endmodule