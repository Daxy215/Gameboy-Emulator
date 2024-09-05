#pragma once
#include <cstdint>

class Serial {
public:
    uint8_t fetch8(uint16_t address);
    void write8(uint16_t address, uint8_t data);
    
private:
    uint8_t transferData;
    uint8_t transferControl;
};
