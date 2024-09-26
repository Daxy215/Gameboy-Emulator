#include "VRAM.h"

#include "PPU.h"

uint8_t VRAM::fetch8(uint16_t address) {
    return RAM[address];
}

void VRAM::write8(uint16_t address, uint8_t data) {
    RAM[address] = data;
}