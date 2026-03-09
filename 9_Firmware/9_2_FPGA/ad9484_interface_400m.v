module ad9484_interface_400m (
    // ADC Physical Interface (LVDS)
    input wire [7:0] adc_d_p,        // ADC Data P
    input wire [7:0] adc_d_n,        // ADC Data N
    input wire adc_dco_p,            // Data Clock Output P (400MHz)
    input wire adc_dco_n,            // Data Clock Output N (400MHz)
    
    // System Interface
    input wire sys_clk,              // 100MHz system clock (for control only)
    input wire reset_n,
    
    // Output at 400MHz domain
    output wire [7:0] adc_data_400m, // ADC data at 400MHz
    output wire adc_data_valid_400m  // Valid at 400MHz
);

// LVDS to single-ended conversion
wire [7:0] adc_data;
wire adc_dco;

// IBUFDS for each data bit
genvar i;
generate
    for (i = 0; i < 8; i = i + 1) begin : data_buffers
        IBUFDS #(
            .DIFF_TERM("TRUE"),
            .IOSTANDARD("LVDS_25")
        ) ibufds_data (
            .O(adc_data[i]),
            .I(adc_d_p[i]),
            .IB(adc_d_n[i])
        );
    end
endgenerate

// IBUFDS for DCO
IBUFDS #(
    .DIFF_TERM("TRUE"),
    .IOSTANDARD("LVDS_25") 
) ibufds_dco (
    .O(adc_dco),
    .I(adc_dco_p),
    .IB(adc_dco_n)
);

// IDDR for capturing DDR data
wire [7:0] adc_data_rise;  // Data on rising edge
wire [7:0] adc_data_fall;  // Data on falling edge

genvar j;
generate
    for (j = 0; j < 8; j = j + 1) begin : iddr_gen
        IDDR #(
            .DDR_CLK_EDGE("SAME_EDGE_PIPELINED"),
            .INIT_Q1(1'b0),
            .INIT_Q2(1'b0),
            .SRTYPE("SYNC")
        ) iddr_inst (
            .Q1(adc_data_rise[j]),   // Rising edge data
            .Q2(adc_data_fall[j]),   // Falling edge data
            .C(adc_dco),             // 400MHz DCO
            .CE(1'b1),
            .D(adc_data[j]),
            .R(1'b0),
            .S(1'b0)
        );
    end
endgenerate

// Combine rising and falling edge data to get 400MSPS stream
reg [7:0] adc_data_400m_reg;
reg adc_data_valid_400m_reg;
reg dco_phase;

always @(posedge adc_dco or negedge reset_n) begin
    if (!reset_n) begin
        adc_data_400m_reg <= 8'b0;
        adc_data_valid_400m_reg <= 1'b0;
        dco_phase <= 1'b0;
    end else begin
        dco_phase <= ~dco_phase;
        
        if (dco_phase) begin
            // Output falling edge data (completes the 400MSPS stream)
            adc_data_400m_reg <= adc_data_fall;
        end else begin
            // Output rising edge data
            adc_data_400m_reg <= adc_data_rise;
        end
        
        adc_data_valid_400m_reg <= 1'b1; // Always valid when ADC is running
    end
end

assign adc_data_400m = adc_data_400m_reg;
assign adc_data_valid_400m = adc_data_valid_400m_reg;

endmodule