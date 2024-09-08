#include "WRAM.h"

#include <random>

WRAM::WRAM() {
    
}

uint8_t WRAM::fetch8(uint16_t address) {
    return RAM[address];
}

uint16_t lastwWrite = 0;

void WRAM::write8(uint16_t address, uint8_t data) {
    if(address == 0x1FFD) {
        lastwWrite = address;
    }
    
    RAM[address] = data;
}
