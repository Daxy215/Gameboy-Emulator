#pragma once
#include <cstdint>

/*
 * VRAM can be locked
 */

class LCDC;
struct TileData;

class VRAM {
public:
	VRAM(LCDC& lcdc) : lcdc(lcdc) {}
	
    uint8_t fetch8(uint16_t address);
    void write8(uint16_t address, uint8_t data);
private:
	LCDC& lcdc;
	
    uint8_t RAM[0x4000] = { 0 };
};
