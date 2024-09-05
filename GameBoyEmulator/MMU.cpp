#include "MMU.h"

#include <iomanip>
#include <iostream>

#include "HRAM.h"
#include "VRAM.h"
#include "WRAM.h"
#include "ExternalRAM.h"

#include "InterrupHandler.h"
#include "LCDC.h"
#include "Serial.h"

#include "OAM.h"
#include "PPU.h"

uint8_t MMU::fetch8(uint16_t address) {
    /**
     * Information of memory map is taken from;
     * https://gbdev.io/pandocs/Memory_Map.html
     */
    
    if(address <= 0x7FFF) {
        return memory[address];
    } else if(address >= 0x8000 && address <= 0x9FFF) {
        return vram.fetch8(address);
    } else if(address >= 0xA000 && address <= 0xBFFF) {
        return externalRam.read(address - 0xA000);
    } else if(address >= 0xC000 && address <= 0xDFFF) { // 0xCFFF
        return wram.fetch8(address - 0xC000);
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        return oam.fetch8(address);
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        return fetchIO(address);
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        return hram.fetch8(address - 0XFF80);
    } else if(address == 0xFFFF) {
        return interruptHandler.fetch8(address);
    } else {
        std::cerr << "Unknown fetch address\n";
    }
    
    return 0xFF;
}

uint8_t MMU::fetchIO(uint16_t address) {
    if(address == 0xFF00) {
        std::cerr << "Joypad input\n";
    } else if(address >= 0xFF01 && address <= 0xFF02) {
        return serial.fetch8(address);
    } else if(address >= 0xFF04 && address <= 0xFF07) {
        std::cerr << "Timer and divider\n";
    } else if(address == 0xFF0F) {
        //std::cerr << "Interrupts\n";
        return interruptHandler.fetch8(address);
    } else if(address >= 0xFF10 && address <= 0xFF26) {
        std::cerr << "Wave pattern\n";
    } else if(address >= 0xFF40 && address <= 0xFF4B) {
        //std::cerr << "LCD Control, Status, Position, Scrolling, and Palettes\n";
        
        if(address >= 0xFF40 && address <= 0xFF45) {
            return lcdc.fetch8(address);
        }/* else if(address == 0xFF46) {
            //  $FF46	DMA	OAM DMA source address & start
            
        } else if(address == 0xFF47) {
            // https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
            ppu.write8(address, data);
        } else if(address == 0xFF48 || address == 0xFF49) {
            // https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
            ppu.write8(address, data);
        }*/
    } else if(address == 0xFF4F) {
        std::cerr << "CGB VRAM Bank Select\n";
    } else if(address == 0xFF50) {
        std::cerr << "Set to non-zero to disable boot ROM\n";
        return 1;
    } else if(address >= 0xFF51 && address <= 0xFF55) {
        std::cerr << "CGB VRAM DMA\n";
    } else if(address >= 0xFF68 && address <= 0xFF6B) {
        std::cerr << "CGB BG / OBJ Pallets\n";
    } else if(address == 0xFF70) {
        std::cerr << "CGB WRAM Bank Select\n";
    } else {
        printf("IO Fetch Address; %x\n", address);
        std::cerr << "";
    }
    
    return 0;
}

uint16_t MMU::fetch16(uint16_t address) {
    uint16_t val = fetch8(address);
    val |= fetch8(address + 1) << 8;
    
    return val;
}

/*uint32_t MMU::fetch32(uint16_t address) {
    return static_cast<uint32_t>(memory[address]) | 
       (static_cast<uint32_t>(memory[address + 1]) << 8) |
       (static_cast<uint32_t>(memory[address + 2]) << 16) |
       (static_cast<uint32_t>(memory[address + 3]) << 24);
}*/

void MMU::write8(uint16_t address, uint8_t data) {
    /*if(address > 0x7FFF)
        printf("S");
        */
    
    if(address <= 0x7FFF) {
        memory[address] = data;
    } else if(address >= 0x8000 && address <= 0x9FFF) {
        vram.write8(address - 0x8000, data);
    } else if(address >= 0xA000 && address <= 0xBFFF) {
        externalRam.write(address - 0xA000, data);
    } else if(address >= 0xC000 && address <= 0xDFFF) { //  0xCFFF
        wram.write8(address - 0xC000, data);
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        oam.write8(address - 0xFE00, data);
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        writeIO(address, data);
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        hram.write8(address - 0xFF80, data);
    } else if(address == 0xFFFF) {
        interruptHandler.write8(address, data);
    } else {
        std::cerr << "Unknown write address\n";
    }
}

void MMU::writeIO(uint16_t address, uint8_t data) {
    // https://gbdev.io/pandocs/Hardware_Reg_List.html
    
     if(address == 0xFF00) {
        std::cerr << "Joypad input\n";
    } else if(address >= 0xFF01 && address <= 0xFF02) {
        serial.write8(address, data);
    } else if(address >= 0xFF04 && address <= 0xFF07) {
        std::cerr << "Timer and divider\n";
    } else if(address == 0xFF0F) {
        //std::cerr << "Interrupts\n";
        interruptHandler.write8(address, data);
    } else if(address >= 0xFF10 && address <= 0xFF26) {
        std::cerr << "Wave pattern\n";
    } else if(address >= 0xFF40 && address <= 0xFF4B) {
        //std::cerr << "LCD Control, Status, Position, Scrolling, and Palettes\n";
        
        if(address >= 0xFF40 && address <= 0xFF45) {
            lcdc.write8(address, data);
        } else if(address == 0xFF46) {
            //  $FF46	DMA	OAM DMA source address & start
            
        } else if(address == 0xFF47) {
            // https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
            ppu.write8(address, data);
        } else if(address == 0xFF48 || address == 0xFF49) {
            // https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
            ppu.write8(address, data);
        }
    } else if(address == 0xFF4F) {
        std::cerr << "CGB VRAM Bank Select\n";
    } else if(address == 0xFF50) {
        std::cerr << "Set to non-zero to disable boot ROM\n";
    } else if(address >= 0xFF51 && address <= 0xFF55) {
        std::cerr << "CGB VRAM DMA\n";
    } else if(address >= 0xFF68 && address <= 0xFF6B) {
        std::cerr << "CGB BG / OBJ Pallets\n";
    } else if(address == 0xFF70) {
        std::cerr << "CGB WRAM Bank Select\n";
    } else {
        printf("IO Write Address; %x = %x\n", address, data);
        std::cerr << "";
    }
}

void MMU::write16(uint16_t address, uint16_t data) {
    write8(address, data & 0xFF);
    write8(address + 1, (data >> 8) & 0xFF);
}

void MMU::clear() {
    std::fill(std::begin(memory), std::end(memory), 0);
}

/*
void MMU::write32(uint16_t address) {
    
}
*/
