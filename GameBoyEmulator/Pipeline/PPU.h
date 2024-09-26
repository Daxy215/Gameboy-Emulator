#pragma once
#include <cstdint>
#include <SDL_render.h>
#include <SDL_video.h>

#include "Fetcher.h"

/*struct Pixel {
	uint8_t color;
	uint8_t palette;
	bool spritePriority;
	bool bgPriority;
};*/

class VRAM;
class OAM;

class LCDC;

class MMU;

class PPU {
public:
	enum PPUMode {
		HBlank       = 0,
		VBlank		 = 1,
		OAMScan      = 2,
		VRAMTransfer = 3
	};

public:
	PPU(VRAM& vram, OAM& oam, LCDC& lcdc, MMU& mmu)
		: fetcher(mmu),
		  vram(vram),
		  oam(oam),
		  lcdc(lcdc),
		  mmu(mmu) {
		
	}
	
	void tick(const int& cycles);
	
	void drawScanline();
	void drawBackground();
	void drawSprites();
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
	void createWindow();
	
	void updatePixel(uint8_t x, uint8_t y, uint32_t color);
	void setPixel(uint8_t x, uint8_t y, uint32_t color);
	void reset(const uint32_t& clock);

	// TODO; Remove
	
	const uint32_t COLOR_WHITE = 0xFFFFFF;  // White
	const uint32_t COLOR_LIGHT_GRAY = 0xAAAAAA;  // Light Gray
	const uint32_t COLOR_DARK_GRAY = 0x505050;  // Dark Gray
	const uint32_t COLOR_BLACK = 0x000000;  // Black
	
	uint32_t paletteIndexToColor(uint8_t index) {
		switch (index) {
		case 0: return COLOR_WHITE;
		case 1: return COLOR_LIGHT_GRAY;
		case 2: return COLOR_DARK_GRAY;
		case 3: return COLOR_BLACK;
		default: return COLOR_WHITE;
		}
	}
	
	SDL_Texture* createTexture(uint8_t width, uint8_t height);

public:
	static PPUMode mode;

private:
	//bool test = false;
	uint32_t clock = 0;
	uint32_t winLineCounter = 0;
	
	bool drawWindow = false;
	
	bool bgPriority[160] = { false };
public:
	uint8_t interrupt;

	//bool wyTrigger = false;
	
private:
	Fetcher fetcher;
	
	VRAM& vram;
	OAM& oam;
	
public:
	LCDC& lcdc;
	
private:
	MMU& mmu;
	
public:
	uint8_t bgp = 0;
	uint8_t obj0 = 0;
	uint8_t obj1 = 0;
	
	uint8_t BGPalette[4];
	uint8_t OBJ0Palette[4];
	uint8_t OBJ1Palette[4];
	
private:
	SDL_Window* window;
	SDL_GLContext sdl_context;
	SDL_Renderer* renderer;
	SDL_Surface* surface;
};
