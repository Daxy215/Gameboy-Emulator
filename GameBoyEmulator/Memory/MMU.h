﻿#pragma once
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
    MMU(InterruptHandler& interruptHandler, Joypad& joypad, MBC& mbc, WRAM& wram, HRAM& hram, VRAM& vram, LCDC& lcdc, Serial& serial, Timer& timer, OAM& oam, PPU& ppu, APU& apu, const std::vector<uint8_t>& memory)
        : interruptHandler(interruptHandler),
          joypad(joypad),
          serial(serial),
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
    
    // Prepare speed switch thingies
    uint8_t key1 = 0;

public:
    // CGB Registes
    
    /**
     * false - Normal
     * true - Double
     */
    bool doubleSpeed = false;
    
    bool switchArmed = false;
    
private:
    // I/O
    InterruptHandler& interruptHandler;

public: // TODO; Im too lazy! im sorry..
    Joypad& joypad;
    
    Serial& serial;
    
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
