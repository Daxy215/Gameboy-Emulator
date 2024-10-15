#include "MMU.h"

#include <iomanip>
#include <iostream>

#include "HRAM.h"
#include "../Pipeline/VRAM.h"
#include "WRAM.h"
#include "../APU/APU.h"

#include "../IO/InterrupHandler.h"
#include "../Pipeline/LCDC.h"
#include "../IO/Serial.h"
#include "../IO/Timer.h"

#include "../Pipeline/OAM.h"
#include "../Pipeline/PPU.h"
#include "../Utility/Bitwise.h"
#include "MBC/MBC.h"

MMU::MMU(InterruptHandler& interruptHandler, Serial& serial, Joypad& joypad, MBC& mbc, WRAM& wram, HRAM& hram,
    VRAM& vram, LCDC& lcdc, Timer& timer, OAM& oam, PPU& ppu, APU& apu, const std::vector<uint8_t>& bootRom,
    const std::vector<uint8_t>& memory) 
        : interruptHandler(interruptHandler),
          serial(serial),
          joypad(joypad),
          timer(timer),
          mbc(mbc),
          wram(wram),
          hram(hram),
          vram(vram),
          lcdc(lcdc),
          oam(oam),
          ppu(ppu),
          apu(apu),
          bootRom(bootRom) {
    bootRomActive = (Cartridge::mode == DMG);
}

void MMU::tick(uint32_t cycles) {
    //joypad.checkForInterrupts();
    
    /*if(dma.active) {
        dma.remainingCycles -= cycles;
        
        if(dma.remainingCycles <= 0) {
            dma.active = false;
            
            for(uint16_t i = 0; i < 160; i++) {
                uint8_t val = fetch8(dma.source + i);
                write8(0xFE00 + i, val);
            }
        }
    }*/
    
    // HDMA
    // TODO; Check if this correct
    if(enabled) {
        // GDMA
        if (mode == 0) {
            uint16_t len = length + 1;
            
            for (uint16_t i = 0; i < len; i++) {
                for(uint16_t j = 0; j < 0x10; j++) {
                    uint8_t val = fetch8(sourceIndex + j);
                    write8(destIndex + j, val);
                }
                
                sourceIndex += 0x10;
                destIndex += 0x10;
                
                if(length == 0) {
                    length = 0x7F;
                } else {
                    length--;
                }
            }
            
            enabled = false;
            this->cycles = len * 8;
        } else if(mode == 1 && PPU::mode == PPU::HBlank) {
            // HBlank - HDMA
            for(uint16_t j = 0; j < 0x10; j++) {
                uint8_t val = fetch8(sourceIndex + j);
                write8(destIndex + j, val);
            }
            
            sourceIndex += 16;
            destIndex += 16;
            
            if(length == 0) {
                enabled = false;
            } else {
                length--;
            }
            
            this->cycles = 8;
        }
    }
}

uint8_t MMU::fetch8(uint16_t address) {
    /**
     * Information of memory map is taken from;
     * https://gbdev.io/pandocs/Memory_Map.html
     */
    
    if(bootRomActive && address < 0x100) {
        return bootRom[address];
    }
    
    /*if (dma.active && (address < 0xFF80 || address > 0xFFFE)) {
        return 0xFF;
    }*/
    
    if(address <= 0x7FFF) {
        return mbc.read(address);
    } else if(address >= 0x8000 && address <= 0x9FFF) {
        return vram.fetch8(address/* - 0x8000*/);
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        return mbc.read(address);
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        return wram.fetch8(address - 0xC000);
    } else if(address >= 0xD000 && address <= 0xDFFF) {
        return wram.fetch8((wramBank * 0x1000) | (address & 0x0FFF));
    } else if(address >= 0xE000 && address <= 0xFDFF) {
        return wram.fetch8(address & 0x0FFF);
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        return oam.fetch8(address);
    } else if(address >= 0xFEA0 && address <= 0xFEFF) {
        // Not usable
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        return fetchIO(address);
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        return hram.fetch8(address & 0x007F);
    } else if(address == 0xFFFF) {
        return interruptHandler.fetch8(address);
    } else {
        printf("Unknown fetch address %x\n", address);
        std::cerr << "";
    }
    
    return 0xFF;
}

uint8_t MMU::fetchIO(uint16_t address) {
    if(address == 0xFF00) {
        return joypad.fetch8(address);
    } else if(address >= 0xFF01 && address <= 0xFF02) {
        return serial.fetch8(address);
    } else if(address >= 0xFF04 && address <= 0xFF07) {
        return timer.fetch8(address);
    } else if(address == 0xFF0F) {
        return interruptHandler.fetch8(address);
    } else if(address >= 0xFF10 && address <= 0xFF26) {
        return apu.fetch8(address);
    } else if(address >= 0xFF30 && address <= 0xFF3F) {
        return apu.fetch8(address);
    } else if(address >= 0xFF40 && address <= 0xFF4B) {
        //std::cerr << "LCD Control, Status, Position, Scrolling, and Palettes\n";
        
        // TODO; Please clean this..
        if(address >= 0xFF40 && address <= 0xFF45 || address >= 0xFF4A && address <= 0xFF4B) {
            return lcdc.fetch8(address);
        } else if(address == 0xFF46) {
            //  $FF46	DMA	OAM DMA source address & start
            return lastDma;
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
        // CGB Mode only
        if(Cartridge::mode != Color) return 0xFF;
        
        return 0b01111110 | (doubleSpeed ? 0x80 : 0) | (switchArmed ? 1 : 0);
    } else if(address == 0xFF4F) {
        return vram.fetch8(address);
    } else if(address == 0xFF50) {
        std::cerr << "Set to non-zero to disable boot ROM\n";
        return 0xFF;
    }  else if(address == 0xFF51) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff51ff52--hdma1-hdma2-cgb-mode-only-vram-dma-source-high-low-write-only
        if(Cartridge::mode != Color) return 0xFF;
        
        return sourceLow;
        //return (source & 0xFF00) >> 8;
    } else if(address == 0xFF52) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff51ff52--hdma1-hdma2-cgb-mode-only-vram-dma-source-high-low-write-only
        if(Cartridge::mode != Color) return 0xFF;
        
        return sourceHigh;
        //return source & 0xFF;
    } else if(address == 0xFF53) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff53ff54--hdma3-hdma4-cgb-mode-only-vram-dma-destination-high-low-write-only
        if(Cartridge::mode != Color) return 0xFF;
        
        return destLow;
        //return (dest & 0xFF00) >> 8;
    } else if(address == 0xFF54) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff53ff54--hdma3-hdma4-cgb-mode-only-vram-dma-destination-high-low-write-only
        if(Cartridge::mode != Color) return 0xFF;
        
        return destHigh;
        //return dest & 0xFF;
    } else if(address == 0xFF55) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff55--hdma5-cgb-mode-only-vram-dma-lengthmodestart
        if(Cartridge::mode != Color) return 0xFF;
        
        return length | (!enabled ? 0x80 : 0);
    } else if(address >= 0xFF68 && address <= 0xFF6C) {
        return ppu.fetch8(address);
    } else if(address == 0xFF70) {
        if(Cartridge::mode != Color) return 0xFF;
        
        return wramBank & 0x7;
    }
    
    // TODO; Addresses that doesn't exist
    else if(address != 0xFF03 && address != 0xFF08 && address != 0xFF09 && address != 0xFF0A &&
            address != 0xFF0B && address != 0xFF0C && address != 0xFF0D &&
            address != 0xFF0E && address != 0xFF27 && address != 0xFF28 && address != 0xFF29 &&
            address != 0xFF2A && address != 0xFF2B && address != 0xFF2C &&
            address != 0xFF2D && address != 0xFF2E && address != 0xFF2F) {
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
    if(address == 65429) {
        printf("");
    }
    
    if (address < 0x8000) {
        mbc.write(address, data);
    } else if(address >= 0x8000 && address <= 0x9FFF) {
        vram.write8(address/* - 0x8000*/, data);
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        mbc.write(address, data);
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        wram.write8(address - 0xC000, data);
    } else if(address >= 0xD000 && address <= 0xDFFF) {
        wram.write8((wramBank * 0x1000) | (address & 0x0FFF), data);
    } else if(address >= 0xE000 && address <= 0xFDFF) {
        wram.write8(address & 0x0FFF, data);
    } else if (address >= 0xFE00 && address <= 0xFE9F) {
        if(!bootRomActive) {
            printf("");
        }
        
        oam.write8(address - 0xFE00, data);
    } else if(address >= 0xFEA0 && address <= 0xFEFF) {
        // Not usable
    } else if (address >= 0xFF00 && address <= 0xFF7F) {
        writeIO(address, data);
    } else if (address >= 0xFF80 && address <= 0xFFFE) {
        hram.write8(address & 0x007F, data);
    } else if (address == 0xFFFF) {
        interruptHandler.write8(address, data);
    } else {
        printf("Unknown write address %x - %x\n", address, data);
        std::cerr << "";
    }
}

void MMU::writeIO(uint16_t address, uint8_t data) {
    // https://gbdev.io/pandocs/Hardware_Reg_List.html
    
     if(address == 0xFF00) {
        joypad.write8(address, data);
    } else if(address >= 0xFF01 && address <= 0xFF02) {
        serial.write8(address, data);
    } else if(address >= 0xFF04 && address <= 0xFF07) {
        timer.write8(address, data);
    } else if(address == 0xFF0F) {
        interruptHandler.write8(address, data);
    } else if(address >= 0xFF10 && address <= 0xFF26) {
        apu.write8(address, data);
    } else if(address >= 0xFF30 && address <= 0xFF3F) {
        apu.write8(address, data);
    } else if(address >= 0xFF40 && address <= 0xFF4B) {
        //std::cerr << "LCD Control, Status, Position, Scrolling, and Palettes\n";
        
        if(address >= 0xFF40 && address <= 0xFF45 || address >= 0xFF4A && address <= 0xFF4B) {
            bool wasEnabled = lcdc.enable;
            
            /* SCX ISSUE
             * 
            if(address == 0xFF43 && PPU::mode == PPU::HBlank && data == 0) {
                /**
                 * So there really isn't any mention of this..
                 * As I'm too lazy to debug it further (it's been 6 hours..).
                 *
                 * It seems while playing "Super Mario Land" for example,
                 * that when you move to the right, it writes an opcode,
                 * 0xE0 - LD (FF00 + U8), A.
                 *
                 * But the value of A is always 0, so it's kind of just,
                 * doing this glitch where SCX is let's say 4, then it,
                 * goes back to 0, then 4 again, then 0. Etc...
                 *
                 * Which causes this weird sudden change mid-scanline...
                 *
                 * THIS APPARENTLY DOESN'T FIX IT YAAAYY ;-;
                 #1#
                return;
            }*/
            
            /**
             * After debugging the SCX issue further,
             *  
             * I have noticed that if I ignore writes of,
             * 0 to SCX, the top winodw doesn't move alongside,
             * the character. So it seems like it's setting it,
             * to be 0 for the top part of the screen, then,
             * the origianl value for the game.
             * 
             * But I am messeing up something somewhere,
             * so luckily I wont have to trace a CPU isntruction!!!
             *
             * OK so in the end it was just.. a halt issue :DDDDDDDDDDDD
             */
            
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
            lastDma = data;
            
            //dma.activate(static_cast<uint16_t>(data) << 8);
            
            uint16_t u16 = static_cast<uint16_t>(data) << 8;
            
            for(uint16_t i = 0; i < 160; i++) {
                uint8_t val = fetch8(u16 + i);
                write8(0xFE00 + i, val);
            }
        } else if(address >= 0xFF47 && address <= 0xFF49) {
            // https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
            // https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
            ppu.write8(address, data);
        }
    } else if(address == 0xFF4D) {
        // CGB Mode only
        if(Cartridge::mode != Color) return;
        
        // Bit 7 - Current Speed
        doubleSpeed = check_bit(data, 7);
        
        // Bit 0 - Switch armed
        switchArmed = check_bit(data, 0);
    } else if(address == 0xFF4F) {
       vram.write8(address, data);
    } else if(address == 0xFF50) {
        //std::cerr << "Set to non-zero to disable boot ROM\n";
    } else if(address == 0xFF51) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff51ff52--hdma1-hdma2-cgb-mode-only-vram-dma-source-high-low-write-only
        if(Cartridge::mode != Color) return;
        
        sourceLow = data;
        //source = (source & 0xFF) | static_cast<uint8_t>(data << 8);
    } else if(address == 0xFF52) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff51ff52--hdma1-hdma2-cgb-mode-only-vram-dma-source-high-low-write-only
        if(Cartridge::mode != Color) return;
        
        sourceHigh = data & 0xF0;
        //source = (source & 0xFF00) | (data);
    } else if(address == 0xFF53) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff53ff54--hdma3-hdma4-cgb-mode-only-vram-dma-destination-high-low-write-only
        if(Cartridge::mode != Color) return;
        
        destLow = data & 0x1F;
        //dest = (dest & 0xFF) | static_cast<uint8_t>(data << 8);
    } else if(address == 0xFF54) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff53ff54--hdma3-hdma4-cgb-mode-only-vram-dma-destination-high-low-write-only
        if(Cartridge::mode != Color) return;
        
        destHigh = data & 0xF0;
        //dest = (dest & 0xFF00) | data;
    } else if(address == 0xFF55) {
        // https://gbdev.io/pandocs/CGB_Registers.html#ff55--hdma5-cgb-mode-only-vram-dma-lengthmodestart
        if(Cartridge::mode != Color) return;
        
        if(/*enabled && */mode == 1 && (data & 0x80) == 0) {
            enabled = false;
            mode = 0;
            
            return;
        }
        
        // mode = 1 (H-Blank DMA), mode = 0 (GDMA)
        if((data & 0x80) == 0x80) {
            printf("");
        }
        
        mode = (data & 0x80) >> 7;
        
        length = data & 0x7F;
        
        enabled = true;
        
        sourceIndex = static_cast<uint16_t>(sourceLow << 8) | static_cast<uint16_t>(sourceHigh);
        destIndex = (static_cast<uint16_t>(destLow << 8) | static_cast<uint16_t>(destHigh)) | 0x8000;
        
        printf("");
    } else if(address >= 0xFF68 && address <= 0xFF6C) {
        ppu.write8(address, data);
    } else if(address == 0xFF70) {
        if(Cartridge::mode != Color) return;
        
        wramBank = (data & 0x7) == 0 ? 1 : static_cast<uint8_t>(data & 0x7);
    }
    
    // TODO; Addresses that doesn't exist
    else if(address != 0xFF03 && address != 0xFF08 && address != 0xFF0A &&
            address != 0xFF0B && address != 0xFF0C && address != 0xFF0D &&
            address != 0xFF0E && address != 0xFF27 && address != 0xFF28 &&
            address != 0xFF2A && address != 0xFF2B && address != 0xFF2C &&
            address != 0xFF2D && address != 0xFF2E && address != 0xFF2F) {
        printf("IO Write Address; %x = %x\n", address, data);
        std::cerr << "";
    }
}

void MMU::write16(uint16_t address, uint16_t data) {
    write8(address, data & 0xFF);
    write8(address + 1, (data >> 8) & 0xFF);
}

void MMU::switchSpeed() {
    if(switchArmed) {
        doubleSpeed = !doubleSpeed;
    }
    
    switchArmed = false;
}

void MMU::clear() {
    // TODO;
    //std::fill(std::begin(memory), std::end(memory), 0);
}

