#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/MBC1.html#00003fff--rom-bank-x0-read-only
class WRAM {
public:
    WRAM();
    
    uint8_t fetch8(uint16_t address);
    void write8(uint16_t address, uint8_t data);
    
private:
    uint8_t ROMBank = 1; // For CGB - It is useless for now
    uint8_t RAM[32 * 1024] = { 0 }; // 8 KB
};
