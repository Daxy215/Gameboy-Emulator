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

void PPU::tick(const int& cycles = 4) {
	this->cycle += cycles;

	if(!lcdc.enable)
		return;
	
	// https://gbdev.io/pandocs/Rendering.html#ppu-modes
	const uint16_t CyclesHBlank = 204;     // Mode 0 (H-Blank) 204 cycles per Scanline
	const uint16_t CyclesVBlank = 456;     // Mode 1 (V-Blank) 4560 cycles per Frame 4560/10 times per Frame
	const uint16_t CyclesOam = 80;         // Mode 2 (OAM Search) 80 cycles per Scanline
	const uint16_t CyclesTransfer = 173;   // Mode 3 (Transfer LCD) 173 cycles per Scanline
	
	uint8_t LCDControl = mmu.fetch8(0xFF40);
	uint8_t LY = lcdc.LY;//mmu.fetch8(0xFF44);
	uint8_t LYC = mmu.fetch8(0xFF45);
	uint8_t SCY = mmu.fetch8(0xFF42);
	uint8_t SCX = mmu.fetch8(0xFF43);
	int8_t WY = mmu.fetch8(0xFF4A);
	int8_t WX = mmu.fetch8(0xFF4B) - 7;
	
	switch (mode) {
	case 2: // OAM
		while(cycle >= CyclesOam) {
			// https://gbdev.io/pandocs/pixel_fifo.html#get-tile
			uint16_t tileWinMapBase = lcdc.windowTileMapArea  ? 0x9C00 : 0x9800;
			uint16_t tileBGMapBase  = lcdc.bgTileMapArea      ? 0x9C00 : 0x9800;
			uint16_t tileBGMap      = lcdc.bgWinTileDataArea  ? 0x8000 : 0x8800;
			
			uint8_t y;
			uint16_t addr;
			
			uint16_t tileX, tileY;
			
			/*if(lcdc.windowEnabled)
				mmu.write8(0xFF4B, WX + 1);
				*/
			
			int16_t winX = -(static_cast<int32_t>(WX)) + x;
			
			/*if(lcdc.windowEnabled && /*winX >= 0 && WY >= 0#1# (LY >= WY) && (WX <= 166)) {
				mmu.write8(0xFF4A, WY + 1);
				
				y = LY - WY;
				addr = tileWinMapBase;
				
				tileY = (static_cast<uint16_t>(WY) >> 3) & 31;
				tileX = (winX >> 3);
			} else {*/
				y = SCY + (LY % 256);
				addr = tileBGMapBase;
				
				tileY = (y >> 3) & 31;
				tileX = ((SCX + 0) >> 3) & 31;
			//}
			
			x = 0;
			fetcher.begin(tileBGMap, addr + tileY * 32 + tileX, y % 8);
			
			mode = 3; // VRRAM Transfer
			cycle -= CyclesOam;
		}
		
		break;
	case 3: { // VRAM Transfer
		fetcher.tick();
		
		if(fetcher.fifo.size() <= 8)
			return;
		
		uint8_t byte0 = fetcher.fifo.pop();
		uint8_t pixelColor = (bgp >> (byte0 * 2)) & 0x03;
		//if(pixelColor != 0)
			updatePixel((x - 16), LY, paletteIndexToColor(pixelColor));
		x++;

		//SDL_RenderPresent(renderer);
		
		if(x >= 160) {
			if(LYC == LY) {
				interrupt |= 0x02;
			}
			
			mode = 0; // HBlank
			cycle -= CyclesTransfer;
		}
		
		break;
	}
	
	case 0: //HBlank
		while(cycle >= CyclesHBlank) {
			if(LY == 143) {
				mode = 1; // VBLank
			} else {
				// Go to OAM mode
				mode = 2;
			}
			
			// Increment LY
			mmu.write8(0xFF44, LY + 1);
			
			cycle -= CyclesHBlank;
		}
		
		break;
	case 1: // VBlank
		while(cycle >= CyclesVBlank) {
			if(LY == 144) {
				// TODO;..
				interrupt |= 0x01;
				
				SDL_RenderPresent(renderer);
			}
			
			if(LY == 153) {
				mmu.write8(0xFF44, 0);
				mode = 2; // OAM
			} else {
				mmu.write8(0xFF44, LY + 1);
			}
			
			cycle -= CyclesVBlank;
		}
		
		break;
	}
}

void PPU::fetchBackground() {
    uint8_t LCDControl = mmu.fetch8(0xFF40);
    uint8_t LY = mmu.fetch8(0xFF44);
    uint8_t SCY = mmu.fetch8(0xFF42);
    uint8_t SCX = mmu.fetch8(0xFF43);
	uint8_t WY = mmu.fetch8(0xFF4A);
	uint8_t WX = mmu.fetch8(0xFF4B);
	
	
}

void PPU::fetchSprites() {
	uint8_t LCDControl = mmu.fetch8(0xFF40);
    uint8_t LY = mmu.fetch8(0xFF44);
	
	
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
	uint8_t scale = 2;
	
	x *= scale;
	y *= scale;
	
	for(int nx = x; nx < x + scale; nx++) {
		for(int ny = y; ny < y + scale; ny++) {
			setPixel(nx, ny, color);
		}
	}
	
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
