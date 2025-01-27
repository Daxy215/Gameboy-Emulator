#pragma once
#include <cstdint>
#include <SDL_render.h>
#include <SDL_video.h>

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

	enum BGPriority {
		None,
		Priority,
		Zero
	};
	
public:
	PPU(VRAM& vram, OAM& oam, LCDC& lcdc, MMU& mmu)
		: vram(vram),
		  oam(oam),
		  lcdc(lcdc),
		  mmu(mmu) {
		
	}
	
	void tick(int cycles);
	void updateMode(PPUMode mode);
	
	void drawScanline();
	void drawBackground();
	void drawSprites();
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
	void checkLYCInterrupt();
	
	void createWindow();
	
	void updatePixel(uint32_t x, uint32_t y, uint32_t color);
	void setPixel(uint32_t x, uint32_t y, uint32_t color);
	void reset(const uint32_t& clock);
	
	// TODO; Remove
	
	uint32_t convertRGB555ToSDL(uint16_t color) {
		uint8_t r = (color >> 0) & 0x1F;         // 0-4   - Red
		uint8_t g = (color >> 5) & 0x1F;         // 5-9   - Green
		uint8_t b = (color >> 10) & 0x1F;        // 10-14 - Blue
		
		r = (r * 255) / 31;
		g = (g * 255) / 31;
		b = (b * 255) / 31;
		
		// Return combined 32-bit color
		return (static_cast<uint32_t>(0xFF) << 24) | (r << 16) | (g << 8) | b;
	}
	
	const uint32_t COLOR_WHITE = 0xFFFFFF;  // White
	const uint32_t COLOR_LIGHT_GRAY = 0xAAAAAA;  // Light Gray
	const uint32_t COLOR_DARK_GRAY = 0x505050;  // Dark Gray
	const uint32_t COLOR_BLACK = 0x000000;  // Black
	
	/*const uint32_t COLOR_WHITE = 0x9A9E3F;  // White
	const uint32_t COLOR_LIGHT_GRAY = 0x496B22;  // Light Gray
	const uint32_t COLOR_DARK_GRAY = 0x0E450B;  // Dark Gray
	const uint32_t COLOR_BLACK = 0x1B2A09;  // Black*/
	
	uint32_t paletteIndexToColor(uint8_t index) {
		switch (index) {
		case 0: return COLOR_WHITE;
		case 1: return COLOR_LIGHT_GRAY;
		case 2: return COLOR_DARK_GRAY;
		case 3: return COLOR_BLACK;
		default: return COLOR_WHITE;
		}
	}
	
	SDL_Texture* createTexture(uint32_t width, uint32_t height);

public:
	static PPUMode mode;

private:
	//bool test = false;
	uint32_t currentDot = 0;
	uint32_t winLineCounter = 0;
	
	bool drawWindow = false;
	
	BGPriority bgPriority[160] = { Zero };
	
public:
	uint8_t interrupt = 0;
	
private:
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
	
	// CGB
	
	// Background
	uint8_t CBGPalette[64];
	uint8_t bgIndex = 0;
	
	// Sprite
	uint8_t COBJPalette[64];
	uint8_t objIndex = 0;
	
	// BCPS/BGPI
	
	/**
	 * 0 = Disabled
	 * 1 = Increment after writing to GCPD / 0CPD
	 */
	bool autoIncrementBG = false;
	bool autoIncrementOBJ = false;
	
	bool opri = false;
	
public:
	SDL_Window* window;
	SDL_GLContext sdl_context;
	SDL_Renderer* renderer;
	SDL_Surface* surface;
	
	SDL_Texture* texture;
	
	int pitch;
	uint32_t* pixels;
};
