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

struct DMA {
    void activate(uint8_t& source) {
        this->source = source;
        
        // 160 M cycles - 640 T cycles
        remainingCycles = 640;
        
        active = true;
    }
    
    uint8_t source = 0;
    uint32_t remainingCycles = 0;
    
    bool active = false;
};

class MMU {
public:
    MMU(InterruptHandler& interruptHandler, Serial& serial, Joypad& joypad, MBC& mbc, WRAM& wram,
        HRAM& hram, VRAM& vram, LCDC& lcdc, Timer& timer, OAM& oam, PPU& ppu, APU& apu,
        const std::vector<uint8_t>& bootRom, const std::vector<uint8_t>& memory)
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
        
    }
    
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

private:
    DMA dma;
    
public:
    PPU& ppu;
    
    APU& apu;
    
    std::vector<uint8_t> bootRom;
};
