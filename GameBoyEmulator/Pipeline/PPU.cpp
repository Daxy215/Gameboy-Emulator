#include "PPU.h"

#include <iostream>

#include <GL/glew.h>
#include <GL/wglew.h>

#include "LCDC.h"
#include "OAM.h"
#include "SDL.h"
#include "VRAM.h"

#include "../Memory/MMU.h"

const int WIDTH = 640;
const int HEIGHT = 480;

PPU::PPUMode PPU::mode = PPU::HBlank;

void PPU::tick(const int& cycles = 4) {
	this->clock += cycles;
	
	//uint8_t LCDControl = mmu.fetch8(0xFF40);
	uint8_t& LY = lcdc.LY;//mmu.fetch8(0xFF44);
	//uint8_t WY = mmu.fetch8(0xFF4A);
	/*uint8_t LYC = mmu.fetch8(0xFF45);
	uint8_t SCY = mmu.fetch8(0xFF42);
	uint8_t SCX = mmu.fetch8(0xFF43);
	uint8_t WX = mmu.fetch8(0xFF4B);*/
	
	// Following ;https://hacktix.github.io/GBEDG/ppu/#h-blank-mode-0
	switch (mode) {
	case HBlank:
		while(clock >= 204) {
			//mmu.write8(0xFF44, ++LY);
			LY++;
			
			if(LY == 144) {
				// VBlank interrupt
				interrupt |= 0x01;
				frames++;
				
				mode = VBlank;
				SDL_RenderPresent(renderer);
			} else {
				mode = OAMScan;
			}
			
			clock -= 204;
		}
		
		break;
	case VBlank:
		while(clock >= 4560) {
			test = false;
			//mmu.write8(0xFF44, ++LY);
			LY++;
			
			if(LY >= 154) {
				//mmu.write8(0xFF44, LY = 0);
				LY = 0;
				mode = OAMScan;
			}
			
			clock -= 4560;
		}
		
		break;
	case OAMScan:
		//if(LY + 16 >= spriteY && LY + 16 < spriteY + spriteHeight) {}
		
		while(clock >= 80) {
			fetchSprites();
			
			mode = VRAMTransfer;
			clock -= 80;
		}
		
		break;
	case VRAMTransfer:
		while(clock >= 172) {
			drawBackground();
			
			/*if(LY == WY) {
				mmu.write8(0xFF4A, 0);
				test = true;
			}*/
			
			mode = HBlank;
			clock -= 172;
		}
		
		break;
	}
}

void PPU::drawBackground() {
    uint8_t LY = mmu.fetch8(0xFF44);
    uint8_t SCY = mmu.fetch8(0xFF42);
    uint8_t SCX = mmu.fetch8(0xFF43);
	uint8_t WY = mmu.fetch8(0xFF4A);
	uint8_t WX = mmu.fetch8(0xFF4B);
	
	uint16_t tileWinMapBase = lcdc.windowTileMapArea  ? 0x9C00 : 0x9800;
	uint16_t tileBGMapBase  = lcdc.bgTileMapArea      ? 0x9C00 : 0x9800;
	uint16_t tileBGMap      = lcdc.bgWinTileDataArea  ? 0x8000 : 0x8800;
	
	if(!lcdc.bgWindowEnabled)
		return;
	
	int32_t winY = WY;
	
	/*if(lcdc.windowEnabled && WX <= 166) {
		mmu.write8(0xFF4A, ++WY);
		winY = WY;
	} else {
		winY = -1;
	}*/
	
	uint8_t bgY = SCY + LY;
	
	uint16_t tilemapAddr;
	uint16_t tileX, tileY, pY;
	uint8_t pX;
	
	// 160 = Screen width
	for(size_t x = 0; x < 160; x++) {
		int32_t winX = (static_cast<int32_t>(WX) - 7) + static_cast<int32_t>(x);
		uint32_t bgX = (static_cast<uint32_t>(SCX) + static_cast<uint32_t>(x)) % 256;
		
		if(lcdc.windowEnabled && winX >= 0 && winY >= 0) {
			mmu.write8(0xFF4A, ++WY);
			
			tilemapAddr = tileWinMapBase;
			
			tileY = (static_cast<uint16_t>(winY) >> 3) & 31;
			tileX = (static_cast<uint16_t>(winX) >> 3) & 31;
			
			pY = winY & 0x07; // % 8
			pX = static_cast<uint8_t>(winX) & 0x07; // % 8
		} else {
			tilemapAddr = tileBGMapBase;
			
			tileY = bgY >> 3 & 31;
			tileX = bgX >> 3 & 31;
			
			pY = bgY & 0x07;
			pX = static_cast<uint8_t>(bgX) & 0x07;
		}
		
		uint8_t tileID = mmu.fetch8(tilemapAddr + tileY * 32 + tileX);
		
		uint16_t offset;
		
		// https://gbdev.io/pandocs/Tile_Data.html?highlight=signed#vram-tile-data
		
		if(tileBGMap == 0x8000) {
			offset = tileBGMap + (static_cast<uint16_t>(tileID)) * 16;
		} else {
			offset = tileBGMap + static_cast<uint16_t>(static_cast<int16_t>(static_cast<int8_t>(tileID) + 128)) * 16;
		}
		
		// Tiles are flipped by default
		pX = 7 - pX;
		
		uint16_t address = offset + (pY * 2);
		uint8_t d0 = mmu.fetch8(address);
		uint8_t d1 = mmu.fetch8(address + 1);
		
		uint8_t pixel = (d0 >> pX) & 1;
		pixel |= (((d1 >> pX) & 1) << 1);
		
		uint8_t pixelColor = (bgp >> (pixel * 2)) & 0x03;
		
		updatePixel(static_cast<uint8_t>(x), LY, paletteIndexToColor(pixelColor));
	}
}

void PPU::fetchSprites() {
	uint8_t LCDControl = mmu.fetch8(0xFF40);
    uint8_t LY = lcdc.LY; //mmu.fetch8(0xFF44);
	
	
}

void PPU::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF47) {
		// https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
		
		bgp = data;
		
		for(int i = 0; i < 4; i++) {
			BGPalette[i] = (bgp >> (i * 2)) & 0x03;
		}
		
	} else if(address == 0xFF48 || address == 0xFF49) {
		// https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
		
		for(int i = 0; i < 4; i++) {
			if(address == 0xFF48)
				OBJ0Palette[i] = (data >> (i * 2)) & 0x03;
			else
				OBJ1Palette[i] = (data >> (i * 2)) & 0x03;
		}
	}
}

void PPU::createWindow() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL could not initialize SDL_Error: " << SDL_GetError() << '\n';
        return;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    window = SDL_CreateWindow("Game Boy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    
    if (window == nullptr) {
        std::cerr << "Window could not be created SDL_Error: " << SDL_GetError() << '\n';
        return;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, GL_CONTEXT_FLAG_DEBUG_BIT);
    
    sdl_context = SDL_GL_CreateContext(window);
    if (sdl_context == nullptr) {
        std::cerr << "OpenGL context could not be created SDL_Error: " << SDL_GetError() << '\n';
        return;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    
    GLenum err = glewInit();
    if (err!= GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << '\n';
        return;
    }
    
	surface = SDL_GetWindowSurface(window);
	if (!surface) {
		std::cerr << "SDL_CreateRGBSurface Error: " << SDL_GetError() << '\n';
	}
	
    GLenum ersr = glGetError();
    if (ersr!= GL_NO_ERROR) {
        std::cerr << "OpenGL Error con: " << ersr << '\n';
    }
	
	for(int x = 0; x < surface->w; x++) {
		for(int y = 0; y < surface->h; y++) {
			updatePixel(x, y, 0xFFFF0000);
		}
	}
	
	SDL_RenderPresent(renderer);
}

void PPU::updatePixel(uint8_t x, uint8_t y, uint32_t color) {
	/*uint8_t scale = 1;
	
	x *= scale;
	y *= scale;
	
	for(int nx = x; nx < x + scale; nx++) {
		for(int ny = y; ny < y + scale; ny++) {
			setPixel(nx, ny, color);
		}
	}*/
	
	setPixel(x, y, color);
	
	/*SDL_RenderCopy(renderer, mainTexture, nullptr, nullptr);
	
	SDL_DestroyTexture(mainTexture);*/
	SDL_FreeSurface(surface);
}

void PPU::setPixel(uint8_t x, uint8_t y, uint32_t color) {
	if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
		return;
	}
	
	uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);
	pixels[(y * surface->w) + x] = color;
}

SDL_Texture* PPU::createTexture(uint8_t width, uint8_t height) {
	return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
											 SDL_TEXTUREACCESS_STREAMING,
											 width, height);
}
