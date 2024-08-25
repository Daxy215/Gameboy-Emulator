#pragma once

#include <cstdint>

// https://gbdev.io/pandocs/MBC1.html#00003fff--rom-bank-x0-read-only
class HRAM {
public:
    
    uint8_t fetchData(uint16_t address);
    void writeData(uint16_t address, uint8_t data);
    
private:
    uint8_t RAM[0x80] = { 0 };
};
