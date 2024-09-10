#include "LCDC.h"

#include <iostream>

#include "../Utility/Bitwise.h"

uint8_t LCDC::fetch8(uint16_t address) {
	if(address == 0xFF40) {
		return LCDCControl;
	} else if(address == 0xFF41) {
		// https://gbdev.io/pandocs/STAT.html#ff41--stat-lcd-status
		return status;
	} else if(address == 0xFF42) {
		// $FF42	SCY	Viewport Y position	R/W	All
		return SCY;
	} else if(address == 0xFF43) {
		// $FF43	SCX	Viewport X position	R/W	All
		return SCX;
	} else if(address == 0xFF44) {
		// https://gbdev.io/pandocs/STAT.html#ff44--ly-lcd-y-coordinate-read-only
		return LY;
		//return 0x90;
	} else if(address == 0xFF45) {
		// https://gbdev.io/pandocs/STAT.html#ff45--lyc-ly-compare
		return LYC;
	} else {
		std::cerr << "Fetch8 LCD???\n";
	}
	
    return 0;
}

void LCDC::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF41) {
		// https://gbdev.io/pandocs/STAT.html#ff41--stat-lcd-status
		status = data;
	} else if(address == 0xFF42) {
		// $FF42	SCY	Viewport Y position	R/W	All
		SCY = data;
	} else if(address == 0xFF43) {
		// $FF43	SCX	Viewport X position	R/W	All
		SCX = data;
	} else if(address == 0xFF44) {
		// https://gbdev.io/pandocs/STAT.html#ff44--ly-lcd-y-coordinate-read-only
		LY = data;
	} else if(address == 0xFF45) {
		// https://gbdev.io/pandocs/STAT.html#ff45--lyc-ly-compare
		LYC = data;
	} else if(address == 0xFF40) {
		/**
		 * https://gbdev.io/pandocs/LCDC.html#ff40--data-lcd-control
		 * 
		 * LCD & PPU enable: 0 = Off; 1 = On -> Bit 7
		 * Window tile map area: 0 = 9800–9BFF; 1 = 9C00–9FFF -> Bit 6
		 * Window enable: 0 = Off; 1 = On -> Bit 5
		 * BG & Window tile data area: 0 = 8800–97FF; 1 = 8000–8FFF -> Bit 4
		 * BG tile map area: 0 = 9800–9BFF; 1 = 9C00–9FFF -> Bit 3
		 * OBJ size: 0 = 8×8; 1 = 8×16 -> Bit 2
		 * OBJ enable: 0 = Off; 1 = On -> Bit 1
		 * BG & Window enable / priority [Different meaning in CGB Mode]: 0 = Off; 1 = On -> Bit 0
		 */
		
		LCDCControl = data;
		
		// LCD & PPU enable (Bit 7)
		enable = check_bit(data, 7);
		
		// Window tile map area (Bit 6)
		tileMapArea = check_bit(data, 6);
		
		// Window enable (Bit 5)
		windowEnabled = check_bit(data, 5);
		
		// BG & Window tile data area (Bit 4)
		bgWindowTileDataArea = check_bit(data, 4);
		//std::cerr << "BG & Window Tile Data Area: " << (bgWindowTileDataArea ? "0x8000-0x8FFF" : "0x8800-0x97FF") << "\n";
		
		// BG tile map area (Bit 3)
		bgTileMapArea = check_bit(data, 3);
		
		// OBJ size (Bit 2)
		objSize = check_bit(data, 2);
		
		// OBJ enable (Bit 1)
		objEnabled = check_bit(data, 1);
		
		// BG & Window enable / priority (Bit 0)
		bgWindowEnabled = check_bit(data, 0);
	}
}
