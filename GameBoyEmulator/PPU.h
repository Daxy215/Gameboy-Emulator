#pragma once
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
	
	void pushPixelsToLCDC();
	
	void write8(uint16_t address, uint8_t data);
	
	void createWindow();
	
	uint8_t getPixelColor(const TileData& data, int row, int col) const;
	void updatePixel(SDL_Texture* texture, uint8_t x, uint8_t y, Uint32 color);
	
	std::tuple<unsigned char, unsigned char, unsigned char> GetRGB(unsigned char color);
	
	Uint32 RGBToUint32(uint8_t r, uint8_t g, uint8_t b) {
		return (0xFF << 24) | (r << 16) | (g << 8) | b;
	}
	
	SDL_Texture* createTexture(uint8_t width, uint8_t height);

	template<typename T, size_t N>
	void pop_front(std::array<T, N>& arr) {
		// Shift all elements to the left
		for(size_t i = 1; i < N; i++) {
			arr[i - 1] = arr[i];
		}

		arr[N - 1] = T(); // Default initialize the last element
	}
	
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
	
	SDL_Texture* mainTexture;
};
