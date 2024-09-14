#include "VRAM.h"

#include <cstdio>

#include "PPU.h"

uint8_t VRAM::fetch8(uint16_t address) {
    if(address >= 0x8000 && address <= 0x9FFF) {
        return RAM[address - 0x8000];
    }
    
    return 0xFF;
}

void VRAM::write8(uint16_t address, uint8_t data) {
    if(data != 0)
        printf("");
    
    RAM[address] = data;
}