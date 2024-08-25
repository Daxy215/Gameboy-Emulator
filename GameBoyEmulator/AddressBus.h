#pragma once
#include <cstdint>
#include <vector>

#include "HRAM.h"
#include "WRAM.h"

class AddressBus {
public:
    AddressBus(std::vector<uint8_t>& memory) : memory(memory) {}
    
    uint8_t fetch8(uint16_t address);
    uint16_t fetch16(uint16_t address);
    //uint32_t fetch32(uint16_t address);
    
    void write8(uint16_t address, uint8_t data);
    void write16(uint16_t address, uint16_t data);
    //void write32(uint16_t address);

private:
    WRAM wram;
    HRAM hram;
    
public:
    std::vector<uint8_t> memory;
};
