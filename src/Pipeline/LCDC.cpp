#include "LCDC.h"

#include <cassert>
#include <iostream>
#include <string>

#include "PPU.h"
#include "../Utility/Bitwise.h"

uint8_t LCDC::fetch8(uint16_t address) {
	if(address == 0xFF40) {
		return LCDCControl;
	} else if(address == 0xFF41) {
		// https://gbdev.io/pandocs/STAT.html#ff41--stat-lcd-status
		
		return 0x80 | (lycInc ? 0x40 : 0) | (mode2 ? 0x20 : 0) | (mode1 ? 0x10 : 0) | (mode0 ? 0x08 : 0) | ((LY == LYC) ? 0x04 : 0) | PPU::mode;
	} else if(address == 0xFF42) {
		// $FF42	SCY	Viewport Y position	R/W	All
		return SCY;
	} else if(address == 0xFF43) {
		// $FF43	SCX	Viewport X position	R/W	All
		return SCX;
	} else if(address == 0xFF44) {
		// https://gbdev.io/pandocs/STAT.html#ff44--ly-lcd-y-coordinate-read-only
		return LY;
		
		// For blarggs tests
		//return 0x90;
	} else if(address == 0xFF45) {
		// https://gbdev.io/pandocs/STAT.html#ff45--lyc-ly-compare
		return LYC;
	}  else if(address == 0xFF4A) {
		// https://gbdev.io/pandocs/Scrolling.html#ff4aff4b--wy-wx-window-y-position-x-position-plus-7
		return WY;
	}  else if(address == 0xFF4B) {
		// https://gbdev.io/pandocs/Scrolling.html#ff4aff4b--wy-wx-window-y-position-x-position-plus-7
		return WX;
	} else {
		std::cerr << "Fetch8 LCD???\n";
	}
	
    return 0xFF;
}

void LCDC::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF41) {
		// https://gbdev.io/pandocs/STAT.html#ff41--stat-lcd-status
		//status = data;
		
		lycInc = (data & 0x40) == 0x40;
		mode2  = (data & 0x20) == 0x20;
		mode1  = (data & 0x10) == 0x10;
		mode0  = (data & 0x08) == 0x08;
		
		// Check for LYC == LY interrupt (Bit 6)
		/*if ((status & 0x40) && (LY == LYC)) {
			interrupt |= 0x02;
		}
		
		// Check for Mode 2 (OAM Scan) interrupt (Bit 5)
		if ((status & 0x20) && (PPU::mode == PPU::OAMScan)) {
			interrupt |= 0x02;
		}
		
		// Check for Mode 1 (VBlank) interrupt (Bit 4)
		if ((status & 0x10) && (PPU::mode == PPU::VBlank)) {
			interrupt |= 0x02;
		}
		
		// Check for Mode 0 (HBlank) interrupt (Bit 3)
		if ((status & 0x08) && (PPU::mode == PPU::HBlank)) {
			interrupt |= 0x02;
		}*/
	} else if(address == 0xFF42) {
		// $FF42	SCY	Viewport Y position	R/W	All
		SCY = data;
	} else if(address == 0xFF43) {
		// $FF43	SCX	Viewport X position	R/W	All
		if(SCX == 0 && data != 0) {
			//printf("F");
		}
		
		//if(data != 0)
			SCX = data;
		//std::cerr << "SCX: " << std::to_string(SCX) << "\n";
	} else if(address == 0xFF44) {
		// https://gbdev.io/pandocs/STAT.html#ff44--ly-lcd-y-coordinate-read-only
		// This is meant to be read only
		//LY = data;
	} else if(address == 0xFF45) {
		// https://gbdev.io/pandocs/STAT.html#ff45--lyc-ly-compare
		LYC = data;
		
		if(LYC == LY) {
			interrupt |= 0x02;
		}
	}  else if(address == 0xFF4A) {
		// https://gbdev.io/pandocs/Scrolling.html#ff4aff4b--wy-wx-window-y-position-x-position-plus-7
		WY = data;
	}  else if(address == 0xFF4B) {
		// https://gbdev.io/pandocs/Scrolling.html#ff4aff4b--wy-wx-window-y-position-x-position-plus-7
		WX = data;
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
		windowTileMapArea = check_bit(data, 6); // 0 = 9800–9BFF; 1 = 9C00–9FFF
		
		// Window enable (Bit 5)
		windowEnabled = check_bit(data, 5);
		
		// BG & Window tile data area (Bit 4)
		bgWinTileDataArea = check_bit(data, 4);
		//std::cerr << "BG & Window Tile Data Area: " << (bgWinTileDataArea ? "0x8000-0x8FFF" : "0x8800-0x97FF") << "\n";
		
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
