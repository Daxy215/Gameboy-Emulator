﻿#include "MMU.h"

#include <iomanip>

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
    //bootRomActive = (Cartridge::mode == DMG);
}

void MMU::tick(uint32_t cycles) {
    apu.tick(cycles);
    
    for(int i = 0; i < dmas.size(); i++) {
        // Begin transfering one byte at a time
        dmas[i].process(*this, cycles * (doubleSpeed ? 2 : 1));
        
        // If DMA is no longer active, delete it
        if(!dmas[i].active) {
            dmas.erase(dmas.begin() + i);
        }
    }
    
    // HDMA
    // TODO; Check if this correct
    // TODO; Please rewrite this
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

uint8_t MMU::fetch8(uint16_t address, bool isDma) {
    /**
     * Information of memory map is taken from;
     * https://gbdev.io/pandocs/Memory_Map.html
     */
    
    if(bootRomActive && address < 0x100) {
        return bootRom[address];
    }
    
    if(address <= 0x7FFF) {
        auto dma = isWithinRange(0, 0x7FFF, dmas);
        
        if(!isDma && dma.active) {
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }
        
        return mbc.read(address);
    } else if(address >= 0x8000 && address <= 0x9FFF) {
        return vram.fetch8(address/* - 0x8000*/);
    } else if (address >= 0xA000 && address <= 0xBFFF) {
        auto dma = isWithinRange(0xA000, 0xBFFF, dmas);
        
        if(!isDma && dma.active) {
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }
        
        return mbc.read(address);
    } else if(address >= 0xC000 && address <= 0xCFFF) {
        auto dma = isWithinRange(0xC000, 0xCFFF, dmas);
        
        if(!isDma && dma.active) {
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }
        
        return wram.fetch8(address - 0xC000);
    } else if(address >= 0xD000 && address <= 0xDFFF) {
        auto dma = isWithinRange(0xD000, 0xDFFF, dmas);
        
        if(!isDma && dma.active) {
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }
        
        return wram.fetch8((wramBank * 0x1000) | (address & 0x0FFF));
    } else if(address >= 0xE000 && address <= 0xFDFF) {
        auto dma = isWithinRange(0xE000, 0xFDFF, dmas);
        
        if(!isDma && dma.active) {
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }
        
        return wram.fetch8(address & 0x0FFF);
    } else if(address >= 0xFE00 && address <= 0xFE9F) {
        auto dma = isWithinRange(0xFE00, 0xFE9F, dmas);
        
        if(!isDma && dma.active) {
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }
        
        return oam.fetch8(address);
    } else if(address >= 0xFEA0 && address <= 0xFEFF) {
        // Not usable
    } else if(address >= 0xFF00 && address <= 0xFF7F) {
        /*auto dma = isWithinRange(0xFE00, 0xFF7F, dmas);
        
        if(!isDma && dma.active) {
            //assert(false);
            // DMA Conflict
            uint8_t val = fetch8(dma.source + dma.index, true);
            return val;
        }*/
        
        return fetchIO(address);
    } else if(address >= 0xFF80 && address <= 0xFFFE) {
        return hram.fetch8(address - 0xFF80);
    } else if(address == 0xFFFF) {
        return interruptHandler.fetch8(address);
    } else {
        printf("Unknown fetch address %x\n", address);
        std::cerr << "";
    }
    
    return 0xFF;
}

uint8_t MMU::fetchIO(uint16_t address, bool isDma) {
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

void MMU::write8(uint16_t address, uint8_t data, bool isDma) {
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
        oam.write8(address - 0xFE00, data);
    } else if(address >= 0xFEA0 && address <= 0xFEFF) {
        // Not usable
    } else if (address >= 0xFF00 && address <= 0xFF7F) {
        writeIO(address, data);
    } else if (address >= 0xFF80 && address <= 0xFFFE) {
        hram.write8(address - 0xFF80, data);
    } else if (address == 0xFFFF) {
        interruptHandler.write8(address, data);
    } else {
        printf("Unknown write address %x - %x\n", address, data);
        std::cerr << "";
    }
}

void MMU::writeIO(uint16_t address, uint8_t data, bool isDma) {
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
            
            lcdc.write8(address, data);
            
            if(address == 0xFF40) {
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
                // TODO; NOT ALLOWED?
            }*/
            
            //  $FF46	DMA	OAM DMA source address & start
            lastDma = data;
            
            // TODO; I haven't emulated DMA Conflict for
            // CGB just yet, and this messes it up, so,
            // now now, I need to instantly cause a transfer,
            // rather than the more "accurate" way

            if(Cartridge::mode == Mode::DMG) {
                // TODO; Not sure if this is correct?
                // but I feel like if another DMA is being,
                // transfered and another DMA is at the same address,
                // it should just restart it?
                // After doing this some of the tests that used to crash,
                // doesn't crash anymore so I suppose is correct in some way?

                // TODO; Ik this is funky
                DMA dma;
                dma.setSource(data);

                dma = isWithinRange(dma.source, dma.source, dmas);
                bool exists = dma.active;

                // Whether it exists or not, active it again
                dma.activate(data/*static_cast<uint16_t>(data << 8)*/);

                // Only add it again if it's active
                if(!exists)
                    dmas.push_back(dma);

                return;
            }
            
            uint16_t u16 = static_cast<uint16_t>(data << 8);
            
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
    } else if(address >= 0xFF68 && address <= 0xFF6C) {
        ppu.write8(address, data);
    } else if(address == 0xFF70) {
        if(Cartridge::mode != Color) return;
        
        wramBank = data & 0x7;

        // Only banks 1-7 matters
        if(wramBank == 0)
            wramBank = 1;
    }
    
    // TODO; Addresses that don't exist
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
