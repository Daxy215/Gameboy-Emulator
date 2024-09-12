#include "PPU.h"

#include <iostream>

#include <GL/glew.h>
#include <GL/wglew.h>

#include "LCDC.h"
#include "OAM.h"
#include "SDL.h"
#include "VRAM.h"

#include "../Memory/MMU.h"

void PPU::tick(const int& cycles = 4) {
	this->cycle += cycles;
	
	const uint16_t CyclesHBlank = 204;     // Mode 0 (H-Blank) 204 cycles per Scanline
	const uint16_t CyclesVBlank = 456;     // Mode 1 (V-Blank) 4560 cycles per Frame 4560/10 times per Frame
	const uint16_t CyclesOam = 80;         // Mode 2 (OAM Search) 80 cycles per Scanline
	const uint16_t CyclesTransfer = 173;   // Mode 3 (Transfer LCD) 173 cycles per Scanline
	
	uint8_t LY = mmu.fetch8(0xFF44);
	
	switch (mode) {
	case 0:
		if(cycle >= CyclesOam) {
			mode = 1; // Transfer
			cycle -= CyclesOam;
		}
		
		break;
	case 1:
		if(cycle >= CyclesTransfer) {
			mode = 2; // HBlank
			cycle -= CyclesTransfer;
		}
		
		break;
	case 2: //HBlank
		if(cycle >= CyclesHBlank) {
			if(LY == 143) {
				mode = 3; // VBLank
			} else {
				// Go to OAM mode
				mode = 0;
			}
			
			// Draw background, window and sprites
			fetchBackground();
			//fetchSprites();
			//pushPixelsToLCDC();

			SDL_RenderPresent(renderer);
			
			// Increment LY
			mmu.write8(0xFF44, LY + 1);
			
			cycle -= CyclesHBlank;
		}
		
		break;
	case 3:
		if(cycle >= CyclesVBlank) {
			if(LY == 144) {
				// TODO;..
			}
			
			if(LY == 153) {
				mmu.write8(0xFF44, 0);
				mode = 0; // OAM
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
	
	
}

void PPU::fetchSprites() {
	uint8_t LCDControl = mmu.fetch8(0xFF40);
    uint8_t LY = mmu.fetch8(0xFF44);
	
	
}

void PPU::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF47) {
		// https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
		
		for(int i = 0; i < 4; i++) {
			BGPalette[i] = (data >> (i * 2)) & 0x03;
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
        1024, 512, SDL_WINDOW_OPENGL);
    
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
    
    glClearColor(1, 0, 0, 1.0f);
    SDL_GL_SwapWindow(window);
    
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
	
	// Create main display texture
	mainTexture = createTexture(160, 144);
	
	for(uint8_t x = 0; x < 160; x++) {
		for(uint8_t y = 0; y < 144; y++) {
			updatePixel(x, y, 0xFFFF0000);
		}
	}
	
	SDL_RenderPresent(renderer);
}

void PPU::updatePixel(uint8_t x, uint8_t y, Uint32 color) {
	if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
		return; // Out of bounds
	}
	
	Uint32* pixels = (Uint32*)surface->pixels;
	pixels[(y * surface->w) + x] = color;
	
	SDL_Rect destRect = { x, y, 1, 1 };
	SDL_RenderCopy(renderer, mainTexture, nullptr, &destRect);
	
	SDL_DestroyTexture(mainTexture);
	SDL_FreeSurface(surface);
}

SDL_Texture* PPU::createTexture(uint8_t width, uint8_t height) {
	return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
											 SDL_TEXTUREACCESS_STREAMING,
											 width, height);
}
