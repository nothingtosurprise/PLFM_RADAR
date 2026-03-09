`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    19:04:35 12/14/2025 
// Design Name: 
// Module Name:    radar_transmitter 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module radar_transmitter(
    // System Clocks
    input wire clk_100m,           // System clock
    input wire clk_120m_dac,       // 120MHz DAC clock
    input wire reset_n,
    
    // DAC Interface
    output wire [7:0] dac_data,
    output wire dac_clk,
    output wire dac_sleep,
    output wire rx_mixer_en,
    output wire tx_mixer_en,
	 
	     // STM32 Control Interface
    input wire stm32_new_chirp,
    input wire stm32_new_elevation, 
    input wire stm32_new_azimuth,
    input wire stm32_mixers_enable,
	 
	 output wire fpga_rf_switch,
	 
	     // ADAR1000 Control Interface
    output wire adar_tx_load_1,
    output wire adar_rx_load_1,
    output wire adar_tx_load_2,
    output wire adar_rx_load_2,
    output wire adar_tx_load_3,
    output wire adar_rx_load_3,
    output wire adar_tx_load_4,
    output wire adar_rx_load_4,
    output wire adar_tr_1,
    output wire adar_tr_2,
    output wire adar_tr_3,
    output wire adar_tr_4,
    
    // Level Shifter SPI Interface (STM32F7 to ADAR1000)
    input wire stm32_sclk_3v3,
    input wire stm32_mosi_3v3,
    output wire stm32_miso_3v3,
    input wire stm32_cs_adar1_3v3,
    input wire stm32_cs_adar2_3v3,
    input wire stm32_cs_adar3_3v3,
    input wire stm32_cs_adar4_3v3,
    
    output wire stm32_sclk_1v8,
    output wire stm32_mosi_1v8,
    input wire stm32_miso_1v8,
    output wire stm32_cs_adar1_1v8,
    output wire stm32_cs_adar2_1v8,
    output wire stm32_cs_adar3_1v8,
    output wire stm32_cs_adar4_1v8,
	 
			 // Beam Position Tracking
	 output wire [5:0] current_elevation,
	 output wire [5:0] current_azimuth,
	 output wire [5:0] current_chirp,
	 output wire new_chirp_frame


    );
	 
// Edge Detection Signals
wire new_chirp_pulse;
wire new_elevation_pulse;
wire new_azimuth_pulse;

// Chirp Control Signals
wire [7:0] chirp_data;
wire chirp_valid;
wire chirp_sequence_done;

// Enhanced STM32 Input Edge Detection with Debouncing
edge_detector_enhanced chirp_edge (
    .clk(clk_100m),
    .reset_n(reset_n),
    .signal_in(stm32_new_chirp),
    .rising_falling_edge(new_chirp_pulse)            
);

edge_detector_enhanced elevation_edge (
    .clk(clk_100m),
    .reset_n(reset_n),
    .signal_in(stm32_new_elevation),
    .rising_falling_edge(new_elevation_pulse)
);

edge_detector_enhanced azimuth_edge (
    .clk(clk_100m),
    .reset_n(reset_n),
    .signal_in(stm32_new_azimuth),
    .rising_falling_edge(new_azimuth_pulse)
);

// Enhanced PLFM Chirp Generation
plfm_chirp_controller_enhanced plfm_chirp_inst (
    .clk_120m(clk_120m_dac),
    .clk_100m(clk_100m),
    .reset_n(reset_n),
    .new_chirp(new_chirp_pulse),
    .new_elevation(new_elevation_pulse),
    .new_azimuth(new_azimuth_pulse),
    .new_chirp_frame(new_chirp_frame),
    .mixers_enable(stm32_mixers_enable),
    .chirp_data(chirp_data),
    .chirp_valid(chirp_valid),
    .chirp_done(chirp_sequence_done),
    .rf_switch_ctrl(fpga_rf_switch),
    .rx_mixer_en(rx_mixer_en),
    .tx_mixer_en(tx_mixer_en),
    .adar_tx_load_1(adar_tx_load_1),
    .adar_rx_load_1(adar_rx_load_1),
    .adar_tx_load_2(adar_tx_load_2),
    .adar_rx_load_2(adar_rx_load_2),
    .adar_tx_load_3(adar_tx_load_3),
    .adar_rx_load_3(adar_rx_load_3),
    .adar_tx_load_4(adar_tx_load_4),
    .adar_rx_load_4(adar_rx_load_4),
    .adar_tr_1(adar_tr_1),
    .adar_tr_2(adar_tr_2),
    .adar_tr_3(adar_tr_3),
    .adar_tr_4(adar_tr_4),
    .elevation_counter(current_elevation),
    .azimuth_counter(current_azimuth),
    .chirp_counter(current_chirp)
);

// Enhanced DAC Interface
dac_interface_enhanced dac_interface_inst (
    .clk_120m(clk_120m_dac),
    .reset_n(reset_n),
    .chirp_data(chirp_data),
    .chirp_valid(chirp_valid),
    .dac_data(dac_data),
    .dac_clk(dac_clk),
    .dac_sleep(dac_sleep)
);
endmodule
