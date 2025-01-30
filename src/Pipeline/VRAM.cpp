#include "VRAM.h"

#include <iostream>

#include "PPU.h"

#include "../Memory/Cartridge.h"
#include "../Utility/Bitwise.h"

uint8_t VRAM::fetch8(uint16_t address) {
    if(address == 0xFF4F) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff4f--vbk-cgb-mode-only-vram-bank
        
        /**
         * Reading from this register will return the number of the currently loaded VRAM bank in bit 0,
         * and all other bits will be set to 1.
         */
        if(Cartridge::mode != Color) return 0xFF;
        
        return 0b11111110 | vramBank;
    }
    
    if(address >= std::size(RAM)) {
        std::cerr << "OUT OF RANGE!\n";
    }
    
    uint16_t addr = (vramBank * 0x2000) + (address & 0x1FFF);
    
    return RAM[addr];
}

void VRAM::write8(uint16_t address, uint8_t data) {
    if(address == 0xFF4F) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff4f--vbk-cgb-mode-only-vram-bank
        if(Cartridge::mode != Color) return;
        
        vramBank = check_bit(data, 0);
        
        return;
    }
    
    if(address >= std::size(RAM)) {
        std::cerr << "OUT OF RANGE!\n";
    }
    
    uint16_t addr = (vramBank * 0x2000) + (address & 0x1FFF);
    
    RAM[addr] = data;
}