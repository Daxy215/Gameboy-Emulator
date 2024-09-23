#include "VRAM.h"

#include "PPU.h"

uint8_t VRAM::fetch8(uint16_t address) {
    uint8_t f1 = RAM[address - 0x8000];
    uint8_t f2 = RAM[address & 0x1FFF];
    if(f1 != f2) {
        printf("nooo\n");
    }
    
    if(address >= 0x8000 && address <= 0x9FFF) {
        return RAM[address - 0x8000];
    }
    
    return 0xFF;
}

void VRAM::write8(uint16_t address, uint8_t data) {
    RAM[address] = data;
}