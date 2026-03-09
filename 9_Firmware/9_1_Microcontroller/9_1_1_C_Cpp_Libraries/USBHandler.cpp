#include "USBHandler.h"
#include <cstring>

USBHandler::USBHandler() {
    reset();
}

void USBHandler::reset() {
    current_state = USBState::WAITING_FOR_START;
    start_flag_received = false;
    buffer_index = 0;
    current_settings.resetToDefaults();
}

void USBHandler::processUSBData(const uint8_t* data, uint32_t length) {
    if (data == nullptr || length == 0) {
        return;
    }
    
    switch (current_state) {
        case USBState::WAITING_FOR_START:
            processStartFlag(data, length);
            break;
            
        case USBState::RECEIVING_SETTINGS:
            processSettingsData(data, length);
            break;
            
        case USBState::READY_FOR_DATA:
            // Ready to receive radar data commands
            // Add additional command processing here if needed
            break;
    }
}

void USBHandler::processStartFlag(const uint8_t* data, uint32_t length) {
    // Start flag: bytes [23, 46, 158, 237]
    const uint8_t START_FLAG[] = {23, 46, 158, 237};
    
    // Check if start flag is in the received data
    for (uint32_t i = 0; i <= length - 4; i++) {
        if (memcmp(data + i, START_FLAG, 4) == 0) {
            start_flag_received = true;
            current_state = USBState::RECEIVING_SETTINGS;
            buffer_index = 0;  // Reset buffer for settings data
            
            // You can send an acknowledgment back here if needed
            // sendUSBAcknowledgment();
            
            // If there's more data after the start flag, process it
            if (length > i + 4) {
                processSettingsData(data + i + 4, length - i - 4);
            }
            return;
        }
    }
}

void USBHandler::processSettingsData(const uint8_t* data, uint32_t length) {
    // Add data to buffer
    uint32_t bytes_to_copy = (length < (MAX_BUFFER_SIZE - buffer_index)) ? 
                             length : (MAX_BUFFER_SIZE - buffer_index);
    
    memcpy(usb_buffer + buffer_index, data, bytes_to_copy);
    buffer_index += bytes_to_copy;
    
    // Check if we have a complete settings packet (contains "SET" and "END")
    if (buffer_index >= 74) {  // Minimum size for valid settings packet
        // Look for "SET" at beginning and "END" somewhere in the packet
        bool has_set = (memcmp(usb_buffer, "SET", 3) == 0);
        bool has_end = false;
        
        for (uint32_t i = 3; i <= buffer_index - 3; i++) {
            if (memcmp(usb_buffer + i, "END", 3) == 0) {
                has_end = true;
                
                // Parse the complete packet up to "END"
                if (has_set && current_settings.parseFromUSB(usb_buffer, i + 3)) {
                    current_state = USBState::READY_FOR_DATA;
                    
                    // You can send settings acknowledgment back here
                    // sendSettingsAcknowledgment();
                }
                break;
            }
        }
        
        // If we didn't find a valid packet but buffer is full, reset
        if (buffer_index >= MAX_BUFFER_SIZE && !has_end) {
            buffer_index = 0;  // Reset buffer to avoid overflow
        }
    }
}