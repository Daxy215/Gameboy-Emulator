#include "HRAM.h"

#include <iostream>
#include <string>

uint8_t HRAM::fetch8(uint16_t address) {
	if(address >= std::size(RAM)) {
		std::cerr << "OUT OF RANGE!\n";
	}
	
    return RAM[address];
}

void HRAM::write8(uint16_t address, uint8_t data) {
	if(address >= std::size(RAM)) {
		std::cerr << "OUT OF RANGE!\n";
	}
	
    RAM[address] = data;
}
