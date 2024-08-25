#include "AddressBus.h"

#include <iostream>

uint8_t AddressBus::fetch8(uint16_t address) {
    /**
     * Information of memory map is taken from;
     * https://gbdev.io/pandocs/Memory_Map.html
     */
    
    if(address <= 0x7FFF) {
        // 16 KiB ROM Bank 00   -   From cartridge, usually  a fixed bank
        //std::cout << "Soo.. ROM Bank 00 - 01\n";
        
        return memory[address];
    } /*else if(address >= 0x4000 && address <= 0x7FFF) {
        // 16 KiB ROM Bank 01-NN    -   From cartridge, switchable bank via mapper (if any)
        // mapper -> https://gbdev.io/pandocs/MBCs.html#mbcs
        std::cout << "Soo.. ROM Bank 01\n";
        
    }*/  else if(address >= 0x8000 && address <= 0x9FFF) {
        // 8 KiB Video Ram (VRAM)   -   In CGB mode, switchable bank 0/1
        std::cout << "Soo.. VRAM\n";
        
    } else if(address >= 0xA000 && address <= 0xBFFF) {
        // 8 KiB External Ram   -   From cartridge, switchable bank if any
        std::cout << "Soo.. External Ram\n";
        
    } else if(address >= 0xC000 && address <= 0xDFFF) { // 0xCFFF
        // 4 KiB Work RAM (WRAM)    -   No information provided
        //std::cout << "Soo.. WRAM\n";
        
        return wram.fetchData(address - 0xC000);
    }/* else if(address >= 0xD000 && address <= 0xDFFF) {
        // 4 KiB Work RAM (WRAM)    -   In CGB mode, switchable bank 1-7
        //std::cout << "Soo.. WRAM 1-7\n";
        
        return wram.fetchData(address);
    }*/ else if(address >= 0xE000 && address <= 0xFDFF) {
        // Echo RAM (mirror of C000-DDFF)   -   Nintendo says use of this area is prohibited
        //std::cout << "Soo.. Echo RAM no use!\n";
        
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        // Object attribute memory (OAM)    -   No further information provided
        std::cout << "Soo.. OAM\n";
        
    } else if(address >= 0xEA0 && address <= 0xFEFF) {
        // Not usable   -   Nintendo says use of this area is prohibited
        //std::cout << "Soo.. NO\n";
        
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        // I/O Registers    -   No further information provided
        std::cout << "Soo I/O..\n";
        
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        // High RAM (HRAM)  -       -   No further information provided
        
        return hram.fetchData(address - 0XFF80);
    } else if(address == 0xFFFF) {
        // Interrupt Enable register (IE)   -   No further information provided
        std::cerr << "Interrupt Enable register (IE)\n";
    }
    
    return -1;
}

uint16_t AddressBus::fetch16(uint16_t address) {
    uint16_t val = fetch8(address);
    val |= fetch8(address + 1) << 8;
    
    return val;
    
    /*return static_cast<uint16_t>(memory[address]) | 
       (static_cast<uint16_t>(memory[address + 1]) << 8);*/
}

/*uint32_t AddressBus::fetch32(uint16_t address) {
    return static_cast<uint32_t>(memory[address]) | 
       (static_cast<uint32_t>(memory[address + 1]) << 8) |
       (static_cast<uint32_t>(memory[address + 2]) << 16) |
       (static_cast<uint32_t>(memory[address + 3]) << 24);
}*/

void AddressBus::write8(uint16_t address, uint8_t data) {
    if(address <= 0x7FFF) {
        // 16 KiB ROM Bank 00   -   From cartridge, usually  a fixed bank
        //std::cout << "Soo.. ROM Bank 00 - 01\n";
        
        memory[address] = data;
    } /*else if(address >= 0x4000 && address <= 0x7FFF) {
        // 16 KiB ROM Bank 01-NN    -   From cartridge, switchable bank via mapper (if any)
        // mapper -> https://gbdev.io/pandocs/MBCs.html#mbcs
        std::cout << "Soo.. ROM Bank 01\n";
        
    }*/  else if(address >= 0x8000 && address <= 0x9FFF) {
        // 8 KiB Video Ram (VRAM)   -   In CGB mode, switchable bank 0/1
        std::cout << "Soo.. VRAM\n";
        
    } else if(address >= 0xA000 && address <= 0xBFFF) {
        // 8 KiB External Ram   -   From cartridge, switchable bank if any
        std::cout << "Soo.. External Ram\n";
        
    } else if(address >= 0xC000 && address <= 0xDFFF) { //  0xCFFF
        // 4 KiB Work RAM (WRAM)    -   No information provided
        wram.writeData(address - 0xC000, data);
    }/* else if(address >= 0xD000 && address <= 0xDFFF) {
        // 4 KiB Work RAM (WRAM)    -   In CGB mode, switchable bank 1-7
        std::cout << "Soo.. WRAM 1-7\n";
        
        wram.writeData(address - 0xD000, data);
    }*/ else if(address >= 0xE000 && address <= 0xFDFF) {
        // Echo RAM (mirror of C000-DDFF)   -   Nintendo says use of this area is prohibited
        //std::cout << "Soo.. Echo RAM no use!\n";
        
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        // Object attribute memory (OAM)    -   No further information provided
        std::cout << "Soo.. OAM\n";
        
    } else if(address >= 0xEA0 && address <= 0xFEFF) {
        // Not usable   -   Nintendo says use of this area is prohibited
        std::cout << "Soo.. NO\n";
        
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        // I/O Registers    -   No further information provided
        std::cout << "Soo I/O..\n";
        
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        // High RAM (HRAM)  -       -   No further information provided
        hram.writeData(address - 0xFF80, data);
    } else if(address == 0xFFFF) {
        // Interrupt Enable register (IE)   -   No further information provided
        std::cerr << "Interrupt Enable register (IE)\n";
    }
}

void AddressBus::write16(uint16_t address, uint16_t data) {
    write8(address, data & 0xFF);
    write8(address + 1, (data >> 8) & 0xFF);
}

/*
void AddressBus::write32(uint16_t address) {
    
}
*/
