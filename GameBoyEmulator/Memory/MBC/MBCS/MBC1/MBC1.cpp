#include "MBC1.h"

MBC1::MBC1(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	eram.resize(static_cast<size_t>(cartridge.ramSize) * 1024);
}

uint8_t MBC1::fetch8(uint16_t address) {
	if(address <= 0x3FFF) { // bank 0 (fixed)
		return rom[address];
	} else if(address >= 0x4000 && address <= 0x7FFF) {
		// Switchable ROM bank
		uint16_t newAddress = address - 0x4000;
		
		// bank 0 isn't allowed in this region
		uint8_t bank = (curRomBank == 0) ? 1 : curRomBank;
		return rom[(newAddress - 0x4000) + (bank * 0x4000)];
	} else if (address >= 0xA000 && address <= 0xBFFF) {
		// Switchable RAM bank
		if (ramEnabled) {
			uint16_t addr = address - 0xA000;
			uint8_t bank = (curRamBank == 0 && bankingMode) ? 1 : curRamBank;
			
			return eram[(addr - 0xA000) + (bank * 0x2000)];
		}
		
		return 0xFF;
	}
	
	return 0xFF;
}

void MBC1::write8(uint16_t address, uint8_t data) {
	// RAM enabling (0x0000 - 0x1FFF)
	if (address < 0x1FFF) {
		/**
		 * According to;
		 * https://gbdev.io/pandocs/MBC1.html
		 *
		 * Before external RAM can be read or written, it must be enabled by writing $A to anywhere in this address space.
		 * Any value with $A in the lower 4 bits enables the RAM attached to the MBC, and any other value disables the RAM.
		 * It is unknown why $A is the value used to enable RAM.
		 */
		
		uint8_t testData = data & 0xF;
		ramEnabled = (testData == 0xA);
	}
	// ROM bank switching (0x2000 - 0x3FFF)
	else if (address >= 0x2000 && address < 0x3FFF) {
		uint8_t lower5 = data & 0x1F;
		curRomBank &= 0xE0; // Clear lower 5 bits
		curRomBank |= lower5; // Set new lower 5 bits
		
		if (curRomBank == 0)
			curRomBank++; // Avoid ROM bank 0
	}
	// ROM or RAM bank switching (0x4000 - 0x5FFF)
	else if (address >= 0x4000 && address < 0x5FFF) {
		if (bankingMode) {
			// ROM bank switching (high bits)
			curRomBank &= 0x1F; // Clear upper bits
			curRomBank |= (data & 0xE0); // Set new upper bits (5 and 6)
			
			if (curRomBank == 0)
				curRomBank++; // Avoid ROM bank 0
		} else {
			curRamBank = data & 0x3;
		}
	}
	// ROM/RAM mode selection (0x6000 - 0x7FFF)
	else if (address >= 0x6000 && address < 0x8000) {
		bankingMode = (data & 0x1) == 0;
		
		if (bankingMode) {
			curRomBank = 1;
		} else {
			curRamBank = 0;
		}
	} else if(address >= 0xA000 && address < 0xBFFF) {
		if(!ramEnabled) {
			return;
		}
		
		uint16_t addr = address - 0xA000;
		if(bankingMode) {
			eram[addr + (curRomBank * 0x2000)] = data;
		} else {
			eram[addr] = data;
		}

		// TODO; Disable after use? Is it needed?
	}
}
