#pragma once
#include <cstdint>
#include <vector>

class InterruptHandler;

class WRAM;
class HRAM;
class VRAM;
class ExternalRAM;

class LCDC;
class Serial;

class OAM;
class PPU;

class MMU {
public:
    MMU(InterruptHandler& interruptHandler, WRAM& wram, HRAM& hram, VRAM& vram, ExternalRAM& externalRam, LCDC& lcdc, Serial& serial, OAM& oam, PPU& ppu, const std::vector<uint8_t>& memory)
        : interruptHandler(interruptHandler),
          wram(wram),
          hram(hram),
          vram(vram),
          externalRam(externalRam),
          lcdc(lcdc),
          serial(serial),
          oam(oam), ppu(ppu),
          memory(memory) {
        
    }
    
    uint8_t fetch8(uint16_t address);
    uint8_t fetchIO(uint16_t address);
    
    uint16_t fetch16(uint16_t address);
    
    void write8(uint16_t address, uint8_t data);
    void writeIO(uint16_t address, uint8_t data);
    
    void write16(uint16_t address, uint16_t data);
    
    void clear();
    
private:
    //TODO; Move to RAM
    bool m_MBC1 = true;
    bool m_MBC2 = false;
    bool m_EnableRAM = false;
    bool m_ROMBanking = true;
    uint8_t m_CurrentROMBank = 1;
    uint8_t m_CurrentRAMBank = 0;
    uint8_t wramBank         = 1;
    //uint8_t m_RAMBanks[0x8000] = { 0 };
    
    InterruptHandler& interruptHandler;
    
    // Memories
    WRAM& wram;
    HRAM& hram;
    VRAM& vram;
    ExternalRAM& externalRam;
    
    // I/O
    LCDC& lcdc;
    Serial& serial;
    
    OAM& oam;

public:
    PPU& ppu;
    
public:
    std::vector<uint8_t> memory;
};
