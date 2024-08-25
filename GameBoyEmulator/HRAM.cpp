#include "HRAM.h"

uint8_t HRAM::fetchData(uint16_t address) {
    return RAM[address];
}

void HRAM::writeData(uint16_t address, uint8_t data) {
    RAM[address] = data;
}
