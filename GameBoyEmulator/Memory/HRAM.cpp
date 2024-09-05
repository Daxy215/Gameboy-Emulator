#include "HRAM.h"

uint8_t HRAM::fetch8(uint16_t address) {
    return RAM[address];
}

void HRAM::write8(uint16_t address, uint8_t data) {
    RAM[address] = data;
}
