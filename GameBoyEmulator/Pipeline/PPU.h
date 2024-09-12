﻿#pragma once
#include <array>
#include <cstdint>
#include <SDL_render.h>
#include <SDL_video.h>
#include <tuple>

/**
 * https://gbdev.io/pandocs/Tile_Data.html#vram-tile-data
 */
struct TileData {
	uint8_t nr;
	uint8_t data[8][8];
};

/**
 * https://gbdev.io/pandocs/Tile_Maps.html
 */
struct TileMap {
	uint8_t data[32][32];
};

struct Pixel {
	uint8_t color;
	uint8_t palette;
	bool spritePriority;
	bool bgPriority;
};

class VRAM;
class OAM;

class LCDC;

class MMU;

class PPU {
public:
	PPU(VRAM& vram, OAM& oam, LCDC& lcdc, MMU& mmu)
		: vram(vram),
		  oam(oam),
		  lcdc(lcdc),
          mmu(mmu) {
		
	}
	
	void tick(const int& cycles);
	
	//void renderTile(const TileData& tileData, int x, int y, const uint8_t* palette);
	void fetchBackground();
	void fetchSprites();
	
	void write8(uint16_t address, uint8_t data);
	
	void createWindow();
	
	void updatePixel(uint8_t x, uint8_t y, Uint32 color);
	
	// TODO;
	
	const uint32_t COLOR_WHITE = 0xFFFFFF;  // White
	const uint32_t COLOR_LIGHT_GRAY = 0xC0C0C0;  // Light Gray
	const uint32_t COLOR_DARK_GRAY = 0x404040;  // Dark Gray
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
	
private:
	VRAM& vram;
	OAM& oam;
	
	LCDC& lcdc;
	
	MMU& mmu;
	
private:
	uint32_t cycle;
	uint8_t mode;
	
	uint8_t scanLineY = 0;
	
	std::array<Pixel, 8> bgFifo;
	std::array<Pixel, 8> spriteFifo;
	
public:
	uint8_t BGPalette[4];
	uint8_t OBJ0Palette[4];
	uint8_t OBJ1Palette[4];
	
public:
	SDL_Window* window;
	SDL_GLContext sdl_context;
	SDL_Renderer* renderer;
	SDL_Surface* surface;
	
	SDL_Texture* mainTexture;
};
