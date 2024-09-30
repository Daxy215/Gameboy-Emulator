﻿#include "WRAM.h"

#include <random>

WRAM::WRAM() {
    // TODO; This should have random values
}

uint8_t WRAM::fetch8(uint16_t address) {
    return RAM[address];
}

void WRAM::write8(uint16_t address, uint8_t data) {
    
    RAM[address] = data;
}
