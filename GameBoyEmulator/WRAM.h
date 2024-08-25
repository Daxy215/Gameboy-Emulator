#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/MBC1.html#00003fff--rom-bank-x0-read-only
class WRAM {
public:
    
    uint8_t fetchData(uint16_t address);
    void writeData(uint16_t address, uint8_t data);
private:
    uint8_t ROMBank = 1;
    uint8_t RAM[0x2000] = { 0 };
};
