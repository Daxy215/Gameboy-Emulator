﻿#include "Serial.h"

#include <functional>
#include <optional>

std::optional<uint8_t> noop(uint8_t) {
    return std::nullopt;
}

uint8_t Serial::fetch8(uint16_t address) {
    if(address == 0xFF01) {
        return transferData;
    } else if(address == 0xFF02) {
        return transferControl | 0x7E;
    }
    
    return 0xFF;
}

void Serial::write8(uint16_t address, uint8_t data) {
    /*
     * $FF01 SB	Serial transfer data	R/W	All
     * $FF02 SC	Serial transfer control	R/W	Mixed
     */
    
    if(address == 0xFF01) {
        transferData = data;
    } else if(address == 0xFF02) {
        transferControl = data;
        
        if((data & 0x81) == 0x81) {
            auto res = callback(transferData);
            
            if(res.has_value()) {
                transferData = res.value();
                interrupt |= 0x08;
            }
        }
    }
}
