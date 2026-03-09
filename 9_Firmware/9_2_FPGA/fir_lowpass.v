`timescale 1ns / 1ps

module fir_lowpass_parallel_enhanced (
    input wire clk,
    input wire reset_n,
    input wire signed [17:0] data_in,
    input wire data_valid,
    output reg signed [17:0] data_out,
    output reg data_out_valid,
    output wire fir_ready,
    output wire filter_overflow
);

parameter TAPS = 32;
parameter COEFF_WIDTH = 18;
parameter DATA_WIDTH = 18;
parameter ACCUM_WIDTH = 36;

// Filter coefficients
reg signed [COEFF_WIDTH-1:0] coeff [0:TAPS-1];

// Parallel delay line
reg signed [DATA_WIDTH-1:0] delay_line [0:TAPS-1];

// Parallel multiply-accumulate structure
wire signed [DATA_WIDTH+COEFF_WIDTH-1:0] mult_result [0:TAPS-1];

// Wires for parallel addition (combinatorial)
wire signed [ACCUM_WIDTH-1:0] sum_stage1_0, sum_stage1_1, sum_stage1_2, sum_stage1_3;
wire signed [ACCUM_WIDTH-1:0] sum_stage2_0, sum_stage2_1;
wire signed [ACCUM_WIDTH-1:0] sum_stage3;

// Registered accumulator
reg signed [ACCUM_WIDTH-1:0] accumulator_reg;

// Initialize coefficients
initial begin
    // Proper low-pass filter coefficients
    coeff[ 0] = 18'sh00AD; coeff[ 1] = 18'sh00CE; coeff[ 2] = 18'sh3FD87; coeff[ 3] = 18'sh02A6;
    coeff[ 4] = 18'sh00E0; coeff[ 5] = 18'sh3F8C0; coeff[ 6] = 18'sh0A45; coeff[ 7] = 18'sh3FD82;
    coeff[ 8] = 18'sh3F0B5; coeff[ 9] = 18'sh1CAD; coeff[10] = 18'sh3EE59; coeff[11] = 18'sh3E821;
    coeff[12] = 18'sh4841; coeff[13] = 18'sh3B340; coeff[14] = 18'sh3E299; coeff[15] = 18'sh1FFFF;
    coeff[16] = 18'sh1FFFF; coeff[17] = 18'sh3E299; coeff[18] = 18'sh3B340; coeff[19] = 18'sh4841;
    coeff[20] = 18'sh3E821; coeff[21] = 18'sh3EE59; coeff[22] = 18'sh1CAD; coeff[23] = 18'sh3F0B5;
    coeff[24] = 18'sh3FD82; coeff[25] = 18'sh0A45; coeff[26] = 18'sh3F8C0; coeff[27] = 18'sh00E0;
    coeff[28] = 18'sh02A6; coeff[29] = 18'sh3FD87; coeff[30] = 18'sh00CE; coeff[31] = 18'sh00AD;
end

// Generate parallel multipliers
genvar k;
generate
    for (k = 0; k < TAPS; k = k + 1) begin : mult_gen
        assign mult_result[k] = delay_line[k] * coeff[k];
    end
endgenerate

// COMBINATORIAL PARALLEL ADDITION TREE
// Stage 1: Group of 8
assign sum_stage1_0 = mult_result[0] + mult_result[1] + mult_result[2] + mult_result[3] +
                     mult_result[4] + mult_result[5] + mult_result[6] + mult_result[7];
assign sum_stage1_1 = mult_result[8] + mult_result[9] + mult_result[10] + mult_result[11] +
                     mult_result[12] + mult_result[13] + mult_result[14] + mult_result[15];
assign sum_stage1_2 = mult_result[16] + mult_result[17] + mult_result[18] + mult_result[19] +
                     mult_result[20] + mult_result[21] + mult_result[22] + mult_result[23];
assign sum_stage1_3 = mult_result[24] + mult_result[25] + mult_result[26] + mult_result[27] +
                     mult_result[28] + mult_result[29] + mult_result[30] + mult_result[31];

// Stage 2: Combine groups of 2
assign sum_stage2_0 = sum_stage1_0 + sum_stage1_1;
assign sum_stage2_1 = sum_stage1_2 + sum_stage1_3;

// Stage 3: Final sum
assign sum_stage3 = sum_stage2_0 + sum_stage2_1;

integer i;

// SINGLE-CYCLE PIPELINE PROCESSING
always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
        // Reset delay line
        for (i = 0; i < TAPS; i = i + 1) begin
            delay_line[i] <= 0;
        end
        accumulator_reg <= 0;
        data_out <= 0;
        data_out_valid <= 0;
    end else begin
        // Always shift in new data when valid
        if (data_valid) begin
            // Shift delay line
            for (i = TAPS-1; i > 0; i = i - 1) begin
                delay_line[i] <= delay_line[i-1];
            end
            delay_line[0] <= data_in;
            
            // Register the combinatorial sum
            accumulator_reg <= sum_stage3;
            
            // Output with 1-cycle latency
            data_out_valid <= 1'b1;
        end else begin
            data_out_valid <= 1'b0;
        end
        
        // Output saturation logic (registered)
        if (accumulator_reg > (2**(ACCUM_WIDTH-2)-1)) begin
            data_out <= (2**(DATA_WIDTH-1))-1;
        end else if (accumulator_reg < -(2**(ACCUM_WIDTH-2))) begin
            data_out <= -(2**(DATA_WIDTH-1));
        end else begin
            // Round and truncate (keep middle bits)
            data_out <= accumulator_reg[ACCUM_WIDTH-2:DATA_WIDTH-1];
        end
    end
end

// Always ready to accept new data
assign fir_ready = 1'b1;

// Overflow detection (simplified)
assign filter_overflow = (accumulator_reg > (2**(ACCUM_WIDTH-2)-1)) || 
                         (accumulator_reg < -(2**(ACCUM_WIDTH-2)));

endmodule