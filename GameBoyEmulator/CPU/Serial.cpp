#include "Serial.h"

#include <cstdio>

#include "../Utility/Bitwise.h"

uint8_t Serial::fetch8(uint16_t address) {
    return transferData;
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
        
        if(check_bit(data, 7)) {
            printf("%x\n", data);
            fflush(stdout);
        }
    }
}
