#include "PPU.h"

#include <iostream>

#include <GL/glew.h>
#include <GL/wglew.h>

#include "LCDC.h"
#include "OAM.h"
#include "SDL.h"
#include "VRAM.h"

#include "MMU.h"

void PPU::tick(const int& cycles) {
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
	
	/*switch (mode) {
	case 0: // H-Blank
		if(this->cycle >= 456) {
			mode = 1; // V-Blank
			this->cycle -= 456;
		}
		
		break;
	case 1: // V-Blank
		if(lcdc.LY < 153 && this->cycle >= 456) {
			mode = 2; // OAM Scan
			this->cycle -= 456;
			lcdc.LY = 0;
		} else if(this->cycle >= 456) {
			lcdc.LY++;
			this->cycle -= 456;
		}
		
		break;
	case 2: // OAM Scan
		if(this->cycle >= 80) {
			mode = 3; // Drawing
			this->cycle -= 80;
		}
		
		break;
	case 3:
		if(this->cycle >= 456) {
			// Push pixels to LCD
			pushPixelsToLCDC();
			
			mode = 0;
			this->cycle -= 456;
			lcdc.LY++;
		} else if(this->cycle >= 172) {
			fetchBackground();
			fetchSprites();
		}
		
		break;
	}*/
}

void PPU::fetchBackground() {
	uint8_t LCDControl = mmu.fetch8(0xFF40);
    uint8_t LY = mmu.fetch8(0xFF44);
    uint8_t SCY = mmu.fetch8(0xFF42);
    uint8_t SCX = mmu.fetch8(0xFF43);
	
    // Calculate starting tile coordinates and offsets
    uint8_t startBackgroundTileY = (SCY + LY) / 8;
    uint8_t backgroundTileOffset = (SCY + LY) % 8;
    uint8_t startBackgroundTileX = SCX / 8;
    uint8_t backgroundTileXOffset = SCX % 8;
	
    uint16_t tileMapAddress = (LCDControl & 0x08) ? 0x9C00 : 0x9800;
	
    // Fetch the tile map
    uint8_t tileMap[32][32];
    for (uint16_t i = 0; i < 32 * 32; i++) {
        tileMap[i / 32][i % 32] = mmu.fetch8(tileMapAddress + i);
    }
	
    uint16_t tileDataAddress = (LCDControl & 0x10) ? 0x8000 : 0x8800;
	
    // Fetch tile data
    TileData tileData[256];
    for (int i = 0; i < 256; i++) {
        TileData tile;
        tile.nr = i;
        uint16_t tileMemoryStart = tileDataAddress + (i * 16);
		
        for (int row = 0; row < 8; row++) {
            uint8_t lineData1 = mmu.fetch8(tileMemoryStart + (row * 2));
            uint8_t lineData2 = mmu.fetch8(tileMemoryStart + (row * 2) + 1);
			
            for (int col = 0; col < 8; col++) {
                uint8_t lowBit = (lineData1 >> (7 - col)) & 0x1;
                uint8_t highBit = (lineData2 >> (7 - col)) & 0x1;
                tile.data[row][col] = (highBit << 1) | lowBit;
            }
        }
		
        tileData[i] = tile;
    }
	
    // Draw background
    for (uint8_t x = 0; x < 21; x++) { // 21 tiles wide
        for (uint8_t y = 0; y < 18; y++) { // 18 tiles high
            uint8_t tileNrX = (startBackgroundTileX + x) % 32;
            uint8_t tileNrY = (startBackgroundTileY + y) % 32;
            uint8_t tileNr = tileMap[tileNrY][tileNrX];
            TileData curTile = tileData[tileNr];
        	
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    int pixelX = (x * 8 + col - backgroundTileXOffset);
                    int pixelY = (y * 8 + row - backgroundTileOffset);
                	
                    //if (pixelX >= 0 && pixelX < 160 && pixelY >= 0 && pixelY < 144) { // Screen bounds check
                        uint8_t pixelData = curTile.data[row][col];
                        uint8_t color = BGPalette[pixelData];
                        auto [r, g, b] = GetRGB(color);
                        updatePixel(mainTexture, pixelX, pixelY, RGBToUint32(r, g, b));
                    //}
                }
            }
        }
    }
	
	/*uint8_t LCDControl = mmu.fetch8(0xFF40);
	uint8_t LY = mmu.fetch8(0xFF44);
	uint8_t SCY = mmu.fetch8(0xFF42);
	uint8_t SCX = mmu.fetch8(0xFF43);
	
	uint8_t startBackgroundTileY  = (SCY + LY) / 8 >= 32
				? (SCY + LY) / 8 % 32
				: (SCY + LY) / 8;
	
	uint8_t backgroundTileOffset = (SCY + LY) % 8;
	
	uint8_t startBackgroundTileX  = SCX / 8;
	uint8_t backgroundTileXOffset = SCX % 8;
	
	uint16_t tileMapAddress = (LCDControl & 0x08) ? 0x9C00 : 0x9800;
	TileMap tileMap;
	
	for(uint16_t i = 0; i < 32 * 32; i++) {
		tileMap.data[i/32][i%32] = mmu.fetch8(tileMapAddress + i);
	}
	
	uint16_t tileDataAddress = (LCDControl & 0x10) ? 0x8000 : 0x8800;
	
	TileData tileData[256];
	for(int i = 0; i < 256; i++) {
		TileData tile;
		tile.nr = i;
		
		// Each title is 16 bytes long
		uint16_t tileMemoryStart = (uint16_t)tileDataAddress + (i * 16);
		
		for(int i = 0; i < 8; i++) {
			uint8_t lineData1 = mmu.fetch8(tileMemoryStart + (i * 2));
			uint8_t lineData2 = mmu.fetch8(tileMemoryStart + (i * 2) + 1);
			
			for(int j = 7; j >= 0; j--) {
				uint8_t lowBit = (lineData1 >> j) & 0x1;
				uint8_t highBit = (lineData2 >> j) & 0x1;
				tile.data[i][std::abs(j - 7)] = (highBit << 1) | lowBit;
			}
		}
		
		tileData[i] = tile;
	}
	
	for(uint8_t i = 0; i < 21; i++) {
		// Wrap around window X
		uint8_t tileNrX = startBackgroundTileX + i >= 32
				? (startBackgroundTileX + i) % 32
				: startBackgroundTileX + i;
		
		uint8_t curTileNr = tileMap.data[startBackgroundTileY][tileNrX];
		TileData curTile = tileData[curTileNr];
		
		for(int j = 0; j < 8; j++) {
			/*if(i == 0 && j < backgroundTileXOffset) continue;
			if(i == 20 && j >= backgroundTileXOffset) continue;
			#1#
			
			uint8_t pixelData = curTile.data[backgroundTileOffset][j];
			uint8_t color =  BGPalette[pixelData];
			
			auto [r, g, b] = GetRGB(color);
			updatePixel(mainTexture, LY, (i * 8) + j - backgroundTileXOffset, RGBToUint32(r, g, b));
		}
	}*/
	
	/*
	int tileX = (SCX + cycle) / 8;
	int tileY = (SCY + LY) / 8;
	
	uint16_t tileNumber = vram.fetch8(tileMapAddress + (tileY * 32) + tileX);
	uint16_t tileAddress = tileDataAddress + (tileNumber * 16);
	
	uint8_t line = (SCY + LY) % 8;
	uint8_t tileLineLow = vram.fetch8(tileAddress + (line * 2));
	uint8_t tileLineHigh = vram.fetch8(tileAddress + ((line * 2) + 1));
	
	for(int i = 0; i < 8; i++) {
		int bitPos = 7 - ((SCX + cycle) % 8);
		
		bgFifo[i].color = ((tileLineLow >> bitPos) & 1) | (((tileLineHigh >> bitPos) & 1) << 1);
		bgFifo[i].palette = 0; // Background palette
		bgFifo[i].spritePriority = false;
		bgFifo[i].bgPriority = true;
	}*/
}

void PPU::fetchSprites() {
	uint8_t LCDControl = mmu.fetch8(0xFF40);
	uint8_t LY = mmu.fetch8(0xFF44);
	
	int spriteHeight = (LCDControl & 0x04) ? 16 : 8;
	
	int spriteIndex = 0;
	
	for(int i = 0; i < 40 && spriteIndex < 8; i++) {
		uint16_t spriteAddress = 0xFE00 + (i * 4);
		
		int spriteY = LY - oam.fetch8(spriteAddress);
		if(spriteY >= 0 && spriteY <= spriteHeight) {
			uint16_t tileNumber = oam.fetch8(spriteAddress + 2);
			
			int tileLine = spriteY % 8;
			uint16_t tileDataAddress = (LCDControl & 0x10) ? 0x8000 : 0x8800;
			uint16_t tileAddress = tileDataAddress + (tileNumber * 16);
			
			uint8_t tileLineLow = vram.fetch8(tileAddress + (tileLine * 2));
			uint8_t tileLineHigh = vram.fetch8(tileAddress + (tileLine * 2) + 1);
			
			bool xFlip = (oam.fetch8(spriteAddress + 3) & 0x20) != 0;
			bool yFlip = (oam.fetch8(spriteAddress + 3) & 0x40) != 0;
			
			for(int j = 0; j < 8; j++) {
				int bitPos = xFlip ? j : 7 - j;
				
				Pixel pixel;
                pixel.color = ((tileLineLow >> bitPos) & 1) | (((tileLineHigh >> bitPos) & 1) << 1);
				pixel.palette = (oam.fetch8(spriteAddress + 3) & 0x10) ? 1 : 0;
				pixel.spritePriority = true;
				pixel.bgPriority = !(oam.fetch8(spriteAddress + 3) & 0x80);
				
				spriteFifo[spriteIndex++] = pixel;
			}
		}
	}
}

void PPU::pushPixelsToLCDC() {
	uint8_t LY = mmu.fetch8(0xFF44);
	
    int spriteIndex = 0;
    
    for (uint8_t i = 0; i < 160; ++i) {
        Pixel bgPixel = bgFifo[i % 8];
        Pixel spritePixel = (spriteIndex < 8) ? spriteFifo[spriteIndex++] : Pixel{0, 0, false, false};
        
        Pixel pixel;
        if (spritePixel.spritePriority && (!bgPixel.bgPriority || spritePixel.color != 0)) {
            pixel = spritePixel;
        } else {
            pixel = bgPixel;
        }
        
        auto [r, g, b] = GetRGB(pixel.color);
        
        updatePixel(mainTexture, i, LY, RGBToUint32(r, g, b));
    }
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
    
    SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
    SDL_UpdateWindowSurface(window);
    
    GLenum ersr = glGetError();
    if (ersr!= GL_NO_ERROR) {
        std::cerr << "OpenGL Error con: " << ersr << '\n';
    }
	
	// Create main display texture
	mainTexture = createTexture(160, 144);
	
	/*for(uint8_t x = 0; x < 160; x++) {
		for(uint8_t y = 0; y < 144; y++) {
			updatePixel(mainTexture, x, y, 0xFFFF0000);
		}
	}*/
}

uint8_t PPU::getPixelColor(const TileData& data, int row, int col) const {
	/*int byte1 = row * 2;
	int byte2 = byte1 + 1;
	
	uint8_t bit1 = (data.data[byte1] >> (7 - col)) & 1;
	uint8_t bit2 = (data.data[byte2] >> (7 - col)) & 1;
	
	return static_cast<uint8_t>((bit2 << 1) | bit1);*/
	return 0;
}

void PPU::updatePixel(SDL_Texture* texture, uint8_t x, uint8_t y, Uint32 color) {
	// Lock the texture to gain direct access to its pixels
	void* pixels = nullptr;
	int pitch = 0;
	if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
		// SDL_LockTexture failed, handle the error
		printf("SDL_LockTexture Error: %s\n", SDL_GetError());
		return;
	}
    
	// Convert the pixels pointer to a 32-bit integer pointer for pixel manipulation
	Uint32* pixel_ptr = static_cast<Uint32*>(pixels);
    
	// Calculate the width based on the pitch
	int texture_width = pitch / 4;
    
	// Ensure we're within the texture bounds before modifying
	if (x < texture_width && y < 144) {
		pixel_ptr[(y * texture_width) + x] = color;
	} else {
		//printf("Attempting to access out-of-bounds pixel: (%d, %d)\n", x, y);
	}
    
	// Unlock the texture after modifying it
	SDL_UnlockTexture(texture);
	
	// Define a destination rectangle that matches the texture's size
	SDL_Rect destRect = {0, 0, 160 * 4, 144 * 4};
    
	// Update the texture on the screen
	SDL_RenderCopy(renderer, texture, nullptr, &destRect);
	SDL_RenderPresent(renderer);
}

std::tuple<unsigned char, unsigned char, unsigned char> PPU::GetRGB(unsigned char color) {
	switch (color) {
	case 0:
		return std::make_tuple(255, 255, 255);  // White
	case 1:
		return std::make_tuple(192, 192, 192);  // Light Gray
	case 2:
		return std::make_tuple(96, 96, 96);    // Dark Gray
	case 3:
		return std::make_tuple(0, 0, 0);       // Black
	default:
		return std::make_tuple(0, 0, 0);       // Default to black in case of an invalid value
	}
}

SDL_Texture* PPU::createTexture(uint8_t width, uint8_t height) {
	return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
											 SDL_TEXTUREACCESS_STREAMING,
											 width, height);
}
