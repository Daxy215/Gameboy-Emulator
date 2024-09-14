#pragma once
#include <string>
#include <vector>

#include "enums/CartridgeTypes.h"

class Cartridge {
public:
    void decode(const std::vector<uint8_t>& data);

private:
    enums::CartridgeType getCartridgeType(uint8_t data);
    const char* cartridgeTypeToString(enums::CartridgeType type);
    
public:
    std::string title = "";
    std::string manufacturer = "";
    
    uint8_t romSize = 0;
    uint8_t ramSize = 0;
    
    enums::CartridgeType type = enums::UNKNOWN_CARTRIDGE;
};
