#include "WRAM.h"

#include <random>
#include <cstdint>
#include <iostream>

WRAM::WRAM() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 255);
    
    for (auto& i : RAM) {
        i = static_cast<uint8_t>(distribution(generator));
    }
}

uint8_t WRAM::fetch8(uint16_t address) {
    if(address >= std::size(RAM)) {
        std::cerr << "OUT OF RANGE!\n";
    }
    
    return RAM[address];
}

void WRAM::write8(uint16_t address, uint8_t data) {
    if(address >= std::size(RAM)) {
        std::cerr << "OUT OF RANGE!\n";
    }
    
    RAM[address] = data;
}
