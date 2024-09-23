#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/LCDC.html

/**
 * LCDC Can never be locked
 *
 * NOTE FUTURE ME; https://gbdev.io/pandocs/LCDC.html#faux-layer-textboxstatus-bar
 */

class LCDC {
public:
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
public:
	uint8_t interrupt = 0;
	
	uint8_t LCDCControl = 0;
	
	// Status Registers
	uint8_t status = 0;
	
	// Viewport Y position
	uint8_t SCY = 0;
	
	// Viewport X position
	uint8_t SCX = 0;
	
	uint8_t WY = 0;
	uint8_t WX = 0;
	
	// LCD Y coordinate
	uint8_t LY = 0;
	uint8_t	LYC = 0;
	
public: //TODO; I am too lazy.. Make this private
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc7--lcd-enable
	 *
	 * Controls whether LCD and PPU are active.
	 * If not, full access to VRAM, OAM, etc. is granted.
	 */
	bool enable;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc6--window-tile-map-area
	 *
	 * Controls which background map to use.
	 * When it's 0, then the $9800 tile map is used.
	 * When it's 1, then the $9C00 tile map is used.
	 *
	 * "0x9C00-0x9FFF" : "0x9800-0x9BFF"
	 */
	bool windowTileMapArea;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc5--window-enable
	 *
	 * Controls whether the window should display or not.
	 * 
	 * NOTE; This bit is overriden by DMG;
	 * When Bit 0 is cleared, both background and window become blank (white),
	 * and the Window Display Bit is ignored in that case.
	 * Only objects may still be displayed (if enabled in Bit 1).
	 */
	bool windowEnabled;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc4--bg-and-window-tile-data-area
	 *
	 * Controls which addressing mode the BG and Window use to pick tiles.
	 *
	 * NOTE; Objects (sprites) aren’t affected by this, and will always use the $8000 addressing mode.
	 *
	 * "0x8800-0x97FF" : "0x8000-0x8FFFF"
	 */
	bool bgWinTileDataArea;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc3--bg-tile-map-area
	 *
	 * This works similarly to tile map area(bit 6), the BG uses tilemap $9800,
	 * otherwise it will use tilemap $9C00.
	 */
	bool bgTileMapArea;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc2--obj-size
	 * 
	 * Controls the size of objects (1 tile or 2 stacked vertically)
	 * 8x8 - 8x16
	 * 
	 */
	bool objSize;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc1--obj-enable
	 *
	 * Toggles whether objects are displayed or not.
	 */
	bool objEnabled;
	
	/**
	 * https://gbdev.io/pandocs/LCDC.html#lcdc0--bg-and-window-enablepriority
	 *
	 *	Controls whether BG and Window should be displayed.
	 *	Only objects can be displayed if bit 1 (objEnabled) is 1.
	 *
	 *	NOTE; Has different meanings depending on mode.
	 */
	bool bgWindowEnabled;
};
