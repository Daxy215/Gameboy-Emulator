#include "MMU.h"

#include <iomanip>
#include <iostream>

#include "HRAM.h"
#include "../Pipeline/VRAM.h"
#include "WRAM.h"
#include "ExternalRAM.h"

#include "../CPU/InterrupHandler.h"
#include "../Pipeline/LCDC.h"
#include "../CPU/Serial.h"

#include "../Pipeline/OAM.h"
#include "../Pipeline/PPU.h"
#include "../Utility/Bitwise.h"

uint8_t MMU::fetch8(uint16_t address) {
    /**
     * Information of memory map is taken from;
     * https://gbdev.io/pandocs/Memory_Map.html
     */
    
    if(address <= 0x3FFF) { // bank 0 (fixed)
        return memory[address];
    } else if(address >= 0x4000 && address <= 0x7FFF) {
        // Switchable ROM bank
        uint16_t newAddress = address - 0x4000;
        
        // bank 0 isn't allowed in this region
        uint8_t bank = (m_CurrentROMBank == 0) ? 1 : m_CurrentROMBank;
        return memory[newAddress + (bank * 0x4000)];
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        // Switchable RAM bank
        if (m_EnableRAM) {
            uint16_t newAddress = address - 0xA000;
            uint8_t bank = (m_CurrentROMBank == 0) ? 1 : m_CurrentROMBank;
            
            //return m_RAMBanks[newAddress + (bank * 0x2000)];
            return externalRam.read(newAddress + (bank * 0x2000));
        }
        
        return 0xFF; // If RAM isn't enabled, return open bus (0xFF)
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        return wram.fetch8(address - 0xC000);
    } else if(address >= 0xD000 && address <= 0xFDFF) {
        return wram.fetch8((wramBank * 0x1000) | (address & 0x0FFF));
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        return oam.fetch8(address);
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        return fetchIO(address);
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        return hram.fetch8(address & 0x007F);
    } else if(address == 0xFFFF) {
        return interruptHandler.fetch8(address);
    } else {
        //std::cerr << "Unknown fetch address\n";
    }
    
    return 0xFF;
}

uint8_t MMU::fetchIO(uint16_t address) {
    if(address == 0xFF00) {
        std::cerr << "Joypad input fetch\n";
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
        }
    } else if(address == 0xFF4D) {
        return (key1 & 0x81) | 0x7E;
        //return (key1 & 0xb10000001) | 0b01111110;
    } else if(address == 0xFF4F) {
        //std::cerr << "CGB VRAM Bank Select\n";
    } else if(address == 0xFF50) {
        std::cerr << "Set to non-zero to disable boot ROM\n";
        return 1;
    } else if(address >= 0xFF51 && address <= 0xFF55) {
        //std::cerr << "CGB VRAM DMA\n";
    } else if(address >= 0xFF68 && address <= 0xFF6B) {
        //std::cerr << "CGB BG / OBJ Pallets\n";
    } else if(address == 0xFF70) {
        //std::cerr << "CGB WRAM Bank Select\n";
        return wramBank;
    } else {
        printf("IO Fetch Address; %x\n", address);
        std::cerr << "";
    }
    
    return 0xFF;
}

uint16_t MMU::fetch16(uint16_t address) {
    /*uint16_t f = static_cast<uint16_t>(fetch8(address));
    uint16_t g = (static_cast<uint16_t>(fetch8(address + 1) << 8));
    printf("F; %x - %x = %x\n", f, g, address);
    std::cerr << "";*/
    
    return static_cast<uint16_t>(fetch8(address)) | (static_cast<uint16_t>(fetch8(address + 1) << 8));
}

void MMU::write8(uint16_t address, uint8_t data) {
    if(address == 57086) {
        printf("");
    }
    
    // Handle ROM and RAM bank switching for MBC1/MBC2
    if (address < 0x8000) {
        // RAM enabling (0x0000 - 0x1FFF)
        if (address < 0x2000) {
            if (m_MBC1 || m_MBC2) {
                if (m_MBC2 && (address & 0x10)) return; // MBC2 checks bit 4 of address
                
                /**
                 * According to;
                 * https://gbdev.io/pandocs/MBC1.html
                 *
                 * Before external RAM can be read or written, it must be enabled by writing $A to anywhere in this address space.
                 * Any value with $A in the lower 4 bits enables the RAM attached to the MBC, and any other value disables the RAM.
                 * It is unknown why $A is the value used to enable RAM.
                 */
                
                uint8_t testData = data & 0xF;
                m_EnableRAM = (testData == 0xA);
            }
        }
        // ROM bank switching (0x2000 - 0x3FFF)
        else if (address >= 0x2000 && address < 0x4000) {
            if (m_MBC1 || m_MBC2) {
                if (m_MBC2) {
                    // MBC2: set ROM bank (lower 4 bits)
                    m_CurrentROMBank = data & 0xF;
                    
                    if (m_CurrentROMBank == 0) m_CurrentROMBank++;  // Avoid ROM bank 0
                } else {
                    // MBC1: set the lower 5 bits of ROM bank
                    uint8_t lower5 = data & 0x1F;
                    m_CurrentROMBank &= 0xE0; // Clear lower 5 bits
                    m_CurrentROMBank |= lower5; // Set new lower 5 bits
                    
                    if (m_CurrentROMBank == 0) m_CurrentROMBank++;  // Avoid ROM bank 0
                }
            }
        }
        // ROM or RAM bank switching (0x4000 - 0x5FFF)
        else if (address >= 0x4000 && address < 0x6000) {
            if (m_MBC1) {
                if (m_ROMBanking) {
                    // ROM bank switching (high bits)
                    m_CurrentROMBank &= 0x1F; // Clear upper bits
                    m_CurrentROMBank |= (data & 0xE0); // Set new upper bits (5 and 6)
                    if (m_CurrentROMBank == 0) m_CurrentROMBank++;  // Avoid ROM bank 0
                } else {
                    // RAM bank switching
                    m_CurrentRAMBank = data & 0x3; // Set lower 2 bits for RAM bank
                }
            }
        }
        // ROM/RAM mode selection (0x6000 - 0x7FFF)
        else if (address >= 0x6000 && address < 0x8000) {
            if (m_MBC1) {
                m_ROMBanking = (data & 0x1) == 0;  // Set ROM banking mode
                if (m_ROMBanking) {
                    m_CurrentRAMBank = 0;  // RAM banking disabled, default to RAM bank 0
                }
            }
        }
    }
    // Handle RAM bank writes (0xA000 - 0xBFFF)
    else if (address >= 0xA000 && address < 0xC000) {
        if (m_EnableRAM) {
            uint16_t newAddress = address - 0xA000;
            //m_RAMBanks[newAddress + (m_CurrentRAMBank * 0x2000)] = data;
            externalRam.write(newAddress + (m_CurrentRAMBank * 0x2000), data);
        }
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        wram.write8(address - 0xC000, data);
    } else if(address >= 0xD000 && address <= 0xFDFF) {
        wram.write8((wramBank * 0x1000) | (address & 0x0FFF), data);
    } else if (address >= 0xFE00 && address <= 0xFE9F) {
        oam.write8(address - 0xFE00, data);
    } else if (address >= 0xFF00 && address <= 0xFF7F) {
        writeIO(address, data);
    } else if (address >= 0xFF80 && address <= 0xFFFE) {
        hram.write8(address & 0x007F, data);
    } else if (address == 0xFFFF) {
        interruptHandler.write8(address, data);
    } else {
        //printf("Unknown write address %x - %x\n", address, data);
        //std::cerr << "";
    }
}

void MMU::writeIO(uint16_t address, uint8_t data) {
    // https://gbdev.io/pandocs/Hardware_Reg_List.html
    
     if(address == 0xFF00) {
        std::cerr << "Joypad input write\n";
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
    } else if(address == 0xFF4D) {
        key1 = (key1 & 0x80) | (data & 0x01); // Preserve current speed (bit 7) and `only` update switch armed (bit 0)
    } else if(address == 0xFF4F) {
        std::cerr << "CGB VRAM Bank Select\n";
    } else if(address == 0xFF50) {
        std::cerr << "Set to non-zero to disable boot ROM\n";
    } else if(address >= 0xFF51 && address <= 0xFF55) {
        std::cerr << "CGB VRAM DMA\n";
    } else if(address >= 0xFF68 && address <= 0xFF6B) {
        std::cerr << "CGB BG / OBJ Pallets\n";
    } else if(address == 0xFF70) {
        //std::cerr << "CGB WRAM Bank Select\n";
        wramBank = (data & 0x7) == 0 ? 1 : static_cast<size_t>(data & 0x7);
    } else {
        //printf("IO Write Address; %x = %x\n", address, data);
        //std::cerr << "";
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
