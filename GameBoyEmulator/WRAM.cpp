#include "WRAM.h"

uint8_t WRAM::fetchData(uint16_t address) {
    return RAM[address];
}

void WRAM::writeData(uint16_t address, uint8_t data) {
    RAM[address] = data;
}
