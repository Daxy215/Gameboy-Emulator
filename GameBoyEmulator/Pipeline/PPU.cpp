#include "PPU.h"

#include <iostream>

#include <GL/glew.h>

#include "LCDC.h"
#include "SDL.h"

#include "../Memory/MMU.h"
#include "../Utility/Bitwise.h"

const int WIDTH = 640;
const int HEIGHT = 480;

PPU::PPUMode PPU::mode = PPU::HBlank;

void PPU::tick(const int& cycles = 4) {
	this->clock += cycles;
	
	/*if(!lcdc.enable)
		return;*/
	
	//uint8_t LCDControl = mmu.fetch8(0xFF40);
	uint8_t LYC = mmu.fetch8(0xFF45);
	uint8_t& LY = lcdc.LY;//mmu.fetch8(0xFF44);
	uint8_t SCX = mmu.fetch8(0xFF43);
	/*uint8_t WY = mmu.fetch8(0xFF4A);
	uint8_t SCY = mmu.fetch8(0xFF42);
	uint8_t WX = mmu.fetch8(0xFF4B);*/
	
	/*uint8_t HBlankCycles = 0;

	switch (SCX & 7) {
	case 0:
		HBlankCycles = 204;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		HBlankCycles = 200;
		break;
	case 5:
	case 6:
	case 7:
		HBlankCycles = 196;
		break;
	default:
		std::cerr << "BREUWH\n";
		break;
	}*/
	
	// Following ;https://hacktix.github.io/GBEDG/ppu/#h-blank-mode-0
	switch (mode) {
	case HBlank:
		while(clock >= 204) {
			LY++;
			
			if(LY >= 144) {
				// VBlank interrupt
				interrupt |= 0x01;
				//frames++;
				
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
			LY++;
			
			/*if(LYC == LY) {
				interrupt |= 0x02;
			}*/
			
			if(LY >= 154) {
				LY = 0;
				//lcdc.WX = 0;
				
				mode = OAMScan;
				
				/*if(LYC == LY) {
					interrupt |= 0x02;
				}*/
			}
			
			clock -= 4560;
		}
		
		break;
	case OAMScan:
		while(clock >= 80) {
			fetchSprites();
			
			mode = VRAMTransfer;
			clock -= 80;
		}
		
		break;
	case VRAMTransfer:
		while(clock >= 172) {
			drawBackground();
			
			mode = HBlank;
			clock -= 172;
		}
		
		break;
	}
}

void PPU::drawBackground() {
    uint8_t LY  = lcdc.LY;//mmu.fetch8(0xFF44);
    uint8_t SCY = lcdc.SCY;//mmu.fetch8(0xFF42);
    uint8_t SCX = lcdc.SCX;//mmu.fetch8(0xFF43);
	uint8_t WY  = lcdc.WY;//mmu.fetch8(0xFF4A);
	uint8_t WX  = lcdc.WX;//mmu.fetch8(0xFF4B);
	
	uint16_t tileWinMapBase = lcdc.windowTileMapArea  ? 0x9C00 : 0x9800;
	uint16_t tileBGMapBase  = lcdc.bgTileMapArea      ? 0x9C00 : 0x9800;
	uint16_t tileBGMap      = lcdc.bgWinTileDataArea  ? 0x8000 : 0x8800;
	
	int32_t winY = WY;
	
	if(lcdc.windowEnabled && WX <= 166) {
		mmu.write8(0xFF4A, ++WY);
		winY = WY;
	} else {
		winY = -1;
	}
	
	if(winY < 0 && !lcdc.enable)
		return;
	
	uint8_t bgY = (SCY + LY) % 256;
	
	uint16_t tilemapAddr;
	uint16_t tileX, tileY, pY;
	uint8_t pX;
	
	// 160 = Screen width
	for(size_t x = 0; x < 160; x++) {
		int32_t winX = ((static_cast<int32_t>(WX) - 7) + static_cast<int32_t>(x)) % 256;
		uint32_t bgX = (static_cast<uint32_t>(SCX) + static_cast<uint32_t>(x)) % 256;
		
		if(/*lcdc.windowEnabled && */winX >= 0 && winY >= 0) {
			//mmu.write8(0xFF4A, ++WY);
			
			tilemapAddr = tileWinMapBase;
			
			tileY = (winY >> 3) & 31;
			tileX = (winX >> 3) & 31;
			
			pY = winY & 0x07; // % 8
			pX = static_cast<uint8_t>(winX) & 0x07; // % 8
		} else {
			tilemapAddr = tileBGMapBase;
			
			tileY = (bgY >> 3) & 31;
			tileX = (bgX >> 3) & 31;
			
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
		
		bgPriority[x][LY] = pixelColor;
		updatePixel(static_cast<uint8_t>(x), LY, paletteIndexToColor(pixelColor));
	}
}

void PPU::fetchSprites() {
	// Following; https://gbdev.io/pandocs/OAM.html
	
	if(!lcdc.objEnabled)
		return;
	
	uint8_t LY = lcdc.LY; //mmu.fetch8(0xFF44);
	uint8_t spriteHeight = lcdc.objSize ? 16 : 8;
	
	// TODO; Move this outta here
	struct Sprite {
		uint8_t index = 0;
		int8_t x = 0, y = 0;
		
		Sprite() : index(0), x(0), y(0) {}
		Sprite(uint8_t index, int8_t x, int8_t y) : index(index), x(x), y(y) {}
	};
	
	// Only 10 sprites can e drawn at a time
	std::vector<Sprite> spriteBuffer(10);
	
	/**
	 * According to (https://gbdev.io/pandocs/OAM.html),
	 * The PPU can render up to 40 moveble objects,
	 * but only 10 objects can be displayed per scanline.
	 */
	
	uint8_t index = 0;
	
	for(uint8_t i = 0; i < 40; i++) {
		// 0xFE00 -> OAM
		uint16_t spriteAdr = 0xFE00 + i * 4;
		int8_t spriteY = static_cast<int8_t>(mmu.fetch8(spriteAdr + 0) - 16);
		
		//if(LY + 16 >= spriteY && LY + 16 < spriteY + spriteHeight) {}
		if(LY < spriteY || LY >= spriteY + spriteHeight) {
			continue;
		}
		
		int8_t spriteX = static_cast<int8_t>(mmu.fetch8(spriteAdr + 1) - 8);
		spriteBuffer[index++] = Sprite(i, spriteX, spriteY);
		
		if(index >= 10)
			break;
	}
	
	// Order sprites by their coordinates
	/*std::sort(spriteBuffer.begin(), spriteBuffer.begin() + 10, [](const Sprite& a, const Sprite& b) {
		// Higher x-coordinate has higher priority
		if (a.x != b.x) {
			return a.x > b.x; // Sort in descending order by x
		}
		
		return a.index < b.index; // Sort in ascending order by index
	});*/
	
	for (auto sprite : spriteBuffer) {
		if(sprite.x < -7 || sprite.x >= 160)
			continue;
		
		// 0xFE00 -> OAM
		uint16_t spriteAddr = 0xFE00 + sprite.index * 4;

		// https://gbdev.io/pandocs/OAM.html#byte-3--attributesflags
		uint16_t tileIndex = (mmu.fetch8(spriteAddr + 2) & (spriteHeight == 16 ? 0xFE : 0xFF));
		
		// https://gbdev.io/pandocs/OAM.html#byte-3--attributesflags
		uint8_t flags = static_cast<uint8_t>(mmu.fetch8(spriteAddr + 3));
		
		/**
		 * 0 - NO
		 * 1 - BG and Window colors 1-3 are drawn over this obj
		 */
		bool priority = check_bit(flags, 7);
		
		/**
		 * 0 = Normal
		 * 1 = Entire OBJ is vertically mirrored
		 */
		bool flipY = check_bit(flags, 6);
		
		/**
		 * 0 = Normal
		 * 1 = Entire OBJ is horizxontally mirrored
		 */
		bool flipX = check_bit(flags, 5);
		
		/**
		 * Non CGB Mode only
		 *
		 * 0 - OBP0
		 * 1 - OBP1
		 */
		bool dmgPallete = check_bit(flags, 4);
		
		/**
		 * CGB Mode only
		 * 0 - Fetch from VRAM bank 0
		 * 1 - Fetch from VRam bank 1
		 */
		bool bank = check_bit(flags, 3);
		
		/**
		 * CGB Mode only
		 *
		 * Which OBP0-7 to use
		 */
		bool pallete = check_bit(flags, 2);
		
		uint16_t tileY = flipY ? (spriteHeight - 1 - (LY - sprite.y)) : (LY - sprite.y);
		
		/**
		 * According to https://gbdev.io/pandocs/Tile_Data.html
		 *
		 * Objects always use “$8000 addressing”, but the BG and Window can use either mode, controlled by LCDC bit 4.
		 */
		uint16_t tileAddr = 0x8000 + tileIndex * 16 + tileY * 2;
		
		// TODO; For CGB check BANK
		uint8_t d0 = mmu.fetch8(tileAddr + 0);
		uint8_t d1 = mmu.fetch8(tileAddr + 1);
		
		/**
		 * From what I understand is that the,
		 * GameBoy PPU works in pixels rather than tiles.
		 *
		 * So 8 here is the width of every sprite.
		 * As only the height changes from 8-16,
		 * I don't need any extra checkls
		 */
		for(uint8_t x = 0; x < 8; x++) {
			if (sprite.x + x < 0 || sprite.x + x >= 160)
				continue;
			
			uint8_t pX = flipX ? (7 - x) : x;
			
			// Just copied this from 'drawBackground'
			uint8_t pixel = (d0 >> pX) & 1;
			pixel |= (((d1 >> pX) & 1) << 1);
			
			if (pixel == 0)
				continue;
			
			uint8_t pixelColor = dmgPallete ? OBJ1Palette[pixel] : OBJ0Palette[pixel];
			
			for (uint8_t y = 0; y < spriteHeight; y++) {
				if(priority && bgPriority[(sprite.x + x)][sprite.y + y] == 0) {
					continue;
				}
				
				updatePixel(static_cast<uint8_t>(sprite.x + x), static_cast<uint8_t>(sprite.y + y), paletteIndexToColor(pixelColor));
				//updatePixel(static_cast<uint8_t>(sprite.x + x), static_cast<uint8_t>(sprite.y + y), 0xFFFF0000);
			}
		}
	}
}

uint8_t PPU::fetch8(uint16_t address) {
	if(address == 0xFF47) {
		// https://gbdev.io/pandocs/Palettes.html#ff47--bgp-non-cgb-mode-only-bg-palette-data
		return bgp;
	} else if(address == 0xFF48 || address == 0xFF49) {
		// https://gbdev.io/pandocs/Palettes.html#ff48ff49--obp0-obp1-non-cgb-mode-only-obj-palette-0-1-data
		
		if(address == 0xFF48)
			return obj0;
		else
			return obj1;
	}
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
			if(address == 0xFF48) {
				obj0 = data;
				OBJ0Palette[i] = (data >> (i * 2)) & 0x03;
			} else {
				obj1 = data;
				OBJ1Palette[i] = (data >> (i * 2)) & 0x03;
			}
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

void PPU::reset() {
	clock = 0;
	//frames = 0;
}

SDL_Texture* PPU::createTexture(uint8_t width, uint8_t height) {
	return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
											 SDL_TEXTUREACCESS_STREAMING,
											 width, height);
}
