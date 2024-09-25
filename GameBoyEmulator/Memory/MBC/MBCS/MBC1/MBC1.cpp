#include "MBC1.h"

#include <iostream>

MBC1::MBC1(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	//eram.resize(static_cast<size_t>(cartridge.ramSize) * 1024);
	eram.resize(static_cast<size_t>(cartridge.romSize) * 1024);
}

uint8_t MBC1::fetch8(uint16_t address) {
	if(address < 0x4000) { // bank 0 (fixed)
		return rom[address];
	} else if(address < 0x8000) {
		// Switchable ROM bank
		// bank 0 isn't allowed in this region
		uint8_t bank = (curRomBank == 0) ? 1 : curRomBank;
		uint16_t addr = (address - 0x4000) + (bank * 0x4000);
		
		return rom[addr];
	} else if (address >= 0xA000 && address <= 0xBFFF) {
		// Switchable RAM bank
		if (ramEnabled) {
			uint16_t addr = address - 0xA000;
			uint8_t bank = (curRomBank == 0 && bankingMode) ? 1 : curRomBank;
			
			return eram[addr + (bank * 0x2000)];
		}
		
		return 0xFF;
	}
	
	return 0xFF;
}

void MBC1::write8(uint16_t address, uint8_t data) {
	// RAM enabling (0x0000 - 0x1FFF)
	if (address < 0x2000) {
		/**
		 * According to;
		 * https://gbdev.io/pandocs/MBC1.html
		 *
		 * Before external RAM can be read or written, it must be enabled by writing $A to anywhere in this address space.
		 * Any value with $A in the lower 4 bits enables the RAM attached to the MBC, and any other value disables the RAM.
		 * It is unknown why $A is the value used to enable RAM.
		 */
		
		ramEnabled = (data & 0x0F) == 0xA;
	}
	// ROM bank switching (0x2000 - 0x3FFF)
	else if (address >= 0x2000 && address <= 0x3FFF) {
		curRomBank = data & 0x1F;
		 
		if(curRomBank == 0)
			curRomBank = 1;
	}
	// ROM or RAM bank switching (0x4000 - 0x5FFF)
	else if (address >= 0x4000 && address < 0x6000) {
		if (!bankingMode) {
			// ROM bank switching (high bits)
			curRomBank &= 0x1F; // Clear upper bits
			curRomBank |= ((data & 0x3) << 5); // Set upper 2 bits (5 and 6)
		} else {
			curRamBank = data & 0x3;
		}
	}
	// ROM/RAM mode selection (0x6000 - 0x7FFF)
	else if (address >= 0x6000 && address < 0x7FFF) {
		bankingMode = data & 0x1;
	} else if(address >= 0xA000 && address < 0xC000) {
		if(!ramEnabled) {
			return;
		}
		
		uint16_t addr = address - 0xA000;
		eram[addr + (curRamBank * 0x2000)] = data;
	}
}
