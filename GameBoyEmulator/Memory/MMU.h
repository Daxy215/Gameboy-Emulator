#pragma once
#include <vector>

class MBC;
class InterruptHandler;

class WRAM;
class HRAM;
class VRAM;
class ExternalRAM;

class LCDC;
class Serial;
class Timer;

class OAM;
class PPU;

class MMU {
public:
    MMU(InterruptHandler& interruptHandler, MBC& mbc, WRAM& wram, HRAM& hram, VRAM& vram, ExternalRAM& externalRam, LCDC& lcdc, Serial& serial, Timer& timer, OAM& oam, PPU& ppu, const std::vector<uint8_t>& memory)
        : interruptHandler(interruptHandler),
          mbc(mbc),
          wram(wram),
          hram(hram),
          vram(vram),
          externalRam(externalRam),
          lcdc(lcdc),
          serial(serial),
          timer(timer),
          oam(oam),
          ppu(ppu) {
        
    }
    
    uint8_t fetch8(uint16_t address);
    uint8_t fetchIO(uint16_t address);
    
    uint16_t fetch16(uint16_t address);
    
    void write8(uint16_t address, uint8_t data);
    void writeIO(uint16_t address, uint8_t data);
    
    void write16(uint16_t address, uint16_t data);
    
    void clear();
    
private:
    uint8_t wramBank = 1;
    
    // Prepare speed switch thingies
    uint8_t key1;
    
    InterruptHandler& interruptHandler;
    
    MBC& mbc;
    
    // Memories
    WRAM& wram;
    HRAM& hram;
    VRAM& vram;
    ExternalRAM& externalRam;
    
    // I/O
    LCDC& lcdc;
    Serial& serial;
    Timer& timer;
    
    OAM& oam;

public:
    PPU& ppu;
    
/*public:
    std::vector<uint8_t> memory;*/
};
