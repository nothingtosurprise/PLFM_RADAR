`timescale 1ns / 1ps
module lvds_to_cmos_400m (
    // ADC Physical Interface (LVDS Inputs)
    input wire clk_400m_p,            // Data Clock Output P (400MHz LVDS, 2.5V)
    input wire clk_400m_n,            // Data Clock Output N (400MHz LVDS, 2.5V)
    input wire reset_n,              // Active-low reset
    
    // CMOS Output Interface (400MHz Domain)
    output reg clk_400m_cmos         // ADC data clock (CMOS, 3.3V)
);

// LVDS to single-ended conversion
wire clk_400m_se;             // Single-ended DCO from ADC


// IBUFDS for DCO clock (LVDS to CMOS conversion)
IBUFDS #(
    .DIFF_TERM("FALSE"),     // DISABLE internal termination (using external 100O)
    .IOSTANDARD("LVDS_25")   // 2.5V LVDS standard
) ibufds_dco (
    .O(clk_400m_se),          // Single-ended 400MHz clock
    .I(clk_400m_p),
    .IB(clk_400m_n)
);

// Optional: Global clock buffer for better clock distribution
wire clk_400m_buffered;
BUFG bufg_dco (
    .I(clk_400m_se),
    .O(clk_400m_buffered)
);


// Main processing: Capture on rising edge only
always @(posedge clk_400m_buffered or negedge reset_n) begin
    if (!reset_n) begin
        // Reset state
        clk_400m_cmos <= 1'b0;
    end else begin
        // Output buffered DCO clock
        clk_400m_cmos <= clk_400m_buffered;
    end
end

endmodule
