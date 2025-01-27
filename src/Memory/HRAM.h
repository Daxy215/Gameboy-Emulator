#pragma once

#include <cstdint>

// https://gbdev.io/pandocs/MBC1.html#00003fff--rom-bank-x0-read-only
class HRAM {
public:
    uint8_t fetch8(uint16_t address);
    void write8(uint16_t address, uint8_t data);
    
private:
    uint8_t RAM[127] = { 0 }; // 127 bytes
};
