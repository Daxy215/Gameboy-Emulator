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
    MMU(InterruptHandler& interruptHandler, Serial& serial, Joypad& joypad, MBC& mbc, WRAM& wram, HRAM& hram, VRAM& vram, LCDC& lcdc, Timer& timer, OAM& oam, PPU& ppu, APU& apu, const std::vector<uint8_t>& memory)
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
          apu(apu) {
    }

    void tick();

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
    uint16_t source = 0xFFFF;
    uint16_t dest = 0xFFFF;
    uint8_t mode = 1;
    uint8_t length = 0x7F;
    uint16_t sourceIndex = 0;
    uint16_t destIndex = 0;
    uint8_t lastLY = 0;
    uint8_t tillNextByte = 0;
    
    bool enabled = false;
    bool active = true;
    bool waiting = false;
    bool completed = false;
    
    bool notifyHBlank = false;
    
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
    PPU& ppu;
    
    APU& apu;
};
