#pragma once
#include <vector>

#include "../IO/Joypad.h"

class InterruptHandler;
class Joypad;
class Serial;
class Timer;

class MBC;
class WRAM;
class HRAM;

class PPU;
class LCDC;
class VRAM;
class OAM;

class APU;

class MMU {
public:
    /**
     * TODO; If a DMA transfer already is active,
     * some roms still send another transfer?
     * Maybe it isn't meant to reset but rather to,
     * process them all?
     */
    struct DMA {
        void process(MMU& mmu, uint32_t cycles) {
            if(active) {
                // DMA has a 4 T-Cycles initial delay
                if(initialDelay > 0) {
                    initialDelay -= cycles;
                
                    // Ik scummy way of doing this
                    if(initialDelay <= 0) {
                        cycles -= 4;
                    } else {
                        return;
                    }
                }
            
                remainingCycles -= cycles;
                ticks += cycles;
            
                // Transfer every 4 T-Cycles
                uint8_t transfersToProcess = cycles / 4;
            
                while(transfersToProcess-- > 0) {
                    uint8_t val = mmu.fetch8(source + index);
                    mmu.write8(0xFE00 + index, val);
                    
                    index++;
                }
            
                // Disable after 644 T-Cycles
                if(remainingCycles <= 0) {
                    //assert(ticks == 640);
                    //assert(dma.index == 160);
                
                    active = false;
                    index = 0;
                
                    ticks = 0;
                }
            }
        }
    
        void activate(uint16_t source) {
            this->source = source;
        
            // Transfers 4 bytes every 4 T-cycles
            remainingCycles = 640;
        
            // DMA has 4 T-Cycles delay
            initialDelay = 4;
        
            index = 0;
        
            active = true;
        }
    
        uint16_t source = 0;
        uint16_t index = 0;
        uint32_t remainingCycles = 0;
        int8_t initialDelay = 0;
    
        // Used for testing
        uint32_t ticks = 0;
    
        bool active = false;
    };
    
public:
    MMU(InterruptHandler& interruptHandler, Serial& serial, Joypad& joypad, MBC& mbc, WRAM& wram,
        HRAM& hram, VRAM& vram, LCDC& lcdc, Timer& timer, OAM& oam, PPU& ppu, APU& apu,
        const std::vector<uint8_t>& bootRom, const std::vector<uint8_t>& memory);
    
    void tick(uint32_t cycles);
    
    uint8_t fetch8(uint16_t address);
    uint8_t fetchIO(uint16_t address);
    
    uint16_t fetch16(uint16_t address);
    
    void write8(uint16_t address, uint8_t data);
    void writeIO(uint16_t address, uint8_t data);
    
    void write16(uint16_t address, uint16_t data);
    
    void switchSpeed();
    void clear();
    
public:
    uint16_t cycles = 0;
    
    bool bootRomActive = false;
    
private:
    
    uint8_t wramBank = 1;
    
    uint8_t lastDma = 0;
    
    // Prepare speed switch thingies
    //uint8_t key1 = 0;

public:
    // CGB Registers
    
    /**
     * false - Normal
     * true - Double
     */
    bool doubleSpeed = false;
    
    bool switchArmed = false;
    
    // HDMA
    // TODO; Make this a struct
    uint8_t sourceLow = 0, sourceHigh = 0;
    uint8_t destLow = 0, destHigh = 0;
    uint8_t mode = 0;
    uint8_t length = 0xFF;
    uint16_t sourceIndex = 0;
    uint16_t destIndex = 0;
    //uint8_t lastLY = 0;
    //uint8_t tillNextByte = 0;
    
    bool enabled = false;
    //bool active = true;
    //bool waiting = false;
    //bool completed = false;
    
    //bool notifyHBlank = false;
    
private:
    // I/O
    InterruptHandler& interruptHandler;
    
public: // TODO; Im too lazy! im sorry..
    Serial& serial;
    
    Joypad& joypad;
    
public:
    Timer& timer;
    
    // Memories
    MBC& mbc;
    WRAM& wram;
    HRAM& hram;
    VRAM& vram;
    
    LCDC& lcdc;
    OAM& oam;
    
public:
    //DMA dma;
    std::vector<DMA> dmas;
    
public:
    PPU& ppu;
    
    APU& apu;
    
    std::vector<uint8_t> bootRom;
};
