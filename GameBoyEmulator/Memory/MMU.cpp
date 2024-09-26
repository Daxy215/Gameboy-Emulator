#include "MMU.h"

#include <iomanip>
#include <iostream>

#include "HRAM.h"
#include "../Pipeline/VRAM.h"
#include "WRAM.h"

#include "../IO/InterrupHandler.h"
#include "../Pipeline/LCDC.h"
#include "../IO/Serial.h"
#include "../IO/Timer.h"

#include "../Pipeline/OAM.h"
#include "../Pipeline/PPU.h"
#include "../Utility/Bitwise.h"
#include "MBC/MBC.h"

uint8_t MMU::fetch8(uint16_t address) {
    /**
     * Information of memory map is taken from;
     * https://gbdev.io/pandocs/Memory_Map.html
     */
    
    if(address <= 0x7FFF) {
        return mbc.read(address);
    } else if(address >= 0x8000 && address < 0x9FFF) {
        return vram.fetch8(address - 0x8000);
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        return mbc.read(address);
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        return wram.fetch8(address - 0xC000);
    } else if(address >= 0xD000 && address <= 0xDFFF) {
        return wram.fetch8((wramBank * 0x1000) | (address & 0x0FFF));
    } else if(address >= 0xE000 && address <= 0xFDFF) {
        return vram.fetch8(address - 0xE000);
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        return oam.fetch8(address);
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        return fetchIO(address);
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        return hram.fetch8(address & 0x007F);
    } else if(address == 0xFFFF) {
        return interruptHandler.fetch8(address);
    } else {
        /*printf("Unknown fetch address %x\n", address);
        std::cerr << "";*/
    }
    
    return 0xFF;
}

uint8_t MMU::fetchIO(uint16_t address) {
    if(address == 0xFF00) {
        return joypad.fetch8(address);
    } else if(address >= 0xFF01 && address <= 0xFF02) {
        return serial.fetch8(address);
    } else if(address >= 0xFF04 && address <= 0xFF07) {
        //std::cerr << "Timer and divider\n";
        return timer.fetch8(address);
    } else if(address == 0xFF0F) {
        //std::cerr << "Interrupts\n";
        return interruptHandler.fetch8(address);
    } else if(address >= 0xFF10 && address <= 0xFF26) {
        //std::cerr << "Wave pattern\n";
    } else if(address >= 0xFF30 && address <= 0xFF3F) {
        //std::cerr << "Wave pattern\n";
    } else if(address >= 0xFF40 && address <= 0xFF4B) {
        //std::cerr << "LCD Control, Status, Position, Scrolling, and Palettes\n";
        
        if(address >= 0xFF40 && address <= 0xFF45 || address >= 0xFF4A && address <= 0xFF4B) {
            return lcdc.fetch8(address);
        } else if(address == 0xFF46) {
            //  $FF46	DMA	OAM DMA source address & start
            return 0xFF;
        } else if(address == 0xFF47) {
            // https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
            return ppu.fetch8(address);
        } else if(address == 0xFF48 || address == 0xFF49) {
            // https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
            ppu.fetch8(address);
        } else {
            printf("IO Fetch Address; %x\n", address);
            std::cerr << "";
        }
    } else if(address == 0xFF4D) {
        // TODO; This is only for CGB
        //return (key1 & 0x81) | 0x7E;
        return (key1 & 0xb10000001) | 0b01111110;
    } else if(address == 0xFF4F) {
        //std::cerr << "CGB VRAM Bank Select\n";
    } else if(address == 0xFF50) {
        //std::cerr << "Set to non-zero to disable boot ROM\n";
        return 1;
    } else if(address >= 0xFF51 && address <= 0xFF55) {
        //std::cerr << "CGB VRAM DMA\n";
    } else if(address >= 0xFF68 && address <= 0xFF6B) {
        //std::cerr << "CGB BG / OBJ Pallets\n";
    } else if(address == 0xFF70) {
        //std::cerr << "CGB WRAM Bank Select\n";
        return wramBank;
    } else {
        /*printf("IO Fetch Address; %x\n", address);
        std::cerr << "";*/
    }
    
    return 0;
}

uint16_t MMU::fetch16(uint16_t address) {
    /*uint16_t f = static_cast<uint16_t>(fetch8(address));
    uint16_t g = (static_cast<uint16_t>(fetch8(address + 1) << 8));
    printf("F; %x - %x = %x\n", f, g, address);
    std::cerr << "";*/
    
    return static_cast<uint16_t>(fetch8(address)) | (static_cast<uint16_t>(fetch8(address + 1) << 8));
}

void MMU::write8(uint16_t address, uint8_t data) {
    if (address < 0x8000) {
        mbc.write(address, data);
    } else if(address >= 0x8000 && address < 0xA000) {
        vram.write8(address - 0x8000, data);
    } else if (address >= 0xA000 && address < 0xC000) {
        mbc.write(address, data);
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        wram.write8(address - 0xC000, data);
    } else if(address >= 0xD000 && address <= 0xDFFF) {
        wram.write8((wramBank * 0x1000) | (address & 0x0FFF), data);
    } else if(address >= 0xE000 && address <= 0xFDFF) {
        vram.write8(address - 0xE000, data);
    } else if (address >= 0xFE00 && address <= 0xFE9F) {
        oam.write8(address - 0xFE00, data);
    } else if (address >= 0xFF00 && address <= 0xFF7F) {
        writeIO(address, data);
    } else if (address >= 0xFF80 && address <= 0xFFFE) {
        hram.write8(address & 0x007F, data);
    } else if (address == 0xFFFF) {
        interruptHandler.write8(address, data);
    } else {
        /*printf("Unknown write address %x - %x\n", address, data);
        std::cerr << "";*/
    }
}

void MMU::writeIO(uint16_t address, uint8_t data) {
    // https://gbdev.io/pandocs/Hardware_Reg_List.html
    
     if(address == 0xFF00) {
        joypad.write8(address, data);
    } else if(address >= 0xFF01 && address <= 0xFF02) {
        serial.write8(address, data);
    } else if(address >= 0xFF04 && address <= 0xFF07) {
        //std::cerr << "Timer and divider\n";
        timer.write8(address, data);
    } else if(address == 0xFF0F) {
        //std::cerr << "Interrupts\n";
        interruptHandler.write8(address, data);
    } else if(address >= 0xFF10 && address <= 0xFF26) {
        //std::cerr << "Wave pattern\n";
    } else if(address >= 0xFF40 && address <= 0xFF4B) {
        //std::cerr << "LCD Control, Status, Position, Scrolling, and Palettes\n";
        
        if(address >= 0xFF40 && address <= 0xFF45 || address >= 0xFF4A && address <= 0xFF4B) {
            bool wasEnabled = lcdc.enable;
            
            lcdc.write8(address, data);
            
            if(address == 0xFF40) {
                /**
                 * set_test 3,"Turning LCD on starts too early in scanline"
                 * call disable_lcd
                 * ld   a,$81
                 * ldh  (LCDC-$FF00),a ; LCD on
                 * delay 110
                 * ldh  a,(LY-$FF00)   ; just after LY increments
                 * cp   1
                 * jp   nz,test_failed
                 *
                 * Passing test 2 correctly but stuck on test 1.
                 * Seems the issue to be that one of my CPU instructions,
                 * are wrongly implemented, as while it does ldh  a,(LY-$FF00),
                 * AF.A is 0 where it's supposed to be 1? If I understood that correctly..
               */
                if(wasEnabled && !lcdc.enable) {
                    if(PPU::mode == PPU::VBlank) {
                        //printf("nooo...");
                        //return;
                    }
                    
                    PPU::mode = PPU::HBlank;
                    lcdc.LY = 0;
                    //lcdc.WY = 0;
                    ppu.reset(0);
                } else if(!wasEnabled && lcdc.enable) {
                    PPU::mode = PPU::OAMScan;
                    ppu.reset(4);
                }
            }
            
        } else if(address == 0xFF46) {
            // TODO;
            /*if(cpu.halt) {
                // NOT ALLOWED!
            }*/
            
            //  $FF46	DMA	OAM DMA source address & start
            uint16_t u16 = (static_cast<uint16_t>(data)) << 8;
            //uint16_t endAddr = u16 | 0x9F;
            
            /*for(uint16_t i = u16; i < endAddr; i++) {
                uint16_t dif = i - u16;
                uint8_t val = fetch8(dif);
                write8(0xFE00 + i, val);
            }*/
            
            for(uint16_t i = 0; i < 160; i++) {
                uint8_t val = fetch8(u16 + i);
                write8(0xFE00 + i, val);
            }
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
        //std::cerr << "CGB VRAM Bank Select\n";
    } else if(address == 0xFF50) {
        //std::cerr << "Set to non-zero to disable boot ROM\n";
    } else if(address >= 0xFF51 && address <= 0xFF55) {
        std::cerr << "CGB VRAM DMA\n";
    } else if(address >= 0xFF68 && address <= 0xFF6B) {
        //std::cerr << "CGB BG / OBJ Pallets\n";
    } else if(address == 0xFF70) {
        //std::cerr << "CGB WRAM Bank Select\n";
        wramBank = (data & 0x7) == 0 ? 1 : static_cast<size_t>(data & 0x7);
    } else {
        /*printf("IO Write Address; %x = %x\n", address, data);
        std::cerr << "";*/
    }
}

void MMU::write16(uint16_t address, uint16_t data) {
    write8(address, data & 0xFF);
    write8(address + 1, (data >> 8) & 0xFF);
}

void MMU::clear() {
    // TODO;
    //std::fill(std::begin(memory), std::end(memory), 0);
}

/*
void MMU::write32(uint16_t address) {
    
}
*/
