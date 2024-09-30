#include "PPU.h"

#include <iostream>
#include <string>

#include <GL/glew.h>

#include "LCDC.h"
#include "SDL.h"
#include "VRAM.h"
#include "../Memory/Cartridge.h"

#include "../Memory/MMU.h"
#include "../Utility/Bitwise.h"

const int WIDTH = 160 * 4;
const int HEIGHT = 144 * 4;

PPU::PPUMode PPU::mode = PPU::HBlank;

void PPU::tick(const int& cycles = 4) {
	if(!lcdc.enable)
		return;
	
	uint8_t LYC = mmu.fetch8(0xFF45);
	uint8_t WY  = mmu.fetch8(0xFF4A);
	uint8_t& LY = lcdc.LY;
	
	/**
	 * https://stackoverflow.com/questions/52153636/how-do-gpu-ppu-timings-work-on-the-gameboy
	 * https://forums.nesdev.org/viewtopic.php?f=20&t=17754&p=225009#p225009
	 * http://blog.kevtris.org/blogfiles/Nitty%20Gritty%20Gameboy%20VRAM%20Timing.txt
	 */
	
	static const int SCANLINE_DOTS = 456;         // Total dots per scanline
	static const int SCANLINES = 154;             // Total scanlines per frame
	static const int MODE_2_DOTS = 80;            // OAM search duration in dots
	static const int VBLANK_START_LINE = 144;     // Start of vertical blanking
	
	//static uint16_t currentDot = 0;
	//uint16_t currentScanline = 0;
	
	currentDot += cycles;
	
	switch (mode) {
		case OAMScan: {
			// https://gbdev.io/pandocs/Scrolling.html?highlight=LY%20%3D%20WY#window
			if(lcdc.status & 0x20) {
				interrupt |= 0x02;
			}
			
			if (currentDot >= MODE_2_DOTS) {
				mode = VRAMTransfer;
			}
			
			break;
		}
		
		case VRAMTransfer: {
			if(lcdc.windowEnabled && !drawWindow && LY == WY) {
				drawWindow = true;
				//WY = 0;
			}
			
			// TODO; 10 - Sprite count, just 10 for now..
			int mode3_dots = 168 + (10) * 10;
			if (currentDot >= MODE_2_DOTS + mode3_dots) {
				mode = HBlank;
			}
			
			break;
		}
		
		case HBlank: {
			if (currentDot >= SCANLINE_DOTS) {
				drawScanline();
				
				currentDot = 0;
				LY = (LY + 1) % 154;
				
				if((lcdc.status & 0x40) && LYC == LY) {
					interrupt |= 0x02;
				}
				
				if(lcdc.status & 0x08) {
					interrupt |= 0x02;
				}
				
				if (LY < VBLANK_START_LINE) {
					mode = OAMScan;
				} else if (LY < SCANLINES) {
					mode = VBlank;
				} else {
					// LY 154 reached
					LY = 0;
					
					mode = OAMScan;
				}
			}
			
			break;
		}
		
		case VBlank: {
			if (currentDot >= SCANLINE_DOTS) {
				drawWindow = false;
				
				currentDot = 0;
				LY = (LY + 1);
				
				// VBlank interrupt
				interrupt |= 0x01;
				
				if((lcdc.status & 0x40) && LYC == LY) {
					interrupt |= 0x02;
				}
				
				if(lcdc.status & 0x10) {
					interrupt |= 0x02;
				}
				
				if (LY >= SCANLINES) {
					// LY 154 reached
					
					LY = 0;
					winLineCounter = 0;
					
					mode = OAMScan;
				}
				
				SDL_RenderPresent(renderer);
			}
			
			break;
		}
	}
}

void PPU::drawScanline() {
	for (BGPriority& i : bgPriority) {
		i = Zero;
	}
	
	drawBackground();
	drawSprites();
}

void PPU::drawBackground() {
    uint8_t LY  = mmu.fetch8(0xFF44);
	
	uint8_t SCY = mmu.fetch8(0xFF42);
    uint8_t SCX = lcdc.SCX; //mmu.fetch8(0xFF43);
	
	//uint8_t WY  = mmu.fetch8(0xFF4A);
	uint8_t WX  = mmu.fetch8(0xFF4B);
	
	uint16_t tileWinMapBase = lcdc.windowTileMapArea  ? 0x9C00 : 0x9800;
	uint16_t tileBGMapBase  = lcdc.bgTileMapArea      ? 0x9C00 : 0x9800;
	uint16_t tileBGMap      = lcdc.bgWinTileDataArea  ? 0x8000 : 0x8800;
	
	//bool drawWindow = lcdc.windowEnabled && LY >= WY && WX <= 166;
	
	if(!lcdc.windowEnabled && !lcdc.bgWindowEnabled)
		return;
	
	if(lcdc.windowEnabled && drawWindow && WX <= 166) {
		winLineCounter++;
	}
	
	uint8_t bgY = SCY + LY;
	
	// 160 = Screen width
	for(size_t x = 0; x < 160; x++) {
		int32_t winX = drawWindow ? -(static_cast<int32_t>(WX) - 7) + static_cast<int32_t>(x) : -1;
		uint8_t bgX = static_cast<uint8_t>(x) + SCX;
		
		uint16_t tilemapAddr;
		uint16_t tileX, tileY;
		uint8_t pY, pX;
		
        if(lcdc.windowEnabled && drawWindow && winX >= 0 && lcdc.WY < 140) {
			tilemapAddr = tileWinMapBase;
			
			tileY = static_cast<uint16_t>((winLineCounter - 1)) >> 3;
			tileX = static_cast<uint16_t>(winX) >> 3;
			
			pY = (winLineCounter - 1) & 0x07; // % 8
			pX = static_cast<uint8_t>(winX) & 0x07; // % 8
		} else {
			tilemapAddr = tileBGMapBase;
			
			tileY = (bgY >> 3) & 31;
			tileX = (bgX >> 3) & 31;
			
			pY = bgY & 0x07;
			pX =(bgX & 0x07);
		}
		
		//uint8_t tileID = mmu.fetch8(tilemapAddr + tileY * 32 + tileX);
		uint8_t tileID = mmu.vram.RAM[(tilemapAddr + tileY * 32 + tileX) & 0x1FFF];
		
		// https://gbdev.io/pandocs/Tile_Maps.html#bg-map-attributes-cgb-mode-only
		bool priority = false;
		bool yFlip = false;
		bool xFlip = false;
		bool bank = false;
		uint8_t colorPalette = 0;
		
		if(Cartridge::mode == Color) {
			uint8_t flags = mmu.vram.RAM[0x2000 + ((tilemapAddr + tileY * 32 + tileX) & 0x1FFF)];
			
			/**
			 * Bit 7 - Priority
			 *
			 * 0 = No
			 * 1 = Colors 1-3 are drawn over OBJ
			 */
			priority = check_bit(flags, 7);
			
			// Bit 6 - YFlip
			yFlip = check_bit(flags, 6);
			
			// Bit 5 - XFlip
			xFlip = check_bit(flags, 5);
			
			// Bit 3 - Bank
			bank = check_bit(flags, 3);
			
			// Bit 2 - 0 - Colour pallete
			colorPalette = flags & 0b00000111;
		}
		
		uint16_t offset;
		
		// https://gbdev.io/pandocs/Tile_Data.html?highlight=signed#vram-tile-data
		
		if(tileBGMap == 0x8000) {
			offset = tileBGMap + (static_cast<uint16_t>(tileID)) * 16;
		} else {
			offset = tileBGMap + static_cast<uint16_t>(static_cast<int16_t>(static_cast<int8_t>(tileID) + 128)) * 16;
		}
		
		// Tiles are flipped by default
		pX = xFlip ? pX : 7 - pX;
		
		uint16_t address = yFlip ? offset + (14 - (pY * 2)) : offset + (pY * 2);
		
		uint8_t b0 = (bank && Cartridge::mode == Color) ? mmu.vram.RAM[(address & 0x1FFF) + 0x2000] : mmu.vram.RAM[(address & 0x1FFF)];
		uint8_t b1 = (bank && Cartridge::mode == Color) ? mmu.vram.RAM[((address & 0x1FFF) + 0x2000) + 1] : mmu.vram.RAM[(address & 0x1FFF) + 1];
		
		uint8_t pixel = static_cast<uint8_t>((b0 >> pX) & 1) | static_cast<uint8_t>(((b1 >> pX) & 1) << 1);
		bgPriority[x] = pixel == 0 ? Zero : (priority ? Priority : None);
		
		if(Cartridge::mode == DMG) {
			uint8_t pixelColor = (bgp >> (pixel * 2)) & 0x03;
			
			updatePixel(static_cast<uint8_t>(x), LY, paletteIndexToColor(pixelColor));
		} else if(Cartridge::mode == Color) {
			uint8_t lsb = CBGPalette[(colorPalette * 8) + (pixel * 2)];
			uint8_t msb = CBGPalette[(colorPalette * 8) + (pixel * 2) + 1];
			
			uint16_t color = static_cast<uint16_t>(msb << 8) | lsb;
			
			uint32_t s = convertRGB555ToSDL(color);
			
			updatePixel(static_cast<uint8_t>(x), LY, s);
		}
		
		//SDL_RenderPresent(renderer);
	}
}

void PPU::drawSprites() {
	// Following; https://gbdev.io/pandocs/OAM.html
	
	if(!lcdc.objEnabled)
		return;
	
	uint8_t LY = lcdc.LY; //mmu.fetch8(0xFF44);
	uint8_t spriteHeight = lcdc.objSize ? 16 : 8;
	
	// TODO; Move this outta here
	struct Sprite {
		uint8_t index = 0;
		int16_t x = 0, y = 0;
		
		Sprite() : index(0), x(0), y(0) {}
		Sprite(uint8_t index, int16_t x, int16_t y) : index(index), x(x), y(y) {}
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
		
		int16_t spriteY = static_cast<int16_t>(mmu.fetch8(spriteAdr + 0) - 16);
		
		if (LY < spriteY || LY >= spriteY + spriteHeight) {
			continue;
		}
		
		int16_t spriteX = static_cast<int16_t>(mmu.fetch8(spriteAdr + 1) - 8);
		
		spriteBuffer[index++] = Sprite(i, spriteX, spriteY);
		
		if (index >= 10)
			break;
	}
	
	// https://gbdev.io/pandocs/OAM.html#drawing-priority
	
	if(Cartridge::mode == Color) {
	//if(opri) {
		// CGB Mode
		std::stable_sort(spriteBuffer.begin(), spriteBuffer.end(), [](const Sprite& a, const Sprite& b) {
			// Order by OAM position
			return a.index > b.index;
		});
	} else {
		// DMG Mode
		std::stable_sort(spriteBuffer.begin(), spriteBuffer.end(), [](const Sprite& a, const Sprite& b) {
			// Order by x prioritization
			if (a.x != b.x) {
				return a.x > b.x;
			}
			
			// Order by OAM position
			return a.index > b.index;
		});
	}
	
	for (auto sprite : spriteBuffer) {
		if (sprite.x < -7 || sprite.x >= 160)
			continue;
		
		// 0xFE00 -> OAM
		uint16_t spriteAddr = 0xFE00 + sprite.index * 4;
		
		// https://gbdev.io/pandocs/OAM.html#byte-3--attributesflags
		uint16_t tileIndex = (mmu.fetch8(spriteAddr + 2) & (spriteHeight == 16 ? 0xFE : 0xFF));
		
		// https://gbdev.io/pandocs/OAM.html#byte-3--attributesflags
		uint8_t flags = mmu.fetch8(spriteAddr + 3);
		
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
		uint8_t palette = flags & 0b00000111;
		
		uint16_t tileY = flipY ? (spriteHeight - 1 - (LY - sprite.y)) : (LY - sprite.y);
		
		/**
		 * According to https://gbdev.io/pandocs/Tile_Data.html
		 *
		 * Objects always use “$8000 addressing”, but the BG and Window can use either mode, controlled by LCDC bit 4.
		 */
		uint16_t tileAddr = 0x8000 + (tileIndex * 16) + (tileY * 2);
		
		uint8_t b0 = (bank && Cartridge::mode == Color) ? mmu.vram.RAM[(tileAddr & 0x1FFF) + 0x2000] : mmu.vram.RAM[(tileAddr & 0x1FFF)];
		uint8_t b1 = (bank && Cartridge::mode == Color) ? mmu.vram.RAM[((tileAddr & 0x1FFF) + 0x2000) + 1] : mmu.vram.RAM[(tileAddr & 0x1FFF) + 1];
		
		/**
		 * From what I understand is that the,
		 * GameBoy PPU works in pixels rather than tiles.
		 *
		 * So 8 here is the width of every sprite.
		 * As only the height changes from 8-16,
		 * I don't need any extra checks
		 */
		for(int8_t x = 0; x < 8; x++) {
			if (sprite.x + x < 0 || sprite.x + x >= 160)
				continue;
			
			// Priority 1 - BG and Window colours 1–3 are drawn over this OBJ
			
			if(Cartridge::mode == DMG) {
				if(priority && bgPriority[sprite.x + x] != Zero) {
					continue;
				}
			} else if(Cartridge::mode == Color) {
				if (lcdc.enable &&
					(bgPriority[sprite.x + x] == Priority || (priority && bgPriority[sprite.x + x] != Zero))) {
					continue;
				}
			}
			
			int8_t pX = (flipX ? x : static_cast<int8_t>(7 - x));
			
			// Just copied this from 'drawBackground'
			uint8_t pixel = static_cast<uint8_t>((b0 >> pX) & 1) | static_cast<uint8_t>(((b1 >> pX) & 1) << 1);
			
			uint32_t color = 0;
			
			if(Cartridge::mode == DMG) {
				uint8_t pixelColor = dmgPallete ? OBJ1Palette[pixel] : OBJ0Palette[pixel];
				
				if(pixelColor == 0)
					continue;
				
				color = paletteIndexToColor(pixelColor);
			} else if(Cartridge::mode == Color) {
				uint8_t lsb = CBGPalette[(palette * 8) + (pixel * 2)];
				uint8_t msb = CBGPalette[(palette * 8) + (pixel * 2) + 1];
				
				uint16_t rgb = static_cast<uint16_t>(msb << 8) | lsb;
				
				color = convertRGB555ToSDL(rgb);
			}
			
			updatePixel(static_cast<uint8_t>(sprite.x + x), LY, color);
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
	} else if(address == 0xFF68) {
		// https://gbdev.io/pandocs/Palettes.html#ff68--bcpsbgpi-cgb-mode-only-background-color-palette-specification--background-palette-index
		
		uint8_t data = 0;
		
		data |= (autoIncrementBG & 0b10000000);
		data |= (bgIndex & 0b00111111);
		
		return data;
	} else if(address == 0xFF69) {
		// https://gbdev.io/pandocs/Palettes.html#ff69--bcpdbgpd-cgb-mode-only-background-color-palette-data--background-palette-data

		if(mode == VRAMTransfer) {
			return 0xFF;
		}
		
		uint8_t data = CBGPalette[bgIndex];
		
		if(autoIncrementBG) {
			bgIndex = (bgIndex + 1) & 0x3F; // Keep in range of 0-63
		}
		
		return data;
	} else if(address == 0xFF6A) {
		uint8_t data = 0;
		
		data |= (autoIncrementOBJ & 0b10000000);
		data |= (objIndex & 0b00111111);
		
		return data;
	} else if(address == 0xFF6B) {
		if(mode == VRAMTransfer) {
			return 0xFF;
		}
		
		uint8_t data = COBJPalette[bgIndex];
		
		if(autoIncrementOBJ) {
			objIndex = (objIndex + 1) & 0x3F; // Keep in range of 0-63
		}
		
		return data;
	} else if(address == 0xFF6C) {
		// https://gbdev.io/pandocs/CGB_Registers.html#ff6c--opri-cgb-mode-only-object-priority-mode
        
		return opri;
	}
	
	printf("Unhandled PPU address: %d", address);
	std::cerr << "";
	
	return 0xFF;
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
	} else if(address == 0xFF68) {
		// https://gbdev.io/pandocs/Palettes.html#ff68--bcpsbgpi-cgb-mode-only-background-color-palette-specification--background-palette-index
		
		/**
		 * Bit 7 - Auto-increment
		 *
		 * 0 = Disabled
		 * 1 = Increment
		 */
		autoIncrementBG = check_bit(data, 7);
		
		// Bit 5 through 0 = Address
		bgIndex = data & 0b00111111;
	} else if(address == 0xFF69) {
		// https://gbdev.io/pandocs/Palettes.html#ff69--bcpdbgpd-cgb-mode-only-background-color-palette-data--background-palette-data
		if(mode == VRAMTransfer) {
			return;
		}
		
		CBGPalette[bgIndex] = data;
		
		if(autoIncrementBG) {
			bgIndex = (bgIndex + 1) & 0x3F; // Keep in range of 0-63
		}
	} else if(address == 0xFF6A) {
		// https://gbdev.io/pandocs/Palettes.html#ff6aff6b--ocpsobpi-ocpdobpd-cgb-mode-only-obj-color-palette-specification--obj-palette-index-obj-color-palette-data--obj-palette-data
		
		// Bit 7 - Auto-increment
		autoIncrementOBJ = check_bit(data, 7);
		
		// Bit 5 through 0 - Address for OBJ palette
		objIndex = data & 0b00111111;
	} else if(address == 0xFF6B) {
		// https://gbdev.io/pandocs/Palettes.html#ff6aff6b--ocpsobpi-ocpdobpd-cgb-mode-only-obj-color-palette-specification--obj-palette-index-obj-color-palette-data--obj-palette-data
		if(mode == VRAMTransfer) {
			return;
		}
		
		COBJPalette[objIndex] = data;
		
		if(autoIncrementOBJ) {
			objIndex = (objIndex + 1) & 0x3F; // Keep in range of 0-63
		}
	} else if(address == 0xFF6C) {
		// https://gbdev.io/pandocs/CGB_Registers.html#ff6c--opri-cgb-mode-only-object-priority-mode
        
		/**
		 * Bit 0 - Priority mode
		 * 
		 * 0 = CGB-Style prioirty
		 * 1 = DMG-Style prioirty
		 */
		opri = check_bit(data, 0);
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
	//surface = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!surface) {
		std::cerr << "SDL_CreateRGBSurface Error: " << SDL_GetError() << '\n';
	}
	
    GLenum ersr = glGetError();
    if (ersr!= GL_NO_ERROR) {
        std::cerr << "OpenGL Error con: " << ersr << '\n';
    }
	
	for(int x = 0; x < surface->w; x++) {
		for(int y = 0; y < surface->h; y++) {
			setPixel(x, y, 0xFFFF0000);
		}
	}
	
	SDL_RenderPresent(renderer);
}

void PPU::updatePixel(uint32_t x, uint32_t y, uint32_t color) {
	float scale = 4;
	
	int startX = static_cast<int>(x * scale);
	int startY = static_cast<int>(y * scale);
	int endX = static_cast<int>((x + 1) * scale);
	int endY = static_cast<int>((y + 1) * scale);
	
	for(int nx = startX; nx < endX; nx++) {
		for(int ny = startY; ny < endY; ny++) {
			setPixel(nx, ny, color);
		}
	}
	
	//setPixel(x, y, color);
	
	/*SDL_RenderCopy(renderer, mainTexture, nullptr, nullptr);
	
	SDL_DestroyTexture(mainTexture);*/
	//SDL_FreeSurface(surface);
}

void PPU::setPixel(uint32_t x, uint32_t y, uint32_t color) {
	if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
		return;
	}
	
	//((uint16_t*)surface->pixels)[y * surface->w + x] = color;
	uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);
	pixels[(y * surface->w) + x] = color;
}

void PPU::reset(const uint32_t& clock) {
	//this->clock = clock;
	this->currentDot = clock;
	drawWindow = false;
	
	//frames = 0;
}

SDL_Texture* PPU::createTexture(uint8_t width, uint8_t height) {
	return SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
											 SDL_TEXTUREACCESS_STREAMING,
											 width, height);
}
