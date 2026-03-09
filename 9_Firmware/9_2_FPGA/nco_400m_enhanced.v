`timescale 1ns / 1ps

module nco_400m_enhanced (
    input wire clk_400m,
    input wire reset_n,
    input wire [31:0] frequency_tuning_word,
    input wire phase_valid,
    input wire [15:0] phase_offset,
    output reg signed [15:0] sin_out,
    output reg signed [15:0] cos_out,
    output reg dds_ready
);

// Phase accumulator with registered outputs for better timing
reg [31:0] phase_accumulator;
reg [31:0] phase_accumulator_reg;
reg [31:0] phase_with_offset;
reg phase_valid_delayed;

// Use only the top 8 bits for LUT addressing (256-entry LUT equivalent)
wire [7:0] lut_address = phase_with_offset[31:24];

// Quarter-wave sine LUT (0-90 degrees only)
reg [15:0] sin_lut [0:63]; // 64 entries for 0-90 degrees

// Initialize sine LUT
integer lut_init_i;
initial begin
    for (lut_init_i = 0; lut_init_i < 64; lut_init_i = lut_init_i + 1) begin
        sin_lut[lut_init_i] = 16'h0000;
    end
    
    // Initialize quarter-wave sine LUT (0-90 degrees)
    sin_lut[0] = 16'h0000; sin_lut[1] = 16'h0324; sin_lut[2] = 16'h0647; sin_lut[3] = 16'h096A;
    sin_lut[4] = 16'h0C8B; sin_lut[5] = 16'h0FA9; sin_lut[6] = 16'h12C4; sin_lut[7] = 16'h15DB;
    sin_lut[8] = 16'h18EC; sin_lut[9] = 16'h1BF8; sin_lut[10] = 16'h1EFC; sin_lut[11] = 16'h21F8;
    sin_lut[12] = 16'h24EB; sin_lut[13] = 16'h27D4; sin_lut[14] = 16'h2AB1; sin_lut[15] = 16'h2D82;
    sin_lut[16] = 16'h3045; sin_lut[17] = 16'h32F9; sin_lut[18] = 16'h359D; sin_lut[19] = 16'h3830;
    sin_lut[20] = 16'h3AB1; sin_lut[21] = 16'h3D1E; sin_lut[22] = 16'h3F76; sin_lut[23] = 16'h41B8;
    sin_lut[24] = 16'h43E3; sin_lut[25] = 16'h45F5; sin_lut[26] = 16'h47EE; sin_lut[27] = 16'h49CD;
    sin_lut[28] = 16'h4B90; sin_lut[29] = 16'h4D37; sin_lut[30] = 16'h4EC1; sin_lut[31] = 16'h502D;
    sin_lut[32] = 16'h517A; sin_lut[33] = 16'h52A8; sin_lut[34] = 16'h53B6; sin_lut[35] = 16'h54A4;
    sin_lut[36] = 16'h5572; sin_lut[37] = 16'h561F; sin_lut[38] = 16'h56AA; sin_lut[39] = 16'h5715;
    sin_lut[40] = 16'h575E; sin_lut[41] = 16'h5785; sin_lut[42] = 16'h578B; sin_lut[43] = 16'h576E;
    sin_lut[44] = 16'h5730; sin_lut[45] = 16'h56D0; sin_lut[46] = 16'h564E; sin_lut[47] = 16'h55AB;
    sin_lut[48] = 16'h54E7; sin_lut[49] = 16'h5403; sin_lut[50] = 16'h52FE; sin_lut[51] = 16'h51DA;
    sin_lut[52] = 16'h5096; sin_lut[53] = 16'h4F34; sin_lut[54] = 16'h4DB4; sin_lut[55] = 16'h4C17;
    sin_lut[56] = 16'h4A5E; sin_lut[57] = 16'h4889; sin_lut[58] = 16'h4699; sin_lut[59] = 16'h448F;
    sin_lut[60] = 16'h426B; sin_lut[61] = 16'h402F; sin_lut[62] = 16'h3DDB; sin_lut[63] = 16'h3B71;
end

// Quadrant determination
wire [1:0] quadrant = lut_address[7:6]; // 00: Q1, 01: Q2, 10: Q3, 11: Q4
wire [5:0] lut_index = (quadrant[1] ? ~lut_address[5:0] : lut_address[5:0]); // Mirror for Q2/Q3

// Sine and cosine calculation with quadrant mapping
wire [15:0] sin_abs = sin_lut[lut_index];
wire [15:0] cos_abs = sin_lut[63 - lut_index]; // Cosine is phase-shifted sine

// Pipeline stage for better timing
always @(posedge clk_400m or negedge reset_n) begin
    if (!reset_n) begin
        phase_accumulator <= 32'h00000000;
        phase_accumulator_reg <= 32'h00000000;
        phase_with_offset <= 32'h00000000;
        phase_valid_delayed <= 1'b0;
        dds_ready <= 1'b0;
        sin_out <= 16'h0000;
        cos_out <= 16'h7FFF;
    end else begin
        phase_valid_delayed <= phase_valid;
        
        if (phase_valid) begin
            // Update phase accumulator with dithered frequency tuning word
            phase_accumulator <= phase_accumulator + frequency_tuning_word;
            phase_accumulator_reg <= phase_accumulator;
            
            // Apply phase offset
            phase_with_offset <= phase_accumulator + {phase_offset, 16'b0};
            dds_ready <= 1'b1;
        end else begin
            dds_ready <= 1'b0;
        end
        
        // Generate outputs with one cycle delay for pipelining
        if (phase_valid_delayed) begin
            // Calculate sine and cosine with proper quadrant signs
            case (quadrant)
                2'b00: begin // Quadrant I: sin+, cos+
                    sin_out <= sin_abs;
                    cos_out <= cos_abs;
                end
                2'b01: begin // Quadrant II: sin+, cos-
                    sin_out <= sin_abs;
                    cos_out <= -cos_abs;
                end
                2'b10: begin // Quadrant III: sin-, cos-
                    sin_out <= -sin_abs;
                    cos_out <= -cos_abs;
                end
                2'b11: begin // Quadrant IV: sin-, cos+
                    sin_out <= -sin_abs;
                    cos_out <= cos_abs;
                end
            endcase
        end
    end
end

// Add this to ensure LUT is properly loaded:
initial begin
    // Wait a small amount of time for LUT initialization
    #10;
    $display("NCO: Sine LUT initialized with %0d entries", 64);
end

endmodule
