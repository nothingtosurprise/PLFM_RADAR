`timescale 1ns / 1ps

// latency_buffer_2159_fixed.v
module latency_buffer_2159 #(
    parameter DATA_WIDTH = 32,
    parameter LATENCY = 3187
) (
    input wire clk,
    input wire reset_n,
    input wire [DATA_WIDTH-1:0] data_in,
    input wire valid_in,
    output wire [DATA_WIDTH-1:0] data_out,
    output wire valid_out
);

// ========== FIXED PARAMETERS ==========
localparam ADDR_WIDTH = 12;  // Enough for 4096 entries (>2159)

// ========== FIXED LOGIC ==========
(* ram_style = "block" *) reg [DATA_WIDTH-1:0] bram [0:4095];
reg [ADDR_WIDTH-1:0] write_ptr;
reg [ADDR_WIDTH-1:0] read_ptr;
reg valid_out_reg;

// Delay counter to track when LATENCY cycles have passed
reg [ADDR_WIDTH-1:0] delay_counter;
reg buffer_has_data;  // Flag when buffer has accumulated LATENCY samples

// ========== FIXED INITIALIZATION ==========
integer k;
initial begin
    for (k = 0; k < 4096; k = k + 1) begin
        bram[k] = {DATA_WIDTH{1'b0}};
    end
    write_ptr = 0;
    read_ptr = 0;
    valid_out_reg = 0;
    delay_counter = 0;
    buffer_has_data = 0;
end

// ========== FIXED STATE MACHINE ==========
always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
        write_ptr <= 0;
        read_ptr <= 0;
        valid_out_reg <= 0;
        delay_counter <= 0;
        buffer_has_data <= 0;
    end else begin
        // Default: no valid output
        valid_out_reg <= 0;
        
        // ===== WRITE SIDE =====
        if (valid_in) begin
            // Store data
            bram[write_ptr] <= data_in;
            
            // Increment write pointer (wrap at 4095)
            if (write_ptr == 4095) begin
                write_ptr <= 0;
            end else begin
                write_ptr <= write_ptr + 1;
            end
            
            // Count how many samples we've written
            if (delay_counter < LATENCY) begin
                delay_counter <= delay_counter + 1;
                
                // When we've written LATENCY samples, buffer is "primed"
                if (delay_counter == LATENCY - 1) begin
                    buffer_has_data <= 1'b1;
                //    $display("[LAT_BUF] Buffer now has %d samples (primed)", LATENCY);
                end
            end
        end
        
        // ===== READ SIDE =====
        // Only start reading after we have LATENCY samples in buffer
        if (buffer_has_data && valid_in) begin
            // Read pointer follows write pointer with LATENCY delay
            // Calculate: read_ptr = (write_ptr - LATENCY) mod 4096
            
            // Handle wrap-around correctly
            if (write_ptr >= LATENCY) begin
                read_ptr <= write_ptr - LATENCY;
            end else begin
                // Wrap around: 4096 + write_ptr - LATENCY
                read_ptr <= 4096 + write_ptr - LATENCY;
            end
            
            // Output is valid
            valid_out_reg <= 1'b1;
            
            //$display("[LAT_BUF] Reading: write_ptr=%d, read_ptr=%d, data=%h",
              //       write_ptr, read_ptr, bram[read_ptr]);
        end
    end
end

// ========== OUTPUTS ==========
assign data_out = bram[read_ptr];
assign valid_out = valid_out_reg;



endmodule